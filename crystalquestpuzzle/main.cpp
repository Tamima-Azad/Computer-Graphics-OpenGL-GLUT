// ============================================================================
//  CRYSTAL QUEST : The Graduation Mystery   (VERSION 3 - real 3D gameplay)
//  Language : C++ (C++98 compatible)   Graphics : legacy OpenGL + GLU + GLUT
//
//  Version 1 : puzzle-locked crystals, 5 levels, timer, CGPA, graduation portal
//  Version 2 : puzzle gate, time freeze, mystery crystal, dynamic difficulty,
//              final exam, recovery challenge
//  Version 3 (NEW) : the player can now move in HEIGHT, not only on the ground
//    1. JUMP (SPACE) with gravity. Jump over obstacles! A blob shadow shows
//       where you will land.
//    2. STAIR TOWER (levels 1 and 5): climb 3 stone steps by jumping to reach
//       a crystal on top.
//    3. PUZZLE LIFT + SKY DECK (levels 3 and 5): solve the console puzzle to
//       power the lift, ride it up and collect 2 crystals on the Sky Deck.
//    4. LIVE QUIZ ZONE (level 4): the question appears at the top of the
//       screen and you must walk onto the floor plate (1-4) with the correct
//       answer and stay there for 1.5 seconds - while the obstacles keep moving!
//
//  Controls : W / A / S / D   -> move (relative to the camera)
//             SPACE           -> jump
//             Arrow keys      -> rotate / tilt the camera
//             F               -> use crystal / gate / lift console / start or cancel quiz
//             1 2 3 4         -> choose an answer      H -> hint
//             ENTER / SPACE   -> continue (menus)
//             R               -> restart the whole game
//             ESC             -> quit
// ============================================================================
#define GLUT_DISABLE_ATEXIT_HACK
#include <windows.h>
#include <GL/glut.h>   // GLUT (also pulls in gl.h and glu.h)
#include <cmath>       // sin, cos, sqrt, atan2
#include <cstdio>      // sprintf
#include <cstdlib>     // rand, srand, exit
#include <ctime>       // time
#include <cctype>      // tolower

// ----------------------------------------------------------------------------
//  CONSTANTS
// ----------------------------------------------------------------------------
const float PI              = 3.14159265f;
const float ARENA           = 10.0f;   // arena goes from -ARENA to +ARENA on X and Z
const float WALL_HEIGHT     = 1.5f;
const int   MAX_CRYSTALS    = 7;       // slot 6 is also used for the mystery crystal in level 4
const int   NUM_OBSTACLES   = 3;
const int   NUM_LEVELS      = 5;
const float PLAYER_SPEED    = 6.0f;    // units per second
const float PLAYER_RADIUS   = 0.40f;   // used for collision checks
const float CRYSTAL_RADIUS  = 0.50f;
const float OBSTACLE_RADIUS = 0.70f;
const float INTERACT_DIST   = 1.8f;    // how close you must be to press F
const float MAX_ENERGY      = 100.0f;
const float PORTAL_X        = 0.0f;    // graduation portal position
const float PORTAL_Z        = -7.0f;
const int   MAX_CONFETTI    = 150;

// --- Puzzle gate / vault (level 2) ---
const float GATE_X          = 8.5f;
const float GATE_Z          = -6.5f;
const float FREEZE_SECONDS  = 5.0f;
const int   EXAM_QUESTIONS  = 3;
const int   EXAM_PASS       = 2;
const int   RECOVERY_NEEDED = 2;

// --- Version 3: 3D movement ---
const float GRAVITY         = 18.0f;   // downward acceleration
const float JUMP_SPEED      = 8.0f;    // start speed of a jump (apex is about 1.8 units)
const float STEP_UP         = 0.35f;   // small height differences are walked up automatically
const float OBSTACLE_TOP    = 1.3f;    // you must be higher than this to jump over an obstacle
const int   MAX_PLATFORMS   = 8;
const float TOWER_TOP       = 2.1f;    // height of the highest tower step
const float DECK_H          = 3.0f;    // height of the Sky Deck
const float LIFT_SPEED      = 1.6f;
const float CONSOLE_X       = -3.6f;   // lift console position
const float CONSOLE_Z       = -8.6f;

// --- Version 3: Live Quiz Zone (4 floor plates) ---
const float PLATE_X[4]      = { -8.5f, -6.8f, -8.5f, -6.8f };
const float PLATE_Z[4]      = {  5.0f,  5.0f,  6.8f,  6.8f };
const float PLATE_HALF      = 0.75f;
const float QUIZ_TIME       = 30.0f;   // seconds to answer
const float QUIZ_HOLD       = 1.5f;    // seconds you must stand on a plate to lock in

// All possible game states. Each state has its own keyboard controls.
enum GameState {
    STATE_LEVEL_INTRO,      // level title screen, waiting for ENTER
    STATE_PLAYING,          // walking around the 3D arena
    STATE_PUZZLE,           // question panel is open (everything else paused)
    STATE_LEVEL_COMPLETE,   // level summary + grade
    STATE_GAME_OVER,        // time is up
    STATE_GRADUATED         // final ceremony
};

enum PuzzlePhase { PZ_ASKING, PZ_FEEDBACK };
enum Category    { CAT_MATH = 0, CAT_LOGIC = 1, CAT_CODE = 2 };

// What the open question panel is for
enum PuzzleType  { TG_CRYSTAL, TG_GATE, TG_EXAM, TG_RECOVERY, TG_LIFT };

// ----------------------------------------------------------------------------
//  DATA STRUCTURES
// ----------------------------------------------------------------------------
struct Crystal {
    float x, z;        // position on the ground (y is computed when drawing)
    float baseY;       // height of the floor under the crystal (tower / deck)
    bool  active;      // false = this slot is not used in the current level
    bool  collected;   // true after the puzzle was solved
    bool  hidden;      // true for the mystery crystal
    bool  inVault;     // true if it stands behind the puzzle gate
    bool  quiz;        // true = solved with the Live Quiz plates
    float phase;       // random number so the crystals do not all move in sync
    int   category;    // CAT_MATH / CAT_LOGIC / CAT_CODE
    int   baseDiff;    // difficulty given when the level was created
    int   difficulty;  // current difficulty (baseDiff + dynamic adjustment)
    float burst;       // seconds since it was solved (-1 = not solved yet)
};

struct Obstacle {
    int   type;        // 0 = moves along X, 1 = moves along Z, 2 = moves in a circle
    float cx, cz;      // centre of the movement
    float amp;         // how far it moves (amplitude / radius)
    float speed;       // how fast it moves
    float phase;       // starting offset
    float x, z;        // current position (updated every frame)
};

// A solid box you can stand on (tower step, Sky Deck, lift). It goes from the
// ground (y = 0) up to "top".
struct Platform {
    float x0, x1, z0, z1;
    float top;
    int   kind;        // 0 = tower step, 1 = deck, 2 = lift
};

// One multiple choice question
struct Question {
    const char* text;
    const char* opt[4];
    int         correct;      // index (0-3) of the right option
    int         category;
    int         difficulty;   // 1..3
    const char* hint;
};

// Settings of one level
struct LevelConfig {
    const char* name;
    const char* topic;
    int   crystals;       // crystals to solve
    float timeLimit;      // seconds
    int   catMask;        // bit0 = math, bit1 = logic, bit2 = programming
    int   minDiff, maxDiff;
    float obstacleSpeed;  // speed multiplier for the obstacles
    int   credits;        // weight of this level in the CGPA
    bool  hasGate;        // puzzle gate + vault
    bool  hasHidden;      // mystery crystal
    bool  hasTower;       // stair tower (jump puzzle)
    bool  hasLift;        // puzzle lift + Sky Deck
    bool  hasPlates;      // Live Quiz Zone
    const char* special;  // extra lines on the intro screen (or NULL)
    const char* special2;
};

struct Confetti {
    float x, y, vy, drift, phase, size, r, g, b;
};

// ----------------------------------------------------------------------------
//  QUESTION BANK  (51 questions)
// ----------------------------------------------------------------------------
const Question questionBank[] = {
    // ---------------- MATH - EASY ----------------
    {"What is 12 + 15 ?", {"27","25","29","31"}, 0, CAT_MATH, 1, "Add the tens first, then the ones."},
    {"What is 9 x 7 ?", {"54","63","72","56"}, 1, CAT_MATH, 1, "9 x 7 = 9 x 5 + 9 x 2."},
    {"What is 48 / 6 ?", {"6","7","8","9"}, 2, CAT_MATH, 1, "Think: 6 times what equals 48?"},
    {"What is 100 - 37 ?", {"73","63","67","57"}, 1, CAT_MATH, 1, "Take away 40, then add 3 back."},
    {"Solve: x + 9 = 21", {"10","11","12","13"}, 2, CAT_MATH, 1, "Subtract 9 from both sides."},
    {"Area of a rectangle 8 long and 5 wide?", {"35","40","45","26"}, 1, CAT_MATH, 1, "Area = length x width."},
    {"Next number: 5, 10, 15, 20, ?", {"25","30","24","26"}, 0, CAT_MATH, 1, "The pattern adds 5 each time."},
    {"Missing number: 3, 6, ?, 12, 15", {"8","9","10","7"}, 1, CAT_MATH, 1, "The sequence goes up by 3."},
    // ---------------- MATH - MEDIUM ----------------
    {"Solve: 3x + 5 = 20", {"3","5","7","10"}, 1, CAT_MATH, 2, "Subtract 5 first, then divide by 3."},
    {"Next number: 2, 6, 12, 20, ?", {"24","28","30","32"}, 2, CAT_MATH, 2, "Differences are 4, 6, 8 ... so add 10 next."},
    {"Solve: 2x - 4 = 10", {"5","6","7","8"}, 2, CAT_MATH, 2, "Add 4 to both sides, then divide by 2."},
    {"Perimeter of a square with side 9?", {"18","27","36","81"}, 2, CAT_MATH, 2, "Perimeter = 4 x side."},
    {"What is 15% of 200 ?", {"20","25","30","35"}, 2, CAT_MATH, 2, "10% of 200 is 20, and 5% is 10."},
    {"Next number: 1, 4, 9, 16, ?", {"20","24","25","36"}, 2, CAT_MATH, 2, "These are square numbers."},
    {"Two angles of a triangle are 50 and 60 degrees. The third one?", {"60","70","80","90"}, 1, CAT_MATH, 2, "Angles of a triangle add up to 180."},
    {"Solve: 5x = 3x + 14", {"5","6","7","8"}, 2, CAT_MATH, 2, "Subtract 3x from both sides."},
    // ---------------- MATH - HARD ----------------
    {"x^2 - 5x + 6 = 0. What is the larger root?", {"2","3","4","6"}, 1, CAT_MATH, 3, "Factor it: (x - 2)(x - 3) = 0."},
    {"Next number: 1, 1, 2, 3, 5, 8, ?", {"11","12","13","14"}, 2, CAT_MATH, 3, "Add the previous two numbers."},
    {"Circumference of a circle, radius 7 (pi = 22/7)?", {"44","22","154","88"}, 0, CAT_MATH, 3, "C = 2 x pi x r."},
    {"Sum of all whole numbers from 1 to 20?", {"190","200","210","220"}, 2, CAT_MATH, 3, "Use n(n+1)/2."},
    {"What is 2^10 ?", {"512","1024","2048","100"}, 1, CAT_MATH, 3, "2^5 = 32, now square it."},
    {"Next number: 3, 7, 15, 31, ?", {"47","55","63","62"}, 2, CAT_MATH, 3, "Double the number and add 1."},
    // ---------------- LOGIC - EASY ----------------
    {"Odd one out: Apple, Banana, Carrot, Mango", {"Apple","Banana","Carrot","Mango"}, 2, CAT_LOGIC, 1, "Three are fruits, one is a vegetable."},
    {"Next letter: A, C, E, G, ?", {"H","I","J","K"}, 1, CAT_LOGIC, 1, "Skip one letter each time."},
    {"All cats are animals. Tom is a cat. So Tom is...", {"An animal","A plant","A car","A bird"}, 0, CAT_LOGIC, 1, "Tom belongs to the group 'cats'."},
    // ---------------- LOGIC - MEDIUM ----------------
    {"Odd one out: 2, 4, 9, 8", {"2","4","9","8"}, 2, CAT_LOGIC, 2, "Look for the odd (not even) number."},
    {"If A=1, B=2, C=3 ... what is C + A + B ?", {"5","6","7","8"}, 1, CAT_LOGIC, 2, "Calculate 3 + 1 + 2."},
    {"Next number: 1, 2, 4, 8, 16, ?", {"24","30","32","36"}, 2, CAT_LOGIC, 2, "Each number is doubled."},
    {"All Bloops are Razzies. All Razzies are Lazzies. Are all Bloops Lazzies?", {"Yes","No","Maybe","Only some"}, 0, CAT_LOGIC, 2, "Follow the chain: Bloop -> Razzie -> Lazzie."},
    {"Code: each letter moves 1 step forward (A->B). CAT = ?", {"DBU","DAU","CBU","DBT"}, 0, CAT_LOGIC, 2, "C->D, A->B, T->U."},
    {"Arrange 7, 3, 9, 1 from smallest to largest:", {"1, 3, 7, 9","9, 7, 3, 1","3, 1, 7, 9","1, 7, 3, 9"}, 0, CAT_LOGIC, 2, "Ascending means smallest first."},
    // ---------------- LOGIC - HARD ----------------
    {"A is faster than B. B is faster than C. D is slower than C. Who is slowest?", {"A","B","C","D"}, 3, CAT_LOGIC, 3, "Write the order: A > B > C > D."},
    {"Code: letters moved 2 steps forward. JGNNQ decodes to?", {"HELLO","WORLD","HAPPY","QUEST"}, 0, CAT_LOGIC, 3, "Move each letter 2 steps backward."},
    {"Next number: 1, 3, 6, 10, 15, ?", {"19","20","21","22"}, 2, CAT_LOGIC, 3, "Differences are 2, 3, 4, 5 ... then 6."},
    {"Odd one out: 16, 25, 48, 49", {"16","25","48","49"}, 2, CAT_LOGIC, 3, "Three of them are perfect squares."},
    {"Two fathers and two sons ate 3 eggs, one each. Minimum number of people?", {"2","3","4","5"}, 1, CAT_LOGIC, 3, "One man can be both a father and a son."},
    // ---------------- PROGRAMMING - EASY ----------------
    {"int x = 5; x = x + 3;   What is x ?", {"5","3","8","15"}, 2, CAT_CODE, 1, "x becomes 5 + 3."},
    {"Which symbol ends a statement in C++ ?", {";",":",".",","}, 0, CAT_CODE, 1, "Look at the end of any line of code."},
    {"What is 7 % 3 in C++ ?", {"1","2","0","3"}, 0, CAT_CODE, 1, "% gives the remainder of a division."},
    {"Which one is a loop in C++ ?", {"if","while","switch","return"}, 1, CAT_CODE, 1, "A loop repeats some code."},
    // ---------------- PROGRAMMING - MEDIUM ----------------
    {"for(int i=0; i<5; i++)  How many times does the loop run?", {"4","5","6","0"}, 1, CAT_CODE, 2, "i takes the values 0, 1, 2, 3, 4."},
    {"int a = 10 / 4;   What is a ?", {"2.5","2","3","4"}, 1, CAT_CODE, 2, "Integer division drops the decimal part."},
    {"In C++ the first array element has index...", {"0","1","-1","2"}, 0, CAT_CODE, 2, "Arrays start counting at zero."},
    {"int x = 2; x *= 5;   What is x ?", {"7","10","25","12"}, 1, CAT_CODE, 2, "x *= 5 means x = x * 5."},
    {"What does   while(false) { ... }   do?", {"Runs forever","Runs once","Never runs","Compile error"}, 2, CAT_CODE, 2, "The condition is checked before the body runs."},
    // ---------------- PROGRAMMING - HARD ----------------
    {"Time complexity of binary search?", {"O(n)","O(log n)","O(n^2)","O(1)"}, 1, CAT_CODE, 3, "The search range is halved every step."},
    {"Binary 1010 equals which decimal number?", {"8","10","12","14"}, 1, CAT_CODE, 3, "8 + 0 + 2 + 0."},
    {"int s=0; for(i=1;i<=4;i++) s += i*i;   What is s ?", {"14","20","30","10"}, 2, CAT_CODE, 3, "1 + 4 + 9 + 16."},
    {"A stack follows which order?", {"FIFO","LIFO","Random","Sorted"}, 1, CAT_CODE, 3, "Last in, first out."},
    {"Worst-case time complexity of bubble sort?", {"O(n)","O(n log n)","O(n^2)","O(log n)"}, 2, CAT_CODE, 3, "Two nested loops over n items."},
    {"int x=7; cout << (x>5 && x<10);   prints...", {"0","1","true","7"}, 1, CAT_CODE, 3, "Both conditions are true, and true prints as 1."}
};
const int NUM_QUESTIONS = (int)(sizeof(questionBank) / sizeof(questionBank[0]));

const char* catNames[3]  = { "Mathematics", "Logic", "Programming" };
const char* diffNames[4] = { "", "Easy", "Medium", "Hard" };

// ----------------------------------------------------------------------------
//  LEVEL TABLE
//  catMask: 1 = math, 2 = logic, 4 = programming
//  Flags after the credits: gate, hidden, tower, lift, plates
// ----------------------------------------------------------------------------
const LevelConfig levels[NUM_LEVELS] = {
    { "LEVEL 1: Knowledge Awakening",    "Basic mathematics",              3, 120.0f, 1, 1, 1, 1.00f, 2,
      false, false, true,  false, false,
      "SPACE = jump!  Climb the stair tower to reach a crystal.", NULL },
    { "LEVEL 2: Logic Labyrinth",        "Logic and pattern questions",    4, 110.0f, 2, 1, 2, 1.15f, 2,
      true,  false, false, false, false,
      "A locked puzzle gate guards a vault with 2 crystals!", NULL },
    { "LEVEL 3: Code Dimension",         "Programming and algorithms",     5, 100.0f, 4, 2, 2, 1.50f, 3,
      false, false, false, true,  false,
      "Obstacles are much faster - jump over them!",
      "Solve the console puzzle to power the lift to the Sky Deck!" },
    { "LEVEL 4: Mystery Examination",    "Mixed mathematics and logic",    6,  90.0f, 3, 2, 3, 1.70f, 3,
      false, true,  false, false, true,
      "Rumor: Math + Logic mastery reveals a mystery crystal.",
      "Quiz Zone: stand on the plate with the right answer!" },
    { "LEVEL 5: Final Graduation Trial", "Hard mixed challenges",          7, 120.0f, 7, 1, 3, 2.00f, 4,
      false, false, true,  true,  false,
      "Pass the final exam (2 of 3), then reach the portal!",
      "Climb the tower, power the lift, jump over obstacles!" }
};

// ----------------------------------------------------------------------------
//  GLOBAL VARIABLES
// ----------------------------------------------------------------------------
int   winW = 900, winH = 600;          // window size

GameState state = STATE_LEVEL_INTRO;

// Player
float playerX = 0.0f, playerZ = 8.0f;  // position on the ground plane
float playerY = 0.0f;                  // height (feet) - NEW in version 3
float velY = 0.0f;                     // vertical speed
bool  onGround = true;
bool  jumpRequested = false;
float playerAngle = 180.0f;            // direction the player is facing (degrees)
float invuln = 0.0f;                   // seconds of invulnerability left
float energy = MAX_ENERGY;             // Knowledge Energy (0..100)

// Score / academics
int   score = 0;                       // total gameplay score (NOT the CGPA)
int   totalCorrect = 0, totalWrong = 0;
int   levelCorrect = 0, levelWrong = 0;
int   snapScore = 0, snapCorrect = 0, snapWrong = 0;   // values at level start (for retry)
int   collectedCount = 0;              // normal crystals solved in the current level
float gradePoints[NUM_LEVELS];         // grade point (0..4) of each completed level
int   gradesDone = 0;                  // how many levels have a grade
float cgpa = 0.0f;
const char* lastGrade = "-";           // letter grade of the last completed level
float lastLevelAcc = 0.0f, lastLevelPerf = 0.0f, lastLevelGP = 0.0f;

int   currentLevel = 0;                // 0..4
float levelTimeLeft = 0.0f;
bool  portalActive = false;
const char* failReason = "";

// Camera (orbits around the player)
float camYaw      = 0.0f;              // rotation around the Y axis (degrees)
float camPitch    = 40.0f;             // height angle (degrees)
float camDistance = 13.0f;             // distance from the player

// Objects
Crystal  crystals[MAX_CRYSTALS];
Obstacle obstacles[NUM_OBSTACLES];
bool     questionUsed[NUM_QUESTIONS];

// Platforms (tower, deck, lift)
Platform plats[MAX_PLATFORMS];
int      numPlats = 0;
int      liftPlat = -1;                // index of the lift in plats[]
bool     liftActive  = false;          // this level has a lift
bool     liftPowered = false;          // console puzzle solved
float    liftY = 0.0f;                 // current height of the lift

// Version 2 features
bool  gateActive   = false;
bool  gateUnlocked = false;
float gateOpen     = 0.0f;             // 0 = closed ... 1 = fully open (animation)
float freezeTimer  = 0.0f;             // seconds of Time Freeze left
bool  freezePending = false;
bool  mysteryRevealed = false;
int   levelMath = 0, levelLogic = 0;
int   streak = 0, wrongStreak = 0;
int   diffTrend = 0;
bool  examActive = false;
int   examIndex = 0, examCorrect = 0;
char  examNote[100] = "";
int   recoveryDone = 0;
float obstacleSpin = 0.0f;

// Live Quiz Zone
bool  quizActive = false;
float quizTime = 0.0f;
int   quizPlate = -1;                  // plate the player is standing on
float quizHold = 0.0f;                 // seconds spent on that plate

// Puzzle interface data
int         puzzleType = TG_CRYSTAL;
int         puzzleCrystal = -1;
int         puzzleQ = 0;
int         optOrder[4] = { 0, 1, 2, 3 };   // shuffled order of the options
PuzzlePhase pzPhase = PZ_ASKING;
bool        pzHintUsed = false;
int         pzReward = 0;
int         pzSelected = -1;
bool        pzCorrect = false;
float       pzTimer = 0.0f;

// Short message in the middle of the screen
char  flashMsg[80] = "";
float flashTimer = 0.0f;
float flashR = 1, flashG = 1, flashB = 1;

// Confetti for the ending
Confetti confetti[MAX_CONFETTI];

// Timers
float gameTime = 0.0f;                 // obstacle clock: only runs while PLAYING and not frozen
float animTime = 0.0f;                 // always increases (rotates / pulses crystals)
int   lastTimeMs = 0;                  // for computing delta time

// Keyboard state (true while a key is held down)
bool keyDown[256];
bool specialDown[256];

// GLU quadric object used to draw cylinders
GLUquadric* quad = NULL;

// ----------------------------------------------------------------------------
//  SMALL HELPER FUNCTIONS
// ----------------------------------------------------------------------------

// Random float between a and b
float randomRange(float a, float b)
{
    return a + (b - a) * (rand() / (float)RAND_MAX);
}

// Distance between two points on the ground (x,z)
float distance2D(float x1, float z1, float x2, float z2)
{
    float dx = x1 - x2;
    float dz = z1 - z2;
    return sqrt(dx * dx + dz * dz);
}

// Draw a coloured box centred at (x,y,z) with size (sx,sy,sz)
void drawBox(float x, float y, float z, float sx, float sy, float sz,
             float r, float g, float b)
{
    glColor3f(r, g, b);
    glPushMatrix();
        glTranslatef(x, y, z);
        glScalef(sx, sy, sz);
        glutSolidCube(1.0);
    glPopMatrix();
}

// Draw a cylinder standing on the ground plane (base at y = 0 of the current matrix)
void drawCylinder(float radius, float height)
{
    glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);          // gluCylinder points along +Z, so turn it up
        gluCylinder(quad, radius, radius, height, 20, 1);
    glPopMatrix();
}

// Set the "emission" colour of the material (makes an object glow)
void setEmission(float r, float g, float b)
{
    GLfloat e[4] = { r, g, b, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, e);
}

// Colour of crystal number i: cyan = math, magenta = logic, yellow = programming,
// shimmering rainbow = the mystery crystal
void crystalColor(int i, float& r, float& g, float& b)
{
    if (crystals[i].hidden)
    {
        r = 0.65f + 0.35f * sin(animTime * 3.0f);
        g = 0.65f + 0.35f * sin(animTime * 3.0f + 2.1f);
        b = 0.65f + 0.35f * sin(animTime * 3.0f + 4.2f);
        return;
    }
    int c = crystals[i].category;
    if (c == CAT_MATH)       { r = 0.1f; g = 0.9f; b = 1.0f; }   // cyan
    else if (c == CAT_LOGIC) { r = 1.0f; g = 0.2f; b = 0.9f; }   // magenta
    else                     { r = 1.0f; g = 0.9f; b = 0.2f; }   // yellow
}

// Show a short message in the middle of the screen
void setFlash(const char* msg, float r, float g, float b)
{
    sprintf(flashMsg, "%s", msg);
    flashTimer = 2.2f;
    flashR = r; flashG = g; flashB = b;
}

// ----------------------------------------------------------------------------
//  3D PLATFORMS (tower, deck, lift)
// ----------------------------------------------------------------------------

// Adds one platform to the list
int addPlatform(float x0, float x1, float z0, float z1, float top, int kind)
{
    if (numPlats >= MAX_PLATFORMS) return -1;
    plats[numPlats].x0 = x0;  plats[numPlats].x1 = x1;
    plats[numPlats].z0 = z0;  plats[numPlats].z1 = z1;
    plats[numPlats].top = top;
    plats[numPlats].kind = kind;
    numPlats++;
    return numPlats - 1;
}

// Creates the platforms of the current level
void buildPlatforms()
{
    const LevelConfig& L = levels[currentLevel];
    numPlats = 0;
    liftPlat = -1;

    if (L.hasTower)     // three steps going up: 0.7 -> 1.4 -> 2.1
    {
        addPlatform(5.5f, 7.0f, 3.5f, 5.0f, 0.7f, 0);
        addPlatform(7.0f, 8.5f, 3.5f, 5.0f, 1.4f, 0);
        addPlatform(7.0f, 8.5f, 5.0f, 6.5f, TOWER_TOP, 0);
    }
    if (L.hasLift)      // Sky Deck + the lift standing next to it
    {
        addPlatform(-9.4f, -6.5f, -9.4f, -6.5f, DECK_H, 1);
        liftPlat = addPlatform(-6.5f, -5.1f, -9.4f, -8.0f, 0.0f, 2);
    }
}

// Height of the highest platform under the point (x,z) that can be reached
// from height y (it must not be more than STEP_UP above y). 0 = floor.
float groundAt(float x, float z, float y)
{
    float best = 0.0f;
    for (int i = 0; i < numPlats; i++)
    {
        const Platform& p = plats[i];
        if (x < p.x0 - 0.1f || x > p.x1 + 0.1f) continue;
        if (z < p.z0 - 0.1f || z > p.z1 + 0.1f) continue;
        if (p.top <= y + STEP_UP && p.top > best) best = p.top;
    }
    return best;
}

// Would the player collide with the side of a platform at (nx,nz)?
bool blockedByPlatform(float nx, float nz)
{
    for (int i = 0; i < numPlats; i++)
    {
        const Platform& p = plats[i];
        if (p.top <= playerY + STEP_UP) continue;        // low enough to step on
        if (nx > p.x0 - PLAYER_RADIUS && nx < p.x1 + PLAYER_RADIUS &&
            nz > p.z0 - PLAYER_RADIUS && nz < p.z1 + PLAYER_RADIUS)
            return true;
    }
    return false;
}

// Is the player standing on the lift pad?
bool playerOnLift()
{
    if (!liftActive || liftPlat < 0) return false;
    const Platform& p = plats[liftPlat];
    if (playerX < p.x0 || playerX > p.x1 || playerZ < p.z0 || playerZ > p.z1) return false;
    return (playerY >= p.top - 0.05f && playerY <= p.top + 0.3f);
}

// ----------------------------------------------------------------------------
//  GAME LOGIC
// ----------------------------------------------------------------------------

// Sets up the 3 moving obstacles
void initObstacles()
{
    // Obstacle 0: slides left <-> right
    obstacles[0].type = 0;  obstacles[0].cx = 0.0f;  obstacles[0].cz = 2.0f;
    obstacles[0].amp = 7.0f; obstacles[0].speed = 1.0f; obstacles[0].phase = 0.0f;

    // Obstacle 1: slides forward <-> backward
    obstacles[1].type = 1;  obstacles[1].cx = -5.0f; obstacles[1].cz = 0.0f;
    obstacles[1].amp = 7.0f; obstacles[1].speed = 0.8f; obstacles[1].phase = 1.0f;

    // Obstacle 2: moves in a circle
    obstacles[2].type = 2;  obstacles[2].cx = 2.0f;  obstacles[2].cz = -3.0f;
    obstacles[2].amp = 5.0f; obstacles[2].speed = 0.9f; obstacles[2].phase = 0.0f;
}

// Calculates the current position of every obstacle from the obstacle clock.
// While Time Freeze is active the clock does not advance.
void updateObstacles()
{
    float mul = levels[currentLevel].obstacleSpeed;
    for (int i = 0; i < NUM_OBSTACLES; i++)
    {
        Obstacle& o = obstacles[i];
        float a = o.speed * mul * gameTime + o.phase;

        if (o.type == 0)        // back and forth along X
        {
            o.x = o.cx + o.amp * sin(a);
            o.z = o.cz;
        }
        else if (o.type == 1)   // back and forth along Z
        {
            o.x = o.cx;
            o.z = o.cz + o.amp * sin(a);
        }
        else                    // circle
        {
            o.x = o.cx + o.amp * cos(a);
            o.z = o.cz + o.amp * sin(a);
        }
    }
}

// Areas that are reserved for vault / tower / deck / quiz zone.
// Random crystals are never placed there.
bool inReservedZone(const LevelConfig& L, float x, float z)
{
    if (L.hasGate   && x >  5.0f && z < -5.0f) return true;
    if (L.hasTower  && x >  4.5f && z >  2.8f && z < 7.5f) return true;
    if (L.hasLift   && x < -2.5f && z < -5.5f) return true;
    if (L.hasPlates && x < -5.0f && z >  2.5f) return true;
    return false;
}

// Places the crystals of the current level.
// The LAST crystals of a level get special roles:
//   role 1 = inside the vault   role 2 = on the Sky Deck
//   role 3 = on top of the tower  role 4 = the Live Quiz crystal
// All others are placed randomly (max. 200 tries each, so no infinite loop).
void placeCrystals()
{
    const LevelConfig& L = levels[currentLevel];

    // fixed backup positions (all outside the reserved zones)
    const float fallback[MAX_CRYSTALS][2] = {
        { 3.0f, -4.0f }, { 7.0f, 2.0f }, { -7.0f, 0.0f }, { 7.0f, 0.0f },
        { -3.0f, 4.0f }, { 3.0f, 4.0f }, { 0.0f, -2.0f }
    };
    const float vaultSpots[2][2] = { { 8.0f, -8.8f }, { 9.2f, -7.6f } };
    const float deckSpots[2][2]  = { { -8.5f, -7.4f }, { -7.4f, -8.6f } };

    // decide the role of every crystal slot
    int role[MAX_CRYSTALS];
    for (int i = 0; i < MAX_CRYSTALS; i++) role[i] = 0;
    int next = L.crystals - 1;
    if (L.hasLift)   { role[next--] = 2; role[next--] = 2; }
    if (L.hasGate)   { role[next--] = 1; role[next--] = 1; }
    if (L.hasTower)  { role[next--] = 3; }
    if (L.hasPlates) { role[next--] = 4; }

    // list of categories allowed in this level
    int cats[3];
    int numCats = 0;
    for (int c = 0; c < 3; c++)
        if (L.catMask & (1 << c)) cats[numCats++] = c;

    int vaultIdx = 0, deckIdx = 0;

    for (int i = 0; i < MAX_CRYSTALS; i++)
    {
        // default values (also used for unused slots)
        crystals[i].active     = false;
        crystals[i].collected  = false;
        crystals[i].hidden     = false;
        crystals[i].inVault    = false;
        crystals[i].quiz       = false;
        crystals[i].burst      = -1.0f;
        crystals[i].x = 0.0f;
        crystals[i].z = 0.0f;
        crystals[i].baseY      = 0.0f;
        crystals[i].phase      = 0.0f;
        crystals[i].category   = CAT_MATH;
        crystals[i].baseDiff   = 1;
        crystals[i].difficulty = 1;

        if (i >= L.crystals) continue;
        crystals[i].active = true;

        if (role[i] == 1)           // inside the locked vault
        {
            crystals[i].x = vaultSpots[vaultIdx][0];
            crystals[i].z = vaultSpots[vaultIdx][1];
            crystals[i].inVault = true;
            vaultIdx++;
        }
        else if (role[i] == 2)      // on the Sky Deck
        {
            crystals[i].x = deckSpots[deckIdx][0];
            crystals[i].z = deckSpots[deckIdx][1];
            crystals[i].baseY = DECK_H;
            deckIdx++;
        }
        else if (role[i] == 3)      // on top of the tower
        {
            crystals[i].x = 7.75f;
            crystals[i].z = 5.75f;
            crystals[i].baseY = TOWER_TOP;
        }
        else if (role[i] == 4)      // Live Quiz crystal
        {
            crystals[i].x = -7.6f;
            crystals[i].z = 3.4f;
            crystals[i].quiz = true;
        }
        else
        {
            bool ok = false;
            for (int attempt = 0; attempt < 200 && !ok; attempt++)
            {
                crystals[i].x = randomRange(-ARENA + 1.5f, ARENA - 1.5f);
                crystals[i].z = randomRange(-ARENA + 1.5f, ARENA - 1.5f);
                ok = true;

                // not too close to the player start position
                if (distance2D(crystals[i].x, crystals[i].z, 0.0f, 8.0f) < 3.0f)
                    ok = false;

                // not on top of the graduation portal
                if (distance2D(crystals[i].x, crystals[i].z, PORTAL_X, PORTAL_Z) < 2.5f)
                    ok = false;

                // not inside a reserved zone (vault, tower, deck, quiz zone)
                if (inReservedZone(L, crystals[i].x, crystals[i].z))
                    ok = false;

                // not too close to the crystals that were already placed
                for (int j = 0; j < i; j++)
                    if (distance2D(crystals[i].x, crystals[i].z,
                                   crystals[j].x, crystals[j].z) < 2.0f)
                        ok = false;
            }
            if (!ok)    // very unlikely: use a backup position
            {
                crystals[i].x = fallback[i][0];
                crystals[i].z = fallback[i][1];
            }
        }

        crystals[i].phase    = randomRange(0.0f, 6.28f);
        crystals[i].category = cats[i % numCats];

        // base difficulty inside the difficulty range of the level
        int base;
        if (L.maxDiff - L.minDiff >= 2)
            base = (rand() % 100 < 60) ? 3 : 1 + rand() % 2;
        else
            base = L.minDiff + rand() % (L.maxDiff - L.minDiff + 1);
        crystals[i].baseDiff   = base;
        crystals[i].difficulty = base;
    }
}

// Dynamic Difficulty: a streak of correct answers makes the open crystals
// harder, a streak of wrong answers makes them easier (inside the level range).
void recomputeDifficulties()
{
    const LevelConfig& L = levels[currentLevel];
    int off = 0;
    if (streak >= 2)           off = 1;
    else if (wrongStreak >= 2) off = -1;
    diffTrend = off;

    for (int i = 0; i < MAX_CRYSTALS; i++)
    {
        if (!crystals[i].active || crystals[i].collected || crystals[i].hidden) continue;
        int d = crystals[i].baseDiff + off;
        if (d < L.minDiff) d = L.minDiff;
        if (d > L.maxDiff) d = L.maxDiff;
        crystals[i].difficulty = d;
    }
}

// Prepares level number lvl (0..4) and shows its title screen
void startLevel(int lvl)
{
    currentLevel   = lvl;
    levelTimeLeft  = levels[lvl].timeLimit;
    levelCorrect   = 0;
    levelWrong     = 0;
    collectedCount = 0;
    portalActive   = false;
    energy         = MAX_ENERGY;
    invuln         = 0.0f;
    gameTime       = 0.0f;
    flashTimer     = 0.0f;

    // Version 2 features
    gateActive      = levels[lvl].hasGate;
    gateUnlocked    = false;
    gateOpen        = 0.0f;
    freezeTimer     = 0.0f;
    freezePending   = false;
    mysteryRevealed = false;
    levelMath       = 0;
    levelLogic      = 0;
    streak          = 0;
    wrongStreak     = 0;
    diffTrend       = 0;
    examActive      = false;
    examIndex       = 0;
    examCorrect     = 0;
    examNote[0]     = '\0';
    recoveryDone    = 0;

    // Version 3 features
    liftActive  = levels[lvl].hasLift;
    liftPowered = false;
    liftY       = 0.0f;
    quizActive  = false;
    quizPlate   = -1;
    quizHold    = 0.0f;
    buildPlatforms();

    // remember the totals so that "retry level" can restore them
    snapScore   = score;
    snapCorrect = totalCorrect;
    snapWrong   = totalWrong;

    playerX     = 0.0f;
    playerZ     = 8.0f;
    playerY     = 0.0f;
    velY        = 0.0f;
    onGround    = true;
    jumpRequested = false;
    playerAngle = 180.0f;

    initObstacles();
    updateObstacles();
    placeCrystals();

    state = STATE_LEVEL_INTRO;
}

// Retry the current level after a failure (score goes back to level start)
void retryLevel()
{
    score        = snapScore;
    totalCorrect = snapCorrect;
    totalWrong   = snapWrong;
    startLevel(currentLevel);
}

// Resets everything to start a new game
void resetGame()
{
    score        = 0;
    totalCorrect = 0;
    totalWrong   = 0;
    gradesDone   = 0;
    cgpa         = 0.0f;
    lastGrade    = "-";
    for (int i = 0; i < NUM_LEVELS; i++) gradePoints[i] = 0.0f;
    for (int i = 0; i < NUM_QUESTIONS; i++) questionUsed[i] = false;
    startLevel(0);
}

// Letter grade for a grade point average
const char* letterFromGP(float gp)
{
    if (gp >= 3.5f) return "A";
    if (gp >= 2.5f) return "B";
    if (gp >= 1.5f) return "C";
    if (gp >= 0.5f) return "D";
    return "F";
}

// Calculates the grade of the level that was just finished.
//   performance (0..100) = 70% answer accuracy + 30% time left
//   >= 80 : A (4.0)   >= 65 : B (3.0)   >= 50 : C (2.0)   >= 35 : D (1.0)   else F (0.0)
// CGPA = sum(gradePoint * credits) / sum(credits)   over the completed levels.
void gradeLevel()
{
    int   tot = levelCorrect + levelWrong;
    float acc = (tot > 0) ? 100.0f * levelCorrect / (float)tot : 0.0f;
    float tr  = levelTimeLeft / levels[currentLevel].timeLimit;
    if (tr < 0.0f) tr = 0.0f;
    if (tr > 1.0f) tr = 1.0f;
    float perf = 0.7f * acc + 0.3f * 100.0f * tr;

    float gp;
    if      (perf >= 80.0f) { gp = 4.0f; lastGrade = "A"; }
    else if (perf >= 65.0f) { gp = 3.0f; lastGrade = "B"; }
    else if (perf >= 50.0f) { gp = 2.0f; lastGrade = "C"; }
    else if (perf >= 35.0f) { gp = 1.0f; lastGrade = "D"; }
    else                    { gp = 0.0f; lastGrade = "F"; }

    gradePoints[currentLevel] = gp;
    if (gradesDone < currentLevel + 1) gradesDone = currentLevel + 1;

    lastLevelAcc  = acc;
    lastLevelPerf = perf;
    lastLevelGP   = gp;

    float sum = 0.0f;
    int   cr  = 0;
    for (int i = 0; i < gradesDone; i++)
    {
        sum += gradePoints[i] * levels[i].credits;
        cr  += levels[i].credits;
    }
    cgpa = (cr > 0) ? sum / cr : 0.0f;
}

// Returns the index of the closest unsolved crystal within reach, or -1.
// The player must also be at about the same height as the crystal, so a
// crystal on the tower / deck cannot be solved from the ground.
int findNearbyCrystal()
{
    int   best  = -1;
    float bestD = INTERACT_DIST;
    for (int i = 0; i < MAX_CRYSTALS; i++)
    {
        if (!crystals[i].active || crystals[i].collected) continue;
        if (crystals[i].inVault && gateOpen < 0.5f) continue;
        if (fabs(playerY - crystals[i].baseY) > 1.3f) continue;
        float d = distance2D(playerX, playerZ, crystals[i].x, crystals[i].z);
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

// Is the player standing in front of the locked gate?
bool findNearbyGate()
{
    if (!gateActive || gateUnlocked) return false;
    return distance2D(playerX, playerZ, GATE_X, GATE_Z) < 2.4f;
}

// Is the player next to the unpowered lift console?
bool findNearbyConsole()
{
    if (!liftActive || liftPowered) return false;
    if (playerY > 1.0f) return false;
    return distance2D(playerX, playerZ, CONSOLE_X, CONSOLE_Z) < 2.0f;
}

// Which quiz plate (0..3) is the player standing on? -1 = none
int plateUnderPlayer()
{
    if (playerY > 0.4f) return -1;
    for (int i = 0; i < 4; i++)
        if (fabs(playerX - PLATE_X[i]) < PLATE_HALF && fabs(playerZ - PLATE_Z[i]) < PLATE_HALF)
            return i;
    return -1;
}

// Chooses a question with the given category and difficulty.
// 1st try: unused question   2nd try: any with same category + difficulty
// 3rd try: same category, any difficulty
int pickQuestion(int cat, int diff)
{
    int cand[NUM_QUESTIONS];
    for (int pass = 0; pass < 3; pass++)
    {
        int n = 0;
        for (int i = 0; i < NUM_QUESTIONS; i++)
        {
            const Question& q = questionBank[i];
            bool match = (q.category == cat);
            if (pass < 2 && q.difficulty != diff) match = false;
            if (pass == 0 && questionUsed[i])     match = false;
            if (match) cand[n++] = i;
        }
        if (n > 0)
        {
            int pick = cand[rand() % n];
            questionUsed[pick] = true;
            return pick;
        }
    }
    return rand() % NUM_QUESTIONS;
}

// Shuffles the order in which the four options are shown
void shuffleOptions()
{
    for (int i = 0; i < 4; i++) optOrder[i] = i;
    for (int i = 3; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int t = optOrder[i]; optOrder[i] = optOrder[j]; optOrder[j] = t;
    }
}

// ------------------------------ PUZZLE LOGIC --------------------------------

// Opens the question panel.
//   type = TG_CRYSTAL  -> ci is the crystal index (normal or mystery crystal)
//   type = TG_GATE     -> special hard logic puzzle for the gate
//   type = TG_LIFT     -> hard puzzle for the lift console
//   type = TG_EXAM     -> one question of the final exam (hard, any category)
//   type = TG_RECOVERY -> easy math question to recover from zero energy
void openPuzzle(int type, int ci)
{
    puzzleType    = type;
    puzzleCrystal = ci;
    quizActive    = false;               // a running live quiz is cancelled

    int cat = CAT_MATH, diff = 1, reward = 0;
    if (type == TG_CRYSTAL)
    {
        cat    = crystals[ci].category;
        diff   = crystals[ci].difficulty;
        reward = diff * 10;                          // easy 10, medium 20, hard 30
        if (crystals[ci].hidden)                     // mystery crystal
        {
            cat    = (rand() % 2) ? CAT_LOGIC : CAT_MATH;
            diff   = 3;
            reward = 50;                             // bonus points
        }
    }
    else if (type == TG_GATE)
    {
        cat = CAT_LOGIC; diff = 3; reward = 30;
    }
    else if (type == TG_LIFT)
    {
        cat = (rand() % 2) ? CAT_CODE : CAT_LOGIC; diff = 3; reward = 30;
    }
    else if (type == TG_EXAM)
    {
        cat = rand() % 3; diff = 3; reward = 30;
    }
    // TG_RECOVERY: easy math, no reward

    puzzleQ = pickQuestion(cat, diff);
    shuffleOptions();

    pzPhase    = PZ_ASKING;
    pzHintUsed = false;
    pzSelected = -1;
    pzReward   = reward;
    pzTimer    = 0.0f;

    state = STATE_PUZZLE;                            // movement + timers are now paused
}

// Starts the Live Quiz for crystal ci: the question is shown on the HUD and
// the player has to stand on the plate with the right answer.
void startQuiz(int ci)
{
    puzzleType    = TG_CRYSTAL;
    puzzleCrystal = ci;
    puzzleQ       = pickQuestion(crystals[ci].category, crystals[ci].difficulty);
    shuffleOptions();
    pzHintUsed = false;
    pzReward   = crystals[ci].difficulty * 10;
    pzSelected = -1;

    quizActive = true;
    quizTime   = QUIZ_TIME;
    quizPlate  = -1;
    quizHold   = 0.0f;
}

// Energy reached zero: the player must solve easy questions to continue
void startRecovery()
{
    recoveryDone = 0;
    openPuzzle(TG_RECOVERY, -1);
}

// Starts (or restarts) the final exam of level 5
void startExam(const char* note)
{
    examActive  = true;
    examIndex   = 0;
    examCorrect = 0;
    sprintf(examNote, "%s", note);
    openPuzzle(TG_EXAM, -1);
}

// H key: show the hint (costs 5 reward points and 5 energy)
void useHint()
{
    if (pzHintUsed) return;
    if (puzzleType == TG_RECOVERY) return;           // no hints in the recovery challenge
    if (energy <= 5.0f)
    {
        setFlash("Not enough energy for a hint!", 1.0f, 0.4f, 0.3f);
        return;
    }
    pzHintUsed = true;
    pzReward  -= 5;
    if (pzReward < 5) pzReward = 5;
    energy    -= 5.0f;
}

// Applies the result of an answer (score, energy, streaks, unlocking things)
void applyResult(bool correct)
{
    const Question& q = questionBank[puzzleQ];

    if (correct)
    {
        score += pzReward;
        totalCorrect++;
        levelCorrect++;
        energy += 10.0f;
        if (energy > MAX_ENERGY) energy = MAX_ENERGY;
        streak++;
        wrongStreak = 0;

        // Time Freeze reward for solving a HARD question
        if (q.difficulty == 3) freezePending = true;

        if (puzzleType == TG_CRYSTAL)
        {
            Crystal& c = crystals[puzzleCrystal];
            c.collected = true;                      // the crystal is now unlocked
            c.burst     = 0.0f;                      // start the glow animation
            if (!c.hidden)                           // the mystery crystal is a bonus
            {
                collectedCount++;
                if (c.category == CAT_MATH)  levelMath++;
                if (c.category == CAT_LOGIC) levelLogic++;
            }
        }
        else if (puzzleType == TG_GATE)
        {
            gateUnlocked = true;                     // gate animation starts in update()
        }
        else if (puzzleType == TG_LIFT)
        {
            liftPowered = true;                      // the lift works from now on
        }
        else if (puzzleType == TG_EXAM)
        {
            examCorrect++;
        }
    }
    else
    {
        totalWrong++;
        levelWrong++;
        energy -= 10.0f;
        if (energy < 0.0f) energy = 0.0f;
        wrongStreak++;
        streak = 0;
    }
    recomputeDifficulties();
}

// Player picked answer number sel (0..3 = keys 1..4) in the question panel
void submitAnswer(int sel)
{
    const Question& q = questionBank[puzzleQ];
    pzSelected = sel;
    pzCorrect  = (optOrder[sel] == q.correct);
    pzPhase    = PZ_FEEDBACK;
    pzTimer    = 0.0f;

    // recovery questions give no points and do not count in the statistics
    if (puzzleType == TG_RECOVERY) return;

    applyResult(pzCorrect);
}

// The Live Quiz is over: plate = plate number the player locked in (-1 = time out)
void finishQuiz(int plate)
{
    quizActive = false;
    bool correct = (plate >= 0 && optOrder[plate] == questionBank[puzzleQ].correct);
    pzSelected = plate;
    pzCorrect  = correct;
    pzPhase    = PZ_FEEDBACK;
    pzTimer    = 0.0f;
    applyResult(correct);
    state = STATE_PUZZLE;                            // show the result in the panel
}

// Called every frame while a Live Quiz is running
void updateQuiz(float dt)
{
    quizTime -= dt;
    int plate = plateUnderPlayer();
    if (plate >= 0 && plate == quizPlate) quizHold += dt;
    else { quizPlate = plate; quizHold = 0.0f; }

    if (quizPlate >= 0 && quizHold >= QUIZ_HOLD) finishQuiz(quizPlate);
    else if (quizTime <= 0.0f)                   finishQuiz(-1);
}

// Level 4: after 2 Math and 2 Logic crystals are solved, the mystery crystal appears
void checkMysteryReveal()
{
    if (!levels[currentLevel].hasHidden || mysteryRevealed) return;
    if (levelMath < 2 || levelLogic < 2) return;

    int i = MAX_CRYSTALS - 1;                        // the free slot
    bool ok = false;
    for (int attempt = 0; attempt < 200 && !ok; attempt++)
    {
        crystals[i].x = randomRange(-ARENA + 1.5f, ARENA - 1.5f);
        crystals[i].z = randomRange(-ARENA + 1.5f, ARENA - 1.5f);
        ok = true;
        if (distance2D(crystals[i].x, crystals[i].z, playerX, playerZ) < 3.0f) ok = false;
        for (int j = 0; j < MAX_CRYSTALS - 1; j++)
            if (crystals[j].active && !crystals[j].collected &&
                distance2D(crystals[i].x, crystals[i].z, crystals[j].x, crystals[j].z) < 2.0f)
                ok = false;
    }
    if (!ok) { crystals[i].x = -8.0f; crystals[i].z = -8.0f; }   // backup position

    crystals[i].active     = true;
    crystals[i].collected  = false;
    crystals[i].hidden     = true;
    crystals[i].inVault    = false;
    crystals[i].quiz       = false;
    crystals[i].baseY      = 0.0f;
    crystals[i].burst      = -1.0f;
    crystals[i].phase      = randomRange(0.0f, 6.28f);
    crystals[i].category   = CAT_LOGIC;
    crystals[i].baseDiff   = 3;
    crystals[i].difficulty = 3;
    mysteryRevealed = true;
    setFlash("A MYSTERY CRYSTAL HAS APPEARED!", 1.0f, 0.9f, 1.0f);
}

// Go back to the 3D arena (and start a pending Time Freeze)
void returnToPlaying()
{
    if (freezePending)
    {
        freezeTimer   = FREEZE_SECONDS;
        freezePending = false;
    }
    state = STATE_PLAYING;
}

// Closes the feedback screen and decides what happens next
void endPuzzle()
{
    // ---------- recovery challenge ----------
    if (puzzleType == TG_RECOVERY)
    {
        if (pzCorrect) recoveryDone++;
        if (recoveryDone >= RECOVERY_NEEDED)
        {
            energy = 30.0f;
            if (examActive)                          // the exam starts over
            {
                startExam("Energy restored. The exam starts again.");
                return;
            }
            setFlash("ENERGY RESTORED!  +30", 0.4f, 1.0f, 0.6f);
            state = STATE_PLAYING;
            return;
        }
        openPuzzle(TG_RECOVERY, -1);                 // next recovery question
        return;
    }

    // ---------- energy is zero ----------
    if (energy <= 0.0f)
    {
        startRecovery();
        return;
    }

    // ---------- final exam ----------
    if (puzzleType == TG_EXAM)
    {
        examIndex++;
        if (examIndex < EXAM_QUESTIONS)
        {
            openPuzzle(TG_EXAM, -1);
            return;
        }
        if (examCorrect >= EXAM_PASS)
        {
            examActive   = false;
            portalActive = true;
            setFlash("EXAM PASSED!  PORTAL UNLOCKED!", 1.0f, 0.9f, 0.3f);
            returnToPlaying();
            return;
        }
        energy -= 15.0f;                             // failed: lose energy and retake
        if (energy <= 0.0f)
        {
            energy = 0.0f;
            startRecovery();
            return;
        }
        startExam("Exam failed (2 correct needed). -15 energy. Try again!");
        return;
    }

    // ---------- crystal, gate or lift ----------
    if (pzCorrect)
    {
        if (puzzleType == TG_LIFT)
            setFlash("LIFT POWERED!  Stand on the gold pad.", 1.0f, 0.85f, 0.3f);

        checkMysteryReveal();

        if (puzzleType == TG_CRYSTAL && !crystals[puzzleCrystal].hidden &&
            collectedCount >= levels[currentLevel].crystals)
        {
            if (currentLevel < NUM_LEVELS - 1)
            {
                gradeLevel();                        // levels 1-4 end here
                freezePending = false;
                state = STATE_LEVEL_COMPLETE;
                return;
            }
            startExam("Final exam: answer 2 of 3 correctly to open the portal!");
            return;
        }
    }
    returnToPlaying();
}

// ---------------------------- PLAYER + COLLISIONS ---------------------------

// Pushes the point (px,pz) with radius rad out of the rectangle (x0..x1, z0..z1)
void pushOutOfRect(float& px, float& pz, float rad,
                   float x0, float x1, float z0, float z1)
{
    float cx = (px < x0) ? x0 : ((px > x1) ? x1 : px);   // closest point on the rectangle
    float cz = (pz < z0) ? z0 : ((pz > z1) ? z1 : pz);
    float dx = px - cx, dz = pz - cz;
    float d  = sqrt(dx * dx + dz * dz);
    if (d >= rad) return;

    if (d > 0.0001f)
    {
        px = cx + dx / d * rad;
        pz = cz + dz / d * rad;
    }
    else    // centre is inside the rectangle: leave through the nearest side
    {
        float l = px - x0, r = x1 - px, t = pz - z0, b = z1 - pz;
        if (l <= r && l <= t && l <= b) px = x0 - rad;
        else if (r <= t && r <= b)      px = x1 + rad;
        else if (t <= b)                pz = z0 - rad;
        else                            pz = z1 + rad;
    }
}

// Solid walls of the vault (and the closed gate)
void collideVault()
{
    if (!gateActive) return;
    pushOutOfRect(playerX, playerZ, PLAYER_RADIUS, 6.25f, 6.75f, -10.5f, -6.25f);  // west wall
    pushOutOfRect(playerX, playerZ, PLAYER_RADIUS, 6.25f, 7.50f, -6.75f, -6.25f);  // south wall (left part)
    pushOutOfRect(playerX, playerZ, PLAYER_RADIUS, 9.50f, 10.5f, -6.75f, -6.25f);  // south wall (right part)
    if (gateOpen < 0.6f)                                                           // the gate itself
        pushOutOfRect(playerX, playerZ, PLAYER_RADIUS, 7.50f, 9.50f, -6.75f, -6.25f);
}

// Moves the player (W/A/S/D), handles jumping, gravity and platforms
void updatePlayer(float dt)
{
    // Direction vectors on the ground, based on where the camera is looking.
    float yawRad = camYaw * PI / 180.0f;
    float fwdX = -sin(yawRad), fwdZ = -cos(yawRad);   // "forward" = away from camera
    float rgtX =  cos(yawRad), rgtZ = -sin(yawRad);   // "right"

    float dx = 0.0f, dz = 0.0f;
    if (keyDown['w']) { dx += fwdX; dz += fwdZ; }
    if (keyDown['s']) { dx -= fwdX; dz -= fwdZ; }
    if (keyDown['d']) { dx += rgtX; dz += rgtZ; }
    if (keyDown['a']) { dx -= rgtX; dz -= rgtZ; }

    float len = sqrt(dx * dx + dz * dz);
    if (len > 0.0001f)
    {
        dx /= len;  dz /= len;                          // normalise (same speed in diagonals)

        // move one axis at a time, so you can slide along platform sides
        float nx = playerX + dx * PLAYER_SPEED * dt;
        float nz = playerZ + dz * PLAYER_SPEED * dt;
        if (!blockedByPlatform(nx, playerZ)) playerX = nx;
        if (!blockedByPlatform(playerX, nz)) playerZ = nz;

        playerAngle = atan2(dx, dz) * 180.0f / PI;      // face the walking direction
    }

    collideVault();                                     // vault walls + closed gate

    // Keep the player inside the walls
    float limit = ARENA - PLAYER_RADIUS - 0.1f;
    if (playerX >  limit) playerX =  limit;
    if (playerX < -limit) playerX = -limit;
    if (playerZ >  limit) playerZ =  limit;
    if (playerZ < -limit) playerZ = -limit;

    // ---- vertical movement: jump + gravity ----
    if (jumpRequested && onGround)
    {
        velY = JUMP_SPEED;
        onGround = false;
    }
    jumpRequested = false;

    float prevY = playerY;
    velY    -= GRAVITY * dt;
    playerY += velY * dt;

    float ground = groundAt(playerX, playerZ, prevY);   // floor or platform below us
    if (playerY <= ground)
    {
        playerY  = ground;                              // landed (or standing)
        velY     = 0.0f;
        onGround = true;
    }
    else
    {
        onGround = false;                               // in the air / walked off an edge
    }
}

// Puts the player on the spot (out of 8 candidates) that is farthest from all obstacles
void respawnPlayerSafe()
{
    const float spots[8][2] = {
        {  0.0f,  8.0f }, { -8.0f,  8.0f }, { 8.0f,  8.0f }, { -8.0f, 0.0f },
        {  8.0f,  0.0f }, { -2.0f,  6.0f }, { 3.0f,  5.0f }, {  0.0f, -8.0f }
    };
    int   best  = 0;
    float bestD = -1.0f;
    for (int s = 0; s < 8; s++)
    {
        float minD = 1000.0f;
        for (int i = 0; i < NUM_OBSTACLES; i++)
        {
            float d = distance2D(spots[s][0], spots[s][1], obstacles[i].x, obstacles[i].z);
            if (d < minD) minD = d;
        }
        if (minD > bestD) { bestD = minD; best = s; }
    }
    playerX  = spots[best][0];
    playerZ  = spots[best][1];
    playerY  = 0.0f;
    velY     = 0.0f;
    onGround = true;
}

// Obstacle hits. You can jump over an obstacle (be higher than OBSTACLE_TOP).
// Frozen obstacles are harmless.
void checkCollisions()
{
    if (invuln > 0.0f || freezeTimer > 0.0f) return;
    if (playerY > OBSTACLE_TOP) return;                 // jumping over the obstacles

    for (int i = 0; i < NUM_OBSTACLES; i++)
    {
        if (distance2D(playerX, playerZ, obstacles[i].x, obstacles[i].z)
            < PLAYER_RADIUS + OBSTACLE_RADIUS)
        {
            energy -= 20.0f;
            invuln  = 2.5f;
            respawnPlayerSafe();
            setFlash("OUCH!  -20 Knowledge Energy", 1.0f, 0.4f, 0.2f);

            if (energy <= 0.0f)
            {
                energy = 0.0f;
                startRecovery();         // zero energy -> recovery challenge
            }
            return;
        }
    }
}

// ------------------------------- CONFETTI -----------------------------------
void initConfetti()
{
    for (int i = 0; i < MAX_CONFETTI; i++)
    {
        confetti[i].x     = randomRange(0.0f, (float)winW);
        confetti[i].y     = randomRange(0.0f, (float)winH);
        confetti[i].vy    = randomRange(60.0f, 160.0f);
        confetti[i].drift = randomRange(10.0f, 40.0f);
        confetti[i].phase = randomRange(0.0f, 6.28f);
        confetti[i].size  = randomRange(4.0f, 9.0f);
        confetti[i].r     = randomRange(0.3f, 1.0f);
        confetti[i].g     = randomRange(0.3f, 1.0f);
        confetti[i].b     = randomRange(0.3f, 1.0f);
    }
}

void updateConfetti(float dt)
{
    for (int i = 0; i < MAX_CONFETTI; i++)
    {
        confetti[i].y -= confetti[i].vy * dt;
        confetti[i].x += sin(animTime * 2.0f + confetti[i].phase) * confetti[i].drift * dt;
        if (confetti[i].y < -10.0f)
        {
            confetti[i].y = winH + 10.0f;
            confetti[i].x = randomRange(0.0f, (float)winW);
        }
    }
}

// ---------------------------------- UPDATE ----------------------------------

// Moves the lift: it rises (only if powered) while the player stands on it
// and sinks back when nobody is on it.
void updateLift(float dt)
{
    if (!liftActive || liftPlat < 0) return;

    bool  on     = playerOnLift();                 // checked BEFORE the lift moves
    float target = (liftPowered && on && state == STATE_PLAYING) ? DECK_H : 0.0f;
    if (state != STATE_PLAYING) return;            // frozen while a menu is open

    if (liftY < target)      { liftY += LIFT_SPEED * dt; if (liftY > target) liftY = target; }
    else if (liftY > target) { liftY -= LIFT_SPEED * dt; if (liftY < target) liftY = target; }
    plats[liftPlat].top = liftY;
}

// Called about 60 times per second
void update(float dt)
{
    animTime += dt;     // crystals always animate

    // Camera control with the arrow keys (works in every state)
    if (specialDown[GLUT_KEY_LEFT])  camYaw   -= 90.0f * dt;
    if (specialDown[GLUT_KEY_RIGHT]) camYaw   += 90.0f * dt;
    if (specialDown[GLUT_KEY_UP])    camPitch += 45.0f * dt;
    if (specialDown[GLUT_KEY_DOWN])  camPitch -= 45.0f * dt;
    if (camPitch > 80.0f) camPitch = 80.0f;
    if (camPitch < 10.0f) camPitch = 10.0f;

    if (invuln > 0.0f)     invuln     -= dt;
    if (flashTimer > 0.0f) flashTimer -= dt;

    // gate opening animation
    if (gateUnlocked && gateOpen < 1.0f)
    {
        gateOpen += dt * 0.8f;
        if (gateOpen > 1.0f) gateOpen = 1.0f;
    }

    // glow animation of solved crystals
    for (int i = 0; i < MAX_CRYSTALS; i++)
        if (crystals[i].active && crystals[i].collected && crystals[i].burst < 50.0f)
            crystals[i].burst += dt;

    if (state == STATE_PLAYING)
    {
        if (freezeTimer > 0.0f)
        {
            freezeTimer -= dt;                 // obstacles stay frozen
            if (freezeTimer < 0.0f) freezeTimer = 0.0f;
        }
        else
        {
            gameTime     += dt;                // obstacle clock runs
            obstacleSpin += dt;
        }

        levelTimeLeft -= dt;
        updateObstacles();
        updateLift(dt);
        updatePlayer(dt);
        checkCollisions();

        if (state == STATE_PLAYING && quizActive)
            updateQuiz(dt);

        if (state == STATE_PLAYING)
        {
            // reached the graduation portal?
            if (portalActive &&
                distance2D(playerX, playerZ, PORTAL_X, PORTAL_Z) < 1.3f &&
                playerY < 1.0f)
            {
                gradeLevel();
                initConfetti();
                state = STATE_GRADUATED;
            }
            else if (levelTimeLeft <= 0.0f)
            {
                levelTimeLeft = 0.0f;
                quizActive = false;
                failReason = "TIME IS UP!";
                state = STATE_GAME_OVER;
            }
        }
    }
    else if (state == STATE_PUZZLE)
    {
        // timer and obstacles stay paused; only the feedback timer runs
        if (pzPhase == PZ_FEEDBACK)
        {
            pzTimer += dt;
            if (pzTimer >= 2.2f) endPuzzle();
        }
    }
    else if (state == STATE_GRADUATED)
    {
        updateConfetti(dt);
    }
}

// ----------------------------------------------------------------------------
//  DRAWING FUNCTIONS
// ----------------------------------------------------------------------------

// Checkerboard ground made of 1x1 squares
void drawGround()
{
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);                       // the ground faces up
    for (int x = (int)-ARENA; x < (int)ARENA; x++)
    {
        for (int z = (int)-ARENA; z < (int)ARENA; z++)
        {
            if (((x + z) % 2 + 2) % 2 == 0) glColor3f(0.25f, 0.30f, 0.38f);
            else                            glColor3f(0.18f, 0.22f, 0.30f);

            glVertex3f((float)x,     0.0f, (float)z);
            glVertex3f((float)x,     0.0f, (float)(z + 1));
            glVertex3f((float)(x+1), 0.0f, (float)(z + 1));
            glVertex3f((float)(x+1), 0.0f, (float)z);
        }
    }
    glEnd();
}

// Four walls (cubes) + four corner pillars (cylinder + cone)
void drawWalls()
{
    float t = 0.5f;                       // wall thickness
    float L = ARENA * 2.0f + t * 2.0f;    // wall length
    float h = WALL_HEIGHT;

    drawBox(0.0f,  h / 2,  ARENA + t / 2, L, h, t, 0.45f, 0.35f, 0.55f);   // front
    drawBox(0.0f,  h / 2, -ARENA - t / 2, L, h, t, 0.45f, 0.35f, 0.55f);   // back
    drawBox( ARENA + t / 2, h / 2, 0.0f, t, h, L, 0.40f, 0.30f, 0.50f);     // right
    drawBox(-ARENA - t / 2, h / 2, 0.0f, t, h, L, 0.40f, 0.30f, 0.50f);     // left

    // Corner pillars
    float cx[4] = {  ARENA + t / 2, -ARENA - t / 2,  ARENA + t / 2, -ARENA - t / 2 };
    float cz[4] = {  ARENA + t / 2,  ARENA + t / 2, -ARENA - t / 2, -ARENA - t / 2 };
    for (int i = 0; i < 4; i++)
    {
        glPushMatrix();
            glTranslatef(cx[i], 0.0f, cz[i]);

            glColor3f(0.7f, 0.7f, 0.8f);
            drawCylinder(0.5f, 2.2f);                   // pillar

            glColor3f(0.9f, 0.7f, 0.2f);
            glTranslatef(0.0f, 2.2f, 0.0f);
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);        // make the cone point up
            glutSolidCone(0.6, 0.9, 16, 4);             // roof
        glPopMatrix();
    }
}

// The vault in the corner and its locked puzzle gate (a portcullis that slides up)
void drawVault()
{
    if (!gateActive) return;

    float h = WALL_HEIGHT;

    // vault walls
    drawBox(6.5f,   h / 2, -8.125f, 0.5f,  h, 3.75f, 0.35f, 0.35f, 0.65f);   // west wall
    drawBox(6.875f, h / 2, -6.5f,   1.25f, h, 0.5f,  0.35f, 0.35f, 0.65f);   // south wall left
    drawBox(9.75f,  h / 2, -6.5f,   0.5f,  h, 0.5f,  0.35f, 0.35f, 0.65f);   // south wall right

    // the gate (rises when it opens)
    float lift  = gateOpen * 2.3f;
    float pulse = 0.5f + 0.5f * sin(animTime * 5.0f);

    glPushMatrix();
        glTranslatef(0.0f, lift, 0.0f);

        // two gold posts
        glColor3f(0.85f, 0.7f, 0.2f);
        glPushMatrix();
            glTranslatef(7.5f, 0.0f, GATE_Z);
            drawCylinder(0.12f, 2.0f);
        glPopMatrix();
        glPushMatrix();
            glTranslatef(9.5f, 0.0f, GATE_Z);
            drawCylinder(0.12f, 2.0f);
        glPopMatrix();

        // vertical bars
        for (int k = 0; k < 4; k++)
            drawBox(7.8f + k * 0.47f, 1.0f, GATE_Z, 0.08f, 2.0f, 0.08f, 0.7f, 0.7f, 0.8f);

        // horizontal bars
        drawBox(GATE_X, 0.5f, GATE_Z, 2.0f, 0.08f, 0.08f, 0.7f, 0.7f, 0.8f);
        drawBox(GATE_X, 1.6f, GATE_Z, 2.0f, 0.08f, 0.08f, 0.7f, 0.7f, 0.8f);
    glPopMatrix();

    // padlock: glows red while locked
    if (!gateUnlocked)
    {
        setEmission(0.9f * pulse, 0.05f, 0.05f);
        drawBox(GATE_X, 1.05f, GATE_Z + 0.15f, 0.35f, 0.35f, 0.2f, 1.0f, 0.2f, 0.2f);
        setEmission(0.0f, 0.0f, 0.0f);
    }
}

// Tower steps, Sky Deck, lift and the lift console
void drawPlatforms()
{
    for (int i = 0; i < numPlats; i++)
    {
        const Platform& p = plats[i];
        float h  = (p.top < 0.1f) ? 0.1f : p.top;          // a lift on the floor is a thin pad
        float cx = (p.x0 + p.x1) / 2.0f;
        float cz = (p.z0 + p.z1) / 2.0f;
        float sx = p.x1 - p.x0;
        float sz = p.z1 - p.z0;

        float r, g, b;
        if (p.kind == 0)      { r = 0.55f; g = 0.50f; b = 0.45f; }   // stone
        else if (p.kind == 1) { r = 0.35f; g = 0.45f; b = 0.65f; }   // deck
        else                  { r = 0.90f; g = 0.75f; b = 0.25f; }   // lift

        if (p.kind == 2 && liftPowered) setEmission(0.35f, 0.28f, 0.05f);

        drawBox(cx, h / 2.0f, cz, sx, h, sz, r, g, b);                  // body
        float cr = (r * 1.25f > 1.0f) ? 1.0f : r * 1.25f;
        float cg = (g * 1.25f > 1.0f) ? 1.0f : g * 1.25f;
        float cb = (b * 1.25f > 1.0f) ? 1.0f : b * 1.25f;
        drawBox(cx, h + 0.01f, cz, sx + 0.04f, 0.04f, sz + 0.04f, cr, cg, cb);   // lighter top

        setEmission(0.0f, 0.0f, 0.0f);
    }

    // lift console: red screen = locked, green screen = powered
    if (liftActive)
    {
        drawBox(CONSOLE_X, 0.5f, CONSOLE_Z, 0.5f, 1.0f, 0.5f, 0.25f, 0.25f, 0.30f);
        if (liftPowered) setEmission(0.1f, 0.8f, 0.2f);
        else             setEmission(0.4f + 0.4f * sin(animTime * 5.0f), 0.05f, 0.05f);
        drawBox(CONSOLE_X, 1.07f, CONSOLE_Z, 0.42f, 0.14f, 0.42f,
                liftPowered ? 0.2f : 0.9f, liftPowered ? 0.9f : 0.2f, 0.2f);
        setEmission(0.0f, 0.0f, 0.0f);
    }
}

// The four floor plates of the Live Quiz Zone with their numbers
void drawPlates()
{
    if (!levels[currentLevel].hasPlates) return;

    const float pc[4][3] = {
        { 0.85f, 0.25f, 0.25f }, { 0.25f, 0.75f, 0.30f },
        { 0.25f, 0.45f, 0.95f }, { 0.95f, 0.80f, 0.20f }
    };
    int under = quizActive ? plateUnderPlayer() : -1;

    for (int i = 0; i < 4; i++)
    {
        float k = 1.0f;
        if (i == under) { k = 1.3f; setEmission(0.3f, 0.3f, 0.3f); }
        float r = pc[i][0] * k, g = pc[i][1] * k, b = pc[i][2] * k;
        if (r > 1.0f) r = 1.0f;
        if (g > 1.0f) g = 1.0f;
        if (b > 1.0f) b = 1.0f;
        drawBox(PLATE_X[i], 0.04f, PLATE_Z[i], PLATE_HALF * 2.0f, 0.08f, PLATE_HALF * 2.0f, r, g, b);
        setEmission(0.0f, 0.0f, 0.0f);
    }

    // numbers 1-4 written in the 3D world (bitmap text at a 3D position)
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    for (int i = 0; i < 4; i++)
    {
        glRasterPos3f(PLATE_X[i] - 0.08f, 0.12f, PLATE_Z[i] + 0.1f);
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, '1' + i);
    }
    glEnable(GL_LIGHTING);
}

// Soft dark disc under the player: shows where you will land
void drawPlayerShadow()
{
    float gy = groundAt(playerX, playerZ, playerY);     // floor / platform below
    float h  = playerY - gy;
    if (h < 0.0f) h = 0.0f;
    float s  = 0.45f / (1.0f + h * 0.4f);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glColor4f(0.0f, 0.0f, 0.0f, 0.45f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(playerX, gy + 0.07f, playerZ);
    for (int a = 0; a <= 360; a += 30)
        glVertex3f(playerX + s * cos(a * PI / 180.0f), gy + 0.07f, playerZ + s * sin(a * PI / 180.0f));
    glEnd();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// Player: cylinder body, sphere head, cone hat, cone "nose" showing direction
void drawPlayer()
{
    // blink while invulnerable
    if (invuln > 0.0f && ((int)(animTime * 10.0f)) % 2 == 0) return;

    glPushMatrix();
        glTranslatef(playerX, playerY, playerZ);
        glRotatef(playerAngle, 0.0f, 1.0f, 0.0f);       // turn to face the walking direction

        // body (cylinder)
        glColor3f(0.2f, 0.5f, 1.0f);
        glPushMatrix();
            glTranslatef(0.0f, 0.05f, 0.0f);
            drawCylinder(0.35f, 0.75f);
        glPopMatrix();

        // head (sphere)
        glColor3f(1.0f, 0.85f, 0.65f);
        glPushMatrix();
            glTranslatef(0.0f, 1.05f, 0.0f);
            glutSolidSphere(0.3, 20, 20);
        glPopMatrix();

        // hat (cone)
        glColor3f(0.9f, 0.2f, 0.2f);
        glPushMatrix();
            glTranslatef(0.0f, 1.25f, 0.0f);
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            glutSolidCone(0.25, 0.4, 16, 4);
        glPopMatrix();

        // nose (small cone pointing forward = +Z)
        glColor3f(1.0f, 0.6f, 0.1f);
        glPushMatrix();
            glTranslatef(0.0f, 1.05f, 0.25f);
            glutSolidCone(0.07, 0.2, 10, 2);            // a cone points along +Z by default
        glPopMatrix();

        // feet (two small cubes)
        drawBox(-0.15f, 0.06f, 0.05f, 0.18f, 0.12f, 0.3f, 0.1f, 0.1f, 0.1f);
        drawBox( 0.15f, 0.06f, 0.05f, 0.18f, 0.12f, 0.3f, 0.1f, 0.1f, 0.1f);
    glPopMatrix();
}

// Obstacles: a red sphere with 4 spikes (cones). While Time Freeze is active
// they stop spinning and turn ice-blue.
void drawObstacles()
{
    bool frozen = (freezeTimer > 0.0f);

    for (int i = 0; i < NUM_OBSTACLES; i++)
    {
        glPushMatrix();
            glTranslatef(obstacles[i].x, 0.8f, obstacles[i].z);
            glRotatef(obstacleSpin * 120.0f, 0.0f, 1.0f, 0.0f);   // spin

            if (frozen) setEmission(0.1f, 0.25f, 0.4f);

            // core sphere
            if (frozen) glColor3f(0.45f, 0.8f, 1.0f);
            else        glColor3f(0.85f, 0.1f, 0.1f);
            glutSolidSphere(0.55, 20, 20);

            // 4 spikes around the sphere
            if (frozen) glColor3f(0.75f, 0.92f, 1.0f);
            else        glColor3f(1.0f, 0.5f, 0.1f);
            for (int k = 0; k < 4; k++)
            {
                glPushMatrix();
                    glRotatef(k * 90.0f, 0.0f, 1.0f, 0.0f);
                    glTranslatef(0.0f, 0.0f, 0.45f);
                    glutSolidCone(0.18, 0.45, 10, 2);         // points along +Z (outwards)
                glPopMatrix();
            }

            // small cubes on top and bottom
            if (frozen)
            {
                drawBox(0.0f,  0.6f, 0.0f, 0.25f, 0.25f, 0.25f, 0.6f, 0.9f, 1.0f);
                drawBox(0.0f, -0.6f, 0.0f, 0.25f, 0.25f, 0.25f, 0.6f, 0.9f, 1.0f);
            }
            else
            {
                drawBox(0.0f,  0.6f, 0.0f, 0.25f, 0.25f, 0.25f, 0.3f, 0.0f, 0.0f);
                drawBox(0.0f, -0.6f, 0.0f, 0.25f, 0.25f, 0.25f, 0.3f, 0.0f, 0.0f);
            }

            if (frozen) setEmission(0.0f, 0.0f, 0.0f);
        glPopMatrix();
    }
}

// Crystals: spinning, bobbing octahedron with an emissive (glowing) material.
// A solved crystal glows at full brightness, grows, rises and then fades away.
// The mystery crystal is bigger and changes its colour.
void drawCrystals()
{
    for (int i = 0; i < MAX_CRYSTALS; i++)
    {
        if (!crystals[i].active) continue;
        if (crystals[i].collected && crystals[i].burst >= 1.2f) continue;   // finished

        float r, g, b;
        crystalColor(i, r, g, b);

        float pulse = 0.5f + 0.3f * sin(animTime * 4.0f + crystals[i].phase);   // 0.2 .. 0.8
        float bob   = crystals[i].baseY + 0.9f + 0.15f * sin(animTime * 2.0f + crystals[i].phase);
        float s     = crystals[i].hidden ? 1.3f : 1.0f;

        if (crystals[i].collected)               // "solved" animation
        {
            float t = crystals[i].burst / 1.2f;  // 0 .. 1
            pulse = 1.0f;                        // brightest glow
            s     = (1.0f + 2.0f * t) * (1.0f - t);
            bob  += t * 1.5f;
        }

        setEmission(r * pulse, g * pulse, b * pulse);   // the glow
        glColor3f(r, g, b);

        glPushMatrix();
            glTranslatef(crystals[i].x, bob, crystals[i].z);
            glRotatef(animTime * 70.0f + crystals[i].phase * 50.0f, 0.0f, 1.0f, 0.0f);
            glScalef(0.4f * s, 0.75f * s, 0.4f * s);    // make it tall like a crystal
            glutSolidOctahedron();
        glPopMatrix();

        setEmission(0.0f, 0.0f, 0.0f);                  // turn emission off again
    }
}

// The graduation portal (only visible when it is unlocked)
void drawPortal()
{
    if (!portalActive) return;

    float pulse = 0.6f + 0.4f * sin(animTime * 5.0f);

    // golden floor pad
    drawBox(PORTAL_X, 0.05f, PORTAL_Z, 3.0f, 0.1f, 3.0f, 0.9f, 0.8f, 0.3f);

    // glowing ring that always turns to face the camera
    setEmission(0.8f * pulse, 0.6f * pulse, 0.1f);
    glColor3f(1.0f, 0.85f, 0.2f);
    glPushMatrix();
        glTranslatef(PORTAL_X, 1.5f, PORTAL_Z);
        glRotatef(camYaw, 0.0f, 1.0f, 0.0f);
        glutSolidTorus(0.12, 1.3, 12, 32);
    glPopMatrix();
    setEmission(0.0f, 0.0f, 0.0f);
}

// Soft transparent halos around crystals. Must be drawn AFTER everything else.
void drawCrystalHalos()
{
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);                  // additive blending = light-like glow
    glDepthMask(GL_FALSE);                              // do not write to the depth buffer

    for (int i = 0; i < MAX_CRYSTALS; i++)
    {
        if (!crystals[i].active || crystals[i].collected) continue;

        float r, g, b;
        crystalColor(i, r, g, b);

        float pulse = 0.5f + 0.5f * sin(animTime * 4.0f + crystals[i].phase);
        float bob   = crystals[i].baseY + 0.9f + 0.15f * sin(animTime * 2.0f + crystals[i].phase);
        float hs    = crystals[i].hidden ? 1.5f : 1.0f;

        glColor4f(r, g, b, 0.10f + 0.12f * pulse);
        glPushMatrix();
            glTranslatef(crystals[i].x, bob, crystals[i].z);
            glutSolidSphere((0.75f + 0.1f * pulse) * hs, 20, 20);
        glPopMatrix();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// Transparent glowing surface inside the portal ring
void drawPortalGlow()
{
    if (!portalActive) return;

    float pulse = 0.5f + 0.5f * sin(animTime * 4.0f);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    glPushMatrix();
        glTranslatef(PORTAL_X, 1.5f, PORTAL_Z);
        glRotatef(camYaw, 0.0f, 1.0f, 0.0f);
        for (int layer = 0; layer < 2; layer++)
        {
            float rad = (layer == 0) ? 1.25f : 0.8f;
            if (layer == 0) glColor4f(1.0f, 0.8f, 0.2f, 0.12f + 0.10f * pulse);
            else            glColor4f(1.0f, 1.0f, 0.7f, 0.15f + 0.15f * pulse);
            glBegin(GL_TRIANGLE_FAN);
            glVertex3f(0.0f, 0.0f, 0.0f);
            for (int a = 0; a <= 360; a += 15)
                glVertex3f(rad * cos(a * PI / 180.0f), rad * sin(a * PI / 180.0f), 0.0f);
            glEnd();
        }
    glPopMatrix();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// ---------------------------- HUD (2D text) ---------------------------------

// Draws a string at screen position (x,y) in pixels (0,0 = bottom-left)
void drawText(float x, float y, const char* text, void* font)
{
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++)
        glutBitmapCharacter(font, *c);
}

// Draws a string horizontally centred on the screen
void drawCenteredText(float y, const char* text, void* font)
{
    int w = glutBitmapLength(font, (const unsigned char*)text);
    drawText((winW - w) / 2.0f, y, text, font);
}

// Draws a string aligned to the right edge of the screen
void drawRightText(float y, const char* text, void* font)
{
    int w = glutBitmapLength(font, (const unsigned char*)text);
    drawText(winW - w - 20.0f, y, text, font);
}

// Transparent coloured rectangle (2D)
void drawRect(float x, float y, float w, float h,
              float r, float g, float b, float a)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
        glVertex2f(x,     y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
    glEnd();
    glDisable(GL_BLEND);
}

// Outline of a rectangle (2D)
void drawRectOutline(float x, float y, float w, float h, float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x,     y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
    glEnd();
}

// A dark panel with a border, used for all menu screens
void drawPanel(float x, float y, float w, float h)
{
    drawRect(x, y, w, h, 0.03f, 0.03f, 0.12f, 0.90f);
    drawRectOutline(x, y, w, h, 0.5f, 0.6f, 1.0f);
}

// Graduation cap drawn with simple 2D shapes, centred at (cx, cy)
void drawGraduationCap(float cx, float cy)
{
    cy += 6.0f * sin(animTime * 3.0f);                  // gentle bobbing

    // base (the part that sits on the head)
    glColor3f(0.12f, 0.12f, 0.35f);
    glBegin(GL_QUADS);
        glVertex2f(cx - 38, cy - 8);
        glVertex2f(cx + 38, cy - 8);
        glVertex2f(cx + 34, cy - 38);
        glVertex2f(cx - 34, cy - 38);
    glEnd();

    // flat board (diamond)
    glColor3f(0.08f, 0.08f, 0.10f);
    glBegin(GL_QUADS);
        glVertex2f(cx - 85, cy);
        glVertex2f(cx,      cy + 32);
        glVertex2f(cx + 85, cy);
        glVertex2f(cx,      cy - 28);
    glEnd();

    // tassel
    glColor3f(1.0f, 0.8f, 0.1f);
    glLineWidth(3.0f);
    glBegin(GL_LINE_STRIP);
        glVertex2f(cx,      cy + 2);
        glVertex2f(cx + 60, cy - 4);
        glVertex2f(cx + 60, cy - 36);
    glEnd();
    glLineWidth(1.0f);
    glBegin(GL_QUADS);
        glVertex2f(cx + 54, cy - 36);
        glVertex2f(cx + 66, cy - 36);
        glVertex2f(cx + 68, cy - 54);
        glVertex2f(cx + 52, cy - 54);
    glEnd();
}

void drawConfetti()
{
    glBegin(GL_QUADS);
    for (int i = 0; i < MAX_CONFETTI; i++)
    {
        glColor3f(confetti[i].r, confetti[i].g, confetti[i].b);
        float s = confetti[i].size;
        glVertex2f(confetti[i].x,     confetti[i].y);
        glVertex2f(confetti[i].x + s, confetti[i].y);
        glVertex2f(confetti[i].x + s, confetti[i].y + s * 0.6f);
        glVertex2f(confetti[i].x,     confetti[i].y + s * 0.6f);
    }
    glEnd();
}

// The Live Quiz banner at the top of the screen (the game keeps running!)
void drawQuizBanner()
{
    const Question& q = questionBank[puzzleQ];
    char buf[200];

    float bx = 20.0f, bw = winW - 40.0f, bh = 140.0f, by = winH - 300.0f;
    drawRect(bx, by, bw, bh, 0.02f, 0.02f, 0.10f, 0.85f);
    drawRectOutline(bx, by, bw, bh, 1.0f, 0.8f, 0.3f);

    int secs = (int)ceil(quizTime);
    if (secs < 0) secs = 0;
    glColor3f(1.0f, 0.85f, 0.3f);
    sprintf(buf, "LIVE QUIZ   [%s - %s]   Time: %d s   Reward: +%d",
            catNames[q.category], diffNames[q.difficulty], secs, pzReward);
    drawText(bx + 15, by + bh - 28, buf, GLUT_BITMAP_HELVETICA_18);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(bx + 15, by + bh - 58, q.text, GLUT_BITMAP_HELVETICA_18);

    glColor3f(0.8f, 0.9f, 1.0f);
    sprintf(buf, "Plate 1: %s     Plate 2: %s     Plate 3: %s     Plate 4: %s",
            q.opt[optOrder[0]], q.opt[optOrder[1]], q.opt[optOrder[2]], q.opt[optOrder[3]]);
    drawText(bx + 15, by + bh - 88, buf, GLUT_BITMAP_HELVETICA_18);

    if (pzHintUsed)
    {
        glColor3f(1.0f, 0.9f, 0.4f);
        sprintf(buf, "HINT: %s", q.hint);
        drawText(bx + 15, by + 12, buf, GLUT_BITMAP_HELVETICA_12);
    }
    else
    {
        glColor3f(0.7f, 0.8f, 1.0f);
        drawText(bx + 15, by + 12,
                 "Walk onto a plate and stay 1.5 s to lock in.   H: hint   F: cancel",
                 GLUT_BITMAP_HELVETICA_12);
    }

    // lock-in progress bar
    if (quizPlate >= 0)
    {
        float f = quizHold / QUIZ_HOLD;
        if (f > 1.0f) f = 1.0f;
        sprintf(buf, "Locking in plate %d ...", quizPlate + 1);
        glColor3f(0.4f, 1.0f, 0.5f);
        drawText(bx + bw - 330, by + 12, buf, GLUT_BITMAP_HELVETICA_12);
        drawRect(bx + bw - 190, by + 10, 170.0f, 12.0f, 0.2f, 0.2f, 0.2f, 0.9f);
        drawRect(bx + bw - 190, by + 10, 170.0f * f, 12.0f, 0.3f, 1.0f, 0.4f, 0.95f);
    }
}

// The question panel (state STATE_PUZZLE) - used for crystals, the gate,
// the lift console, the final exam and the recovery challenge
void drawPuzzlePanel()
{
    const Question& q = questionBank[puzzleQ];
    char buf[200];

    // dim the 3D scene behind the panel
    drawRect(0, 0, (float)winW, (float)winH, 0.0f, 0.0f, 0.0f, 0.45f);

    float pw = winW - 120.0f, ph = 400.0f;
    float px = 60.0f, py = winH / 2.0f - ph / 2.0f;
    float top = py + ph;
    drawPanel(px, py, pw, ph);

    // header (depends on what the panel is for)
    if (puzzleType == TG_CRYSTAL)
    {
        float r, g, b;
        crystalColor(puzzleCrystal, r, g, b);
        glColor3f(r, g, b);
        if (crystals[puzzleCrystal].hidden)
            sprintf(buf, "MYSTERY CRYSTAL   [%s - %s]", catNames[q.category], diffNames[q.difficulty]);
        else if (crystals[puzzleCrystal].quiz)
            sprintf(buf, "LIVE QUIZ RESULT   [%s - %s]", catNames[q.category], diffNames[q.difficulty]);
        else
            sprintf(buf, "CRYSTAL CHALLENGE   [%s - %s]", catNames[q.category], diffNames[q.difficulty]);
    }
    else if (puzzleType == TG_GATE)
    {
        glColor3f(0.6f, 0.8f, 1.0f);
        sprintf(buf, "PUZZLE GATE   [%s - %s]", catNames[q.category], diffNames[q.difficulty]);
    }
    else if (puzzleType == TG_LIFT)
    {
        glColor3f(1.0f, 0.85f, 0.3f);
        sprintf(buf, "LIFT CONSOLE   [%s - %s]", catNames[q.category], diffNames[q.difficulty]);
    }
    else if (puzzleType == TG_EXAM)
    {
        glColor3f(1.0f, 0.8f, 0.3f);
        sprintf(buf, "FINAL EXAM   Question %d of %d   (pass: %d correct)",
                examIndex + 1, EXAM_QUESTIONS, EXAM_PASS);
    }
    else
    {
        glColor3f(0.4f, 1.0f, 0.6f);
        sprintf(buf, "ENERGY RECOVERY   (%d of %d solved)", recoveryDone, RECOVERY_NEEDED);
    }
    drawText(px + 30, top - 40, buf, GLUT_BITMAP_HELVETICA_18);

    if (pzReward > 0)
    {
        glColor3f(1.0f, 0.9f, 0.3f);
        sprintf(buf, "Reward: +%d", pzReward);
        drawText(px + pw - 130, top - 40, buf, GLUT_BITMAP_HELVETICA_18);
    }

    // extra note line
    glColor3f(1.0f, 0.85f, 0.4f);
    if (puzzleType == TG_RECOVERY)
        drawText(px + 30, top - 62, "Your Knowledge Energy hit zero! Answer correctly to recover.",
                 GLUT_BITMAP_HELVETICA_12);
    else if (puzzleType == TG_EXAM && examIndex == 0 && examNote[0] != '\0')
        drawText(px + 30, top - 62, examNote, GLUT_BITMAP_HELVETICA_12);

    // the question
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(px + 30, top - 100, q.text, GLUT_BITMAP_TIMES_ROMAN_24);

    // the four options
    for (int i = 0; i < 4; i++)
    {
        if (pzPhase == PZ_FEEDBACK && i == pzSelected)
        {
            if (pzCorrect) glColor3f(0.2f, 1.0f, 0.3f);
            else           glColor3f(1.0f, 0.3f, 0.3f);
        }
        else glColor3f(0.9f, 0.9f, 1.0f);

        sprintf(buf, "%d.   %s", i + 1, q.opt[optOrder[i]]);
        drawText(px + 60, top - 155 - i * 38.0f, buf, GLUT_BITMAP_HELVETICA_18);
    }

    // hint
    if (pzHintUsed)
    {
        glColor3f(1.0f, 0.9f, 0.4f);
        sprintf(buf, "HINT: %s", q.hint);
        drawText(px + 30, py + 90, buf, GLUT_BITMAP_HELVETICA_18);
    }

    // time freeze reward text
    if (pzPhase == PZ_FEEDBACK && pzCorrect && puzzleType != TG_RECOVERY && q.difficulty == 3)
    {
        glColor3f(0.5f, 0.9f, 1.0f);
        drawText(px + 30, py + 62, "Hard question solved: TIME FREEZE reward earned!",
                 GLUT_BITMAP_HELVETICA_18);
    }

    // bottom line: instructions or feedback
    if (pzPhase == PZ_ASKING)
    {
        glColor3f(0.7f, 0.8f, 1.0f);
        if (puzzleType == TG_RECOVERY)
            drawText(px + 30, py + 30, "Keys 1-4: choose answer", GLUT_BITMAP_HELVETICA_12);
        else
            drawText(px + 30, py + 30,
                     "Keys 1-4: choose answer     H: hint (-5 reward, -5 energy)",
                     GLUT_BITMAP_HELVETICA_12);
    }
    else if (pzCorrect)
    {
        glColor3f(0.2f, 1.0f, 0.3f);
        if (puzzleType == TG_RECOVERY)
            drawText(px + 30, py + 30, "CORRECT!", GLUT_BITMAP_TIMES_ROMAN_24);
        else if (puzzleType == TG_GATE)
        {
            sprintf(buf, "CORRECT!   +%d POINTS   THE GATE IS OPENING!", pzReward);
            drawText(px + 30, py + 30, buf, GLUT_BITMAP_TIMES_ROMAN_24);
        }
        else if (puzzleType == TG_LIFT)
        {
            sprintf(buf, "CORRECT!   +%d POINTS   THE LIFT IS POWERED!", pzReward);
            drawText(px + 30, py + 30, buf, GLUT_BITMAP_TIMES_ROMAN_24);
        }
        else
        {
            sprintf(buf, "CORRECT!   +%d POINTS   (+10 energy)", pzReward);
            drawText(px + 30, py + 30, buf, GLUT_BITMAP_TIMES_ROMAN_24);
        }
    }
    else
    {
        glColor3f(1.0f, 0.3f, 0.3f);
        if (puzzleType == TG_RECOVERY)
            drawText(px + 30, py + 30, "WRONG!  Try the next question.", GLUT_BITMAP_TIMES_ROMAN_24);
        else if (puzzleType == TG_EXAM)
            drawText(px + 30, py + 30, "WRONG!  (-10 energy)", GLUT_BITMAP_TIMES_ROMAN_24);
        else if (pzSelected < 0)
            drawText(px + 30, py + 30, "TIME'S UP!  No plate chosen.   (-10 energy)", GLUT_BITMAP_TIMES_ROMAN_24);
        else
            drawText(px + 30, py + 30,
                     "WRONG!  It stays locked - try again.   (-10 energy)",
                     GLUT_BITMAP_TIMES_ROMAN_24);
    }
}

void drawHUD()
{
    // Switch to a 2D orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    char buf[200];
    const LevelConfig& L = levels[currentLevel];

    // ---------------- always visible: left side ----------------
    glColor3f(1.0f, 1.0f, 0.6f);
    drawText(20, winH - 28, L.name, GLUT_BITMAP_HELVETICA_18);

    glColor3f(1.0f, 1.0f, 1.0f);
    sprintf(buf, "SCORE: %d", score);
    drawText(20, winH - 52, buf, GLUT_BITMAP_HELVETICA_18);

    sprintf(buf, "CRYSTALS: %d / %d", collectedCount, L.crystals);
    drawText(20, winH - 76, buf, GLUT_BITMAP_HELVETICA_18);

    int answered = totalCorrect + totalWrong;
    int accuracy = (answered > 0) ? (int)(100.0f * totalCorrect / answered + 0.5f) : 0;
    sprintf(buf, "Correct: %d   Wrong: %d   Accuracy: %d%%", totalCorrect, totalWrong, accuracy);
    drawText(20, winH - 100, buf, GLUT_BITMAP_HELVETICA_12);

    // dynamic difficulty indicator
    if (diffTrend > 0)      { glColor3f(1.0f, 0.6f, 0.3f); sprintf(buf, "Difficulty: RISING (streak!)"); }
    else if (diffTrend < 0) { glColor3f(0.6f, 0.9f, 1.0f); sprintf(buf, "Difficulty: EASING"); }
    else                    { glColor3f(0.8f, 0.8f, 0.8f); sprintf(buf, "Difficulty: NORMAL"); }
    drawText(20, winH - 118, buf, GLUT_BITMAP_HELVETICA_12);

    if (gateActive)
    {
        if (gateUnlocked) { glColor3f(0.4f, 1.0f, 0.5f); sprintf(buf, "Puzzle gate: OPEN"); }
        else              { glColor3f(1.0f, 0.4f, 0.4f); sprintf(buf, "Puzzle gate: LOCKED"); }
        drawText(20, winH - 136, buf, GLUT_BITMAP_HELVETICA_12);
    }
    if (liftActive)
    {
        if (liftPowered) { glColor3f(0.4f, 1.0f, 0.5f); sprintf(buf, "Lift: POWERED"); }
        else             { glColor3f(1.0f, 0.4f, 0.4f); sprintf(buf, "Lift: LOCKED (use the console)"); }
        drawText(20, winH - 136, buf, GLUT_BITMAP_HELVETICA_12);
    }

    // height meter (3D position)
    glColor3f(0.7f, 0.9f, 1.0f);
    sprintf(buf, "Height: %.1f", playerY);
    drawText(20, winH - 154, buf, GLUT_BITMAP_HELVETICA_12);

    // ---------------- always visible: right side ----------------
    int secs = (int)ceil(levelTimeLeft);
    if (secs < 0) secs = 0;
    if (levelTimeLeft < 15.0f && ((int)(animTime * 4.0f)) % 2 == 0) glColor3f(1.0f, 0.2f, 0.2f);
    else                                                              glColor3f(1.0f, 1.0f, 1.0f);
    sprintf(buf, "TIME: %d:%02d", secs / 60, secs % 60);
    drawRightText(winH - 28, buf, GLUT_BITMAP_HELVETICA_18);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawRightText(winH - 52, "KNOWLEDGE ENERGY", GLUT_BITMAP_HELVETICA_12);
    {
        float bx = winW - 220.0f, by = winH - 74.0f, bw = 200.0f, bh = 14.0f;
        float f  = energy / MAX_ENERGY;
        drawRect(bx, by, bw, bh, 0.2f, 0.2f, 0.2f, 0.8f);
        if (f > 0.6f)      drawRect(bx, by, bw * f, bh, 0.2f, 0.9f, 0.3f, 0.95f);
        else if (f > 0.3f) drawRect(bx, by, bw * f, bh, 1.0f, 0.8f, 0.1f, 0.95f);
        else               drawRect(bx, by, bw * f, bh, 1.0f, 0.2f, 0.2f, 0.95f);
        drawRectOutline(bx, by, bw, bh, 1.0f, 1.0f, 1.0f);
    }

    glColor3f(0.8f, 1.0f, 0.8f);
    sprintf(buf, "CGPA: %.2f    LAST GRADE: %s", cgpa, lastGrade);
    drawRightText(winH - 100, buf, GLUT_BITMAP_HELVETICA_18);

    // Help line at the bottom
    glColor3f(0.8f, 0.8f, 0.8f);
    drawText(20, 15, "WASD: move   SPACE: jump   Arrows: camera   F: use / solve   R: restart   ESC: quit",
             GLUT_BITMAP_HELVETICA_12);

    // ---------------- state dependent overlays ----------------
    if (state == STATE_PLAYING)
    {
        if (quizActive)
        {
            drawQuizBanner();
        }
        else
        {
            int nc = findNearbyCrystal();
            if (nc >= 0)
            {
                glColor3f(1.0f, 1.0f, 0.3f);
                if (crystals[nc].hidden)
                    sprintf(buf, "Press F to Solve Challenge   [MYSTERY - Hard - +50 bonus]");
                else if (crystals[nc].quiz)
                    sprintf(buf, "Press F to start the LIVE QUIZ   [%s - %s]",
                            catNames[crystals[nc].category], diffNames[crystals[nc].difficulty]);
                else
                    sprintf(buf, "Press F to Solve Challenge   [%s - %s]",
                            catNames[crystals[nc].category], diffNames[crystals[nc].difficulty]);
                drawCenteredText(110, buf, GLUT_BITMAP_HELVETICA_18);
            }
            else if (findNearbyGate())
            {
                glColor3f(0.6f, 0.9f, 1.0f);
                drawCenteredText(110, "Press F to Solve the Gate Puzzle   [Logic - Hard]",
                                 GLUT_BITMAP_HELVETICA_18);
            }
            else if (findNearbyConsole())
            {
                glColor3f(1.0f, 0.85f, 0.3f);
                drawCenteredText(110, "Press F to Power the Lift   [Hard]", GLUT_BITMAP_HELVETICA_18);
            }
        }

        if (portalActive)
        {
            glColor3f(1.0f, 0.9f, 0.3f);
            drawCenteredText(winH - 190.0f,
                             "Exam passed!  Walk to the glowing GRADUATION PORTAL!",
                             GLUT_BITMAP_HELVETICA_18);
        }
        if (freezeTimer > 0.0f)
        {
            glColor3f(0.5f, 0.9f, 1.0f);
            sprintf(buf, "TIME FREEZE ACTIVATED   (%.1f s)", freezeTimer);
            drawCenteredText(winH * 0.80f, buf, GLUT_BITMAP_TIMES_ROMAN_24);
        }
        if (flashTimer > 0.0f)
        {
            glColor3f(flashR, flashG, flashB);
            drawCenteredText(winH * 0.45f, flashMsg, GLUT_BITMAP_TIMES_ROMAN_24);
        }
    }
    else if (state == STATE_PUZZLE)
    {
        drawPuzzlePanel();
    }
    else if (state == STATE_LEVEL_INTRO)
    {
        float pw = 680.0f, ph = 320.0f;
        float px = (winW - pw) / 2.0f, py = (winH - ph) / 2.0f;
        drawPanel(px, py, pw, ph);

        glColor3f(1.0f, 0.9f, 0.3f);
        drawCenteredText(py + ph - 50, L.name, GLUT_BITMAP_TIMES_ROMAN_24);

        glColor3f(1.0f, 1.0f, 1.0f);
        sprintf(buf, "Topic: %s", L.topic);
        drawCenteredText(py + ph - 90, buf, GLUT_BITMAP_HELVETICA_18);
        sprintf(buf, "Solve %d puzzle crystals   |   Time limit: %d seconds",
                L.crystals, (int)L.timeLimit);
        drawCenteredText(py + ph - 120, buf, GLUT_BITMAP_HELVETICA_18);
        sprintf(buf, "Obstacle speed: x%.2f   |   Credits: %d", L.obstacleSpeed, L.credits);
        drawCenteredText(py + ph - 150, buf, GLUT_BITMAP_HELVETICA_18);

        glColor3f(1.0f, 0.8f, 0.4f);
        if (L.special  != NULL) drawCenteredText(py + ph - 185, L.special,  GLUT_BITMAP_HELVETICA_18);
        if (L.special2 != NULL) drawCenteredText(py + ph - 212, L.special2, GLUT_BITMAP_HELVETICA_18);

        glColor3f(0.4f, 1.0f, 0.5f);
        drawCenteredText(py + 30, "Press ENTER to begin", GLUT_BITMAP_HELVETICA_18);
    }
    else if (state == STATE_LEVEL_COMPLETE)
    {
        float pw = 560.0f, ph = 300.0f;
        float px = (winW - pw) / 2.0f, py = (winH - ph) / 2.0f;
        drawPanel(px, py, pw, ph);

        glColor3f(0.3f, 1.0f, 0.4f);
        drawCenteredText(py + ph - 50, "LEVEL COMPLETE!", GLUT_BITMAP_TIMES_ROMAN_24);

        glColor3f(1.0f, 1.0f, 1.0f);
        sprintf(buf, "Level grade: %s   (grade point %.1f)", lastGrade, lastLevelGP);
        drawCenteredText(py + ph - 95, buf, GLUT_BITMAP_HELVETICA_18);
        sprintf(buf, "Accuracy: %d%%     Time left: %d s",
                (int)(lastLevelAcc + 0.5f), (int)levelTimeLeft);
        drawCenteredText(py + ph - 125, buf, GLUT_BITMAP_HELVETICA_18);
        sprintf(buf, "Performance: %d / 100", (int)(lastLevelPerf + 0.5f));
        drawCenteredText(py + ph - 155, buf, GLUT_BITMAP_HELVETICA_18);
        sprintf(buf, "CGPA so far: %.2f     Score: %d", cgpa, score);
        drawCenteredText(py + ph - 185, buf, GLUT_BITMAP_HELVETICA_18);

        glColor3f(0.4f, 1.0f, 0.5f);
        drawCenteredText(py + 30, "Press ENTER for the next level", GLUT_BITMAP_HELVETICA_18);
    }
    else if (state == STATE_GAME_OVER)
    {
        float pw = 560.0f, ph = 220.0f;
        float px = (winW - pw) / 2.0f, py = (winH - ph) / 2.0f;
        drawPanel(px, py, pw, ph);

        glColor3f(1.0f, 0.3f, 0.3f);
        drawCenteredText(py + ph - 55, "LEVEL FAILED", GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(1.0f, 1.0f, 1.0f);
        drawCenteredText(py + ph - 95, failReason, GLUT_BITMAP_HELVETICA_18);
        drawCenteredText(py + 75, "ENTER: retry this level", GLUT_BITMAP_HELVETICA_18);
        drawCenteredText(py + 45, "R: restart the whole game", GLUT_BITMAP_HELVETICA_18);
    }
    else if (state == STATE_GRADUATED)
    {
        float pw = 640.0f, ph = 420.0f;
        float px = (winW - pw) / 2.0f, py = (winH - ph) / 2.0f;
        drawRect(0, 0, (float)winW, (float)winH, 0.0f, 0.0f, 0.0f, 0.5f);
        drawPanel(px, py, pw, ph);

        drawGraduationCap(winW / 2.0f, py + ph - 55.0f);

        glColor3f(1.0f, 0.9f, 0.3f);
        drawCenteredText(py + ph - 130, "CONGRATULATIONS! YOU HAVE GRADUATED!",
                         GLUT_BITMAP_TIMES_ROMAN_24);

        glColor3f(1.0f, 1.0f, 1.0f);
        sprintf(buf, "Total Score: %d", score);
        drawCenteredText(py + ph - 185, buf, GLUT_BITMAP_HELVETICA_18);
        sprintf(buf, "Final CGPA: %.2f", cgpa);
        drawCenteredText(py + ph - 215, buf, GLUT_BITMAP_HELVETICA_18);
        sprintf(buf, "Final Grade: %s", letterFromGP(cgpa));
        drawCenteredText(py + ph - 245, buf, GLUT_BITMAP_HELVETICA_18);
        sprintf(buf, "Accuracy: %d%%   (%d correct, %d wrong)", accuracy, totalCorrect, totalWrong);
        drawCenteredText(py + ph - 275, buf, GLUT_BITMAP_HELVETICA_18);

        glColor3f(0.4f, 1.0f, 0.5f);
        drawCenteredText(py + 30, "Press R to play again", GLUT_BITMAP_HELVETICA_18);

        drawConfetti();
    }

    // Restore the 3D state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

// ----------------------------------------------------------------------------
//  GLUT CALLBACK FUNCTIONS
// ----------------------------------------------------------------------------

// Draws one complete frame
void display()
{
    glClearColor(0.04f, 0.04f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // ---- Camera: orbit around the player (follows the player's height) ----
    float yaw   = camYaw   * PI / 180.0f;
    float pitch = camPitch * PI / 180.0f;
    float tx = playerX, ty = playerY + 0.8f, tz = playerZ;       // look-at target
    float ex = tx + camDistance * cos(pitch) * sin(yaw);         // eye position
    float ey = ty + camDistance * sin(pitch);
    float ez = tz + camDistance * cos(pitch) * cos(yaw);
    gluLookAt(ex, ey, ez,   tx, ty, tz,   0.0, 1.0, 0.0);

    // ---- Light (set AFTER the camera so it stays fixed in the world) ----
    GLfloat lightPos[4] = { 0.0f, 15.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // ---- Scene ----
    drawGround();
    drawWalls();
    drawVault();
    drawPlates();
    drawPlatforms();
    drawObstacles();
    drawPlayerShadow();
    drawPlayer();
    drawCrystals();
    drawPortal();
    drawCrystalHalos();     // transparent things last
    drawPortalGlow();

    // ---- 2D text on top ----
    drawHUD();

    glutSwapBuffers();
}

// Called when the window is resized
void reshape(int w, int h)
{
    if (h == 0) h = 1;
    winW = w;
    winH = h;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55.0, (double)w / (double)h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// Timer: calls update() and redraws about every 16 ms (~60 FPS)
void timer(int value)
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTimeMs) / 1000.0f;
    lastTimeMs = now;
    if (dt > 0.05f) dt = 0.05f;          // avoid huge jumps if the window freezes

    update(dt);
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// Normal keys pressed. What a key does depends on the game state, so puzzle
// keys (1-4, H, F) can never move the player by accident.
void keyboardDown(unsigned char key, int x, int y)
{
    key = (unsigned char)tolower(key);   // so 'W' and 'w' are the same

    if (key == 27) exit(0);              // ESC
    if (key == 'r') { resetGame(); return; }

    bool enter = (key == 13 || key == ' ');

    switch (state)
    {
    case STATE_LEVEL_INTRO:
        if (enter) state = STATE_PLAYING;
        break;

    case STATE_PLAYING:
        keyDown[key] = true;             // movement keys are only stored while playing
        if (key == ' ') jumpRequested = true;

        if (key == 'f')
        {
            if (quizActive)              // F again cancels a running quiz
            {
                quizActive = false;
                setFlash("Quiz cancelled.", 0.8f, 0.8f, 0.8f);
            }
            else
            {
                int c = findNearbyCrystal();
                if (c >= 0)
                {
                    if (crystals[c].quiz) startQuiz(c);
                    else                  openPuzzle(TG_CRYSTAL, c);
                }
                else if (findNearbyGate())    openPuzzle(TG_GATE, -1);
                else if (findNearbyConsole()) openPuzzle(TG_LIFT, -1);
            }
        }
        if (key == 'h' && quizActive) useHint();
        break;

    case STATE_PUZZLE:
        if (pzPhase == PZ_ASKING)
        {
            if (key >= '1' && key <= '4') submitAnswer(key - '1');
            else if (key == 'h')          useHint();
        }
        else if (enter && pzTimer > 0.5f)
        {
            endPuzzle();                 // skip the feedback screen
        }
        break;

    case STATE_LEVEL_COMPLETE:
        if (enter) startLevel(currentLevel + 1);
        break;

    case STATE_GAME_OVER:
        if (enter) retryLevel();
        break;

    case STATE_GRADUATED:
        break;                           // press R to restart
    }
}

// Normal keys released
void keyboardUp(unsigned char key, int x, int y)
{
    key = (unsigned char)tolower(key);
    keyDown[key] = false;
}

// Arrow keys pressed
void specialKeyDown(int key, int x, int y)
{
    if (key >= 0 && key < 256) specialDown[key] = true;
}

// Arrow keys released
void specialKeyUp(int key, int x, int y)
{
    if (key >= 0 && key < 256) specialDown[key] = false;
}

// One-time OpenGL setup
void initGL()
{
    glEnable(GL_DEPTH_TEST);             // hide things that are behind others
    glEnable(GL_NORMALIZE);              // keep normals correct when scaling objects
    glShadeModel(GL_SMOOTH);

    // --- Lighting ---
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat ambient[4]  = { 0.30f, 0.30f, 0.35f, 1.0f };
    GLfloat diffuse[4]  = { 0.90f, 0.90f, 0.85f, 1.0f };
    GLfloat specular[4] = { 1.00f, 1.00f, 1.00f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    // --- Materials: glColor3f() sets the material colour ---
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat matSpec[4] = { 0.6f, 0.6f, 0.6f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 40.0f);

    // Quadric object for cylinders
    quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
}

// ----------------------------------------------------------------------------
//  MAIN
// ----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    srand((unsigned int)time(NULL));     // different crystal positions every run

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutInitWindowPosition(100, 60);
    glutCreateWindow("CRYSTAL QUEST : The Graduation Mystery");

    initGL();
    resetGame();

    // Register the callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);
    glutIgnoreKeyRepeat(1);              // we track key up/down ourselves

    lastTimeMs = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
