/*
 * CAMPUS QUEST 3D: Journey to Graduation
 * ---------------------------------------------------------------
 * A complete 3D university adventure game.
 * Language : C++11        Graphics : legacy OpenGL + GLU + GLUT
 * Target   : Code::Blocks, 32-bit MinGW GCC 5.1.0 on Windows
 * Assets   : none (everything is drawn procedurally)
 *
 * See README_SETUP.md for build instructions and controls.
 */
// NOTE: the standard headers come BEFORE <GL/glut.h>. Old GLUT 3.7 headers
// redefine exit(), which causes errors if <cstdlib> is included afterwards.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#ifdef _WIN32
#define NOMINMAX
#define GLUT_DISABLE_ATEXIT_HACK
#include <windows.h>
#endif
#include <GL/glut.h>

#define PI_F 3.14159265f
static const float DEG2RAD = PI_F / 180.0f;

// ======================================================================
// SECTION 1 - TYPES
// ======================================================================
enum GameState {
    MAIN_MENU, EXPLORING, DIALOGUE, PUZZLE, QUESTION_FEEDBACK,
    LEVEL_COMPLETE, LEVEL_FAILED, PAUSED, GRADUATION, GAME_COMPLETE
};

enum Category { CAT_MATH = 0, CAT_LOGIC = 1, CAT_CODE = 2, CAT_ANY = 3 };

// Building ids (index into g.buildings)
enum BuildingId {
    B_ACADEMIC = 0, B_MATH, B_LIBRARY, B_CSE, B_EXAM, B_CAFE, B_PROJECT, B_AUDITORIUM, B_COUNT
};

struct Question {
    int cat;              // Category
    int diff;             // 1 easy, 2 medium, 3 hard  (score value = diff * 10)
    std::string text;     // '|' separates display lines
    std::string flash;    // memory challenges: shown first for a few seconds
    std::string opt[4];
    int correct;          // 0..3
    std::string hint;
};

struct Player {
    float x, z, y;        // world position (y = height above ground)
    float vy;             // vertical speed
    float heading;        // degrees, direction the character faces
    float walkPhase;      // animation phase
    bool  moving;
    bool  onGround;
};

struct Camera {
    float yaw;            // degrees around the player
    float pitch;          // degrees above the horizon
    float dist;           // distance behind the player
    float cx, cy, cz;     // smoothed camera position
    bool  initialised;
};

struct Building {
    const char* name;
    float x, z;           // centre
    float w, d, h;        // width (x), depth (z), height
    float r, g, b;        // wall colour
    float doorX, doorZ;   // interaction point in front of the door
    int   unlockLevel;    // minimum "current level" needed to enter
};

struct NPC {
    const char* name;
    float x, z;
    float r, g, b;        // clothes colour
    int   building;       // building this teacher belongs to
    float facing;
};

struct AcademicProgress {
    int   totalScore;
    int   semester;
    int   completedMissions;
    int   creditsEarned;
    float cgpa;
    std::string currentGrade;
    // per module (level 1..5, stored 0..4)
    bool  attempted[5];
    bool  passed[5];
    float gradePoint[5];
    std::string grade[5];
    int   credits[5];
    int   bestPercent[5];
    int   bestScore[5];
    bool  achievement[5];
};

struct PuzzleManager {
    std::vector<Question> set;   // questions of the running challenge
    int   index;
    int   challengeScore;        // can't go below zero
    int   maxScore;
    int   correctCount;
    float qTimer;                // seconds since the current question appeared
    bool  hintUsed;
    bool  hintShown;
    int   lastPicked;            // answer chosen at the last question
    bool  lastCorrect;
    float feedbackTimer;
    int   pointsGained;          // for the floating +10 animation
    int   timeLimit;
};

struct GameManager {
    GameState state, prevState;
    int   currentLevel;          // 1..6 (6 = all levels finished)
    int   activeLevel;           // level being played
    float timeLeft;
    float clock;                 // global animation time
    float notifyTimer; std::string notifyText;
    float unlockTimer;  std::string unlockText;
    float coffeeBonus;           // seconds added to the next level
    bool  showMap;
    bool  gradReachedStage;
    bool  achievementPopup; int achNew;
    std::string dialogueName, dialogueText;
    int   dialogueAfter;         // 0 = back to exploring, 1 = start puzzle
    int   menuSel;
    std::string passedMsg;
    int   lastFailReason;        // 0 low score, 1 time up
    int   lastFailLevel;         // level index (0..4) of the last failure, -1 none
    float certTimer;
    bool  hasSave;
    int   winW, winH;
    bool  keys[256];
    bool  special[256];
    std::vector<Building> buildings;
    std::vector<NPC> npcs;
};

struct Confetti { float x, y, z, vx, vy, vz, r, g, b, spin; };

// ======================================================================
// SECTION 2 - GLOBALS
// ======================================================================
static Player           player;
static Camera           cam;
static PuzzleManager    puzzle;
static AcademicProgress acad;
static GameManager      g;
static std::vector<Confetti> confetti;

static const float WORLD_HALF = 52.0f;
static const float PLAYER_RADIUS = 0.6f;
static const char* ACH_NAMES[5] = {
    "Math Master", "Logic Explorer", "Coding Champion", "Exam Survivor", "Future Graduate"
};
static const int   LEVEL_QUESTIONS[5] = { 3, 4, 5, 6, 7 };
static const int   LEVEL_TIME[5]      = { 120, 110, 100, 90, 120 };
static const int   LEVEL_CREDITS[5]   = { 3, 3, 3, 4, 5 };
static const char* LEVEL_NAMES[5] = {
    "Freshman Orientation", "Library Mystery", "Programming Challenge",
    "Final Examination", "Final Year Project"
};
// which building hosts each level
static const int LEVEL_BUILDING[5] = { B_MATH, B_LIBRARY, B_CSE, B_EXAM, B_PROJECT };

// ======================================================================
// SECTION 3 - QUESTION BANK
// Fields: category, difficulty, text, memory-flash, {A,B,C,D}, correct(0-3), hint
// ======================================================================
static std::vector<Question> bank;

static void buildQuestionBank()
{
    bank.clear();
    const Question q[] = {
    // ------------------------------ MATHEMATICS ------------------------------
    {0,1,"What is 15 + 27 ?","",{"32","42","52","41"},1,"Add the tens first: 10+20, then the ones: 5+7."},
    {0,1,"What is 9 x 7 ?","",{"56","63","72","67"},1,"9 x 7 = 9 x 10 minus 9 x 3."},
    {0,1,"What is 100 - 38 ?","",{"52","72","62","68"},2,"Subtract 40, then add 2 back."},
    {0,1,"What is 48 / 6 ?","",{"6","7","8","9"},2,"How many times does 6 fit in 48? 6 x 8 = 48."},
    {0,1,"What comes next?|2, 4, 6, 8, ?","",{"9","10","12","11"},1,"The numbers go up by 2 each time."},
    {0,1,"Solve for x:|x + 9 = 21","",{"11","12","13","30"},1,"Subtract 9 from both sides."},
    {0,1,"What is 25% of 80 ?","",{"15","20","25","40"},1,"25% is one quarter. 80 / 4 = ?"},
    {0,2,"Solve for x:|3x - 5 = 16","",{"5","6","7","8"},2,"Add 5 to both sides, then divide by 3."},
    {0,2,"What comes next?|3, 6, 12, 24, ?","",{"36","42","48","30"},2,"Each number is doubled."},
    {0,2,"What is 12 x 12 - 11 x 11 ?","",{"1","23","22","24"},1,"144 - 121. Or use (a-b)(a+b)."},
    {0,2,"Solve for x:|2(x + 3) = 18","",{"6","9","12","3"},0,"Divide by 2 first: x + 3 = 9."},
    {0,2,"What comes next?|1, 4, 9, 16, ?","",{"20","24","25","36"},2,"These are square numbers: 1x1, 2x2, 3x3 ..."},
    {0,2,"What is 15% of 200 ?","",{"20","25","30","35"},2,"10% is 20, 5% is 10."},
    {0,3,"What is the SUM of the roots of|x^2 - 5x + 6 = 0 ?","",{"5","6","-5","1"},0,"Factor: (x-2)(x-3). Roots are 2 and 3."},
    {0,3,"What comes next?|1, 1, 2, 3, 5, 8, ?","",{"11","12","13","15"},2,"Fibonacci: add the previous two numbers."},
    {0,3,"What is 1 + 2 + 3 + ... + 20 ?","",{"200","210","190","420"},1,"n(n+1)/2 with n = 20."},
    {0,3,"If 2^x = 64, what is x ?","",{"5","6","7","8"},1,"2,4,8,16,32,64 - count the doublings."},
    {0,3,"What comes next?|2, 6, 12, 20, 30, ?","",{"40","42","44","36"},1,"Differences grow: 4, 6, 8, 10, then 12."},
    // ---------------------------------- LOGIC ---------------------------------
    {1,1,"All cats are animals. Tom is a cat.|Therefore Tom is...","",{"a dog","an animal","a plant","a bird"},1,"If all cats are animals, every cat is one."},
    {1,1,"Find the pattern:|A B A B A ?","",{"A","B","C","D"},1,"The letters alternate."},
    {1,1,"Which one is the odd one out?|Apple, Banana, Carrot, Mango","",{"Apple","Banana","Carrot","Mango"},2,"Three are fruits."},
    {1,1,"What day is two days after Monday?","",{"Tuesday","Wednesday","Thursday","Friday"},1,"Mon -> Tue -> ?"},
    {1,1,"Memory! Which number was THIRD|in the sequence you just saw?","7  2  9  4",{"2","9","4","7"},1,"Count: 7, 2, then the third one."},
    {1,2,"What letter comes next?|B, D, F, H, ?","",{"I","J","K","L"},1,"Skip one letter each time."},
    {1,2,"All Bloops are Razzies, and all Razzies|are Lazzies. Are all Bloops Lazzies?","",{"Yes","No","Only some","Cannot say"},0,"Chain the two statements together."},
    {1,2,"Smallest number using the digits|3, 1 and 4 (each once)?","",{"134","143","314","341"},0,"Put the smallest digit first."},
    {1,2,"Memory! Which colour came SECOND?","RED  BLUE  GREEN  GOLD",{"Red","Blue","Green","Gold"},1,"Recall the order you just saw."},
    {1,2,"A is taller than B, B is taller than C.|Who is the shortest?","",{"A","B","C","Cannot tell"},2,"Order them: A > B > C."},
    {1,2,"What comes next?|1, 2, 4, 7, 11, ?","",{"15","16","17","18"},1,"Differences are 1, 2, 3, 4, then 5."},
    {1,3,"What is the angle between the clock|hands at exactly 3:00 ?","",{"60 degrees","90 degrees","120 degrees","180 degrees"},1,"The clock is split into 12 parts of 30 degrees."},
    {1,3,"A farmer has 17 sheep. All but 9 run|away. How many sheep are left?","",{"8","9","17","0"},1,"'All but 9' means 9 stay."},
    {1,3,"What comes next?|2, 3, 5, 7, 11, 13, ?","",{"15","17","19","21"},1,"These are prime numbers."},
    {1,3,"Largest EVEN number using digits|1, 2, 3, 4 (each once)?","",{"4321","4312","4132","3412"},1,"Even means the last digit is 2 or 4."},
    // ------------------------------ PROGRAMMING (C++) -------------------------
    {2,1,"What does this print?|int x = 5; cout << x * 2;","",{"5","10","25","52"},1,"Multiply 5 by 2."},
    {2,1,"Which header is needed for cout ?","",{"stdio.h","iostream","math.h","string"},1,"cout belongs to the input/output stream library."},
    {2,1,"Which loop checks its condition BEFORE|running the body the first time?","",{"do-while","while","if","switch"},1,"do-while runs first and checks later."},
    {2,1,"What does this print?|int a = 7 % 3; cout << a;","",{"0","1","2","3"},1,"% gives the remainder of 7 / 3."},
    {2,1,"What is the index of the FIRST element|of an array in C++ ?","",{"0","1","-1","2"},0,"C++ arrays start counting at zero."},
    {2,1,"What does this print?|cout << (3 > 2);","",{"0","1","true","false"},1,"A true comparison prints 1 by default."},
    {2,2,"What does this print?|int i; for(i=0;i<5;i++){} cout << i;","",{"4","5","6","0"},1,"The loop ends when i reaches 5."},
    {2,2,"What does this print?|int a=10; a-=3; a*=2; cout << a;","",{"7","14","13","20"},1,"10-3 = 7, then 7*2."},
    {2,2,"On a 32-bit compiler, sizeof(int) is...","",{"2","4","8","16"},1,"An int is usually 32 bits = 4 bytes."},
    {2,2,"What does this print?|int x=3; int *p=&x; *p=8; cout << x;","",{"3","8","an address","error"},1,"*p = 8 writes into x itself."},
    {2,2,"DEBUG: arr has 5 items and the loop is|for(int i=0;i<=5;i++) arr[i]=0;  Bug?","",{"Missing semicolon","Out-of-bounds at i==5","Wrong header","No bug"},1,"Valid indices are 0..4."},
    {2,2,"What does this print?|string s = \"abc\"; cout << s.size();","",{"2","3","4","0"},1,"Count the characters."},
    {2,3,"What does this print?|int f(int n){return n<=1?1:n*f(n-1);}|cout << f(5);","",{"24","60","120","720"},2,"5 factorial = 5*4*3*2*1."},
    {2,3,"Time complexity of binary search?","",{"O(n)","O(log n)","O(n^2)","O(1)"},1,"The search range halves every step."},
    {2,3,"What does this print?|int x=0; while(x<10){ x+=3; } cout << x;","",{"9","10","12","13"},2,"x: 0, 3, 6, 9, 12 - stops when x >= 10."},
    {2,3,"DEBUG: what is wrong with  if(x = 5) ?","",{"Missing else","It assigns 5 and is always true","It compares x with 5","Syntax error"},1,"Single = assigns, double == compares."},
    {2,3,"Worst-case time complexity of bubble sort?","",{"O(n)","O(n log n)","O(n^2)","O(log n)"},2,"Two nested loops over n items."}
    };
    for (size_t i = 0; i < sizeof(q) / sizeof(q[0]); ++i) bank.push_back(q[i]);
}

// ======================================================================
// SECTION 4 - SMALL HELPERS AND DRAWING PRIMITIVES
// ======================================================================
struct Circ { float x, z, r; };            // round obstacle (tree, lamp, fountain)
struct Rect { float x0, z0, x1, z1; };     // box obstacle
struct TreeInfo { float x, z, s; };
struct BenchInfo { float x, z, rot; };

static std::vector<Circ>      circles;
static std::vector<Rect>      walls;       // permanent building walls
static std::vector<TreeInfo>  trees;
static std::vector<BenchInfo> benches;
static std::vector<Circ>      lamps;

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static float frand01() { return (float)rand() / (float)RAND_MAX; }
static float distXZ(float ax, float az, float bx, float bz)
{
    float dx = ax - bx, dz = az - bz;
    return std::sqrt(dx * dx + dz * dz);
}
static std::string itos(int v) { char b[32]; std::sprintf(b, "%d", v); return std::string(b); }
static std::string ftos2(float v) { char b[32]; std::sprintf(b, "%.2f", v); return std::string(b); }

// ---- 3D primitives --------------------------------------------------
static void box(float cx, float cy, float cz, float w, float h, float d, float r, float gg, float b)
{
    glColor3f(r, gg, b);
    glPushMatrix();
    glTranslatef(cx, cy, cz);
    glScalef(w, h, d);
    glutSolidCube(1.0);
    glPopMatrix();
}
static void sphere(float cx, float cy, float cz, float rad, float r, float gg, float b)
{
    glColor3f(r, gg, b);
    glPushMatrix();
    glTranslatef(cx, cy, cz);
    glutSolidSphere(rad, 14, 10);
    glPopMatrix();
}
static void coneUp(float cx, float cy, float cz, float rad, float h, float r, float gg, float b)
{
    glColor3f(r, gg, b);
    glPushMatrix();
    glTranslatef(cx, cy, cz);
    glRotatef(-90.0f, 1, 0, 0);
    glutSolidCone(rad, h, 12, 2);
    glPopMatrix();
}
static void coneDown(float cx, float cy, float cz, float rad, float h, float r, float gg, float b)
{
    glColor3f(r, gg, b);
    glPushMatrix();
    glTranslatef(cx, cy, cz);
    glRotatef(90.0f, 1, 0, 0);
    glutSolidCone(rad, h, 12, 2);
    glPopMatrix();
}
static void flatDisc(float cx, float cz, float rad, float y, float r, float gg, float b, float a)
{
    glDisable(GL_LIGHTING);
    glColor4f(r, gg, b, a);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(cx, y, cz);
    for (int i = 0; i <= 20; ++i) {
        float t = i * 2.0f * PI_F / 20.0f;
        glVertex3f(cx + std::cos(t) * rad, y, cz + std::sin(t) * rad);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}
static void groundQuad(float x0, float z0, float x1, float z1, float y, float r, float gg, float b)
{
    glColor3f(r, gg, b);
    glNormal3f(0, 1, 0);
    glBegin(GL_QUADS);
    glVertex3f(x0, y, z0); glVertex3f(x0, y, z1);
    glVertex3f(x1, y, z1); glVertex3f(x1, y, z0);
    glEnd();
}

// ---- 2D overlay helpers ---------------------------------------------
static void begin2D()
{
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, g.winW, 0, g.winH);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
static void end2D()
{
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}
static void rect2D(float x, float y, float w, float h, float r, float gg, float b, float a)
{
    glColor4f(r, gg, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
}
static void frame2D(float x, float y, float w, float h, float r, float gg, float b, float lw)
{
    glLineWidth(lw);
    glColor3f(r, gg, b);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
    glLineWidth(1.0f);
}
static void text2D(float x, float y, const std::string& s, void* font, float r, float gg, float b)
{
    glColor3f(r, gg, b);
    glRasterPos2f(x, y);
    for (size_t i = 0; i < s.size(); ++i) glutBitmapCharacter(font, (unsigned char)s[i]);
}
static float textWidth(const std::string& s, void* font)
{
    float w = 0;
    for (size_t i = 0; i < s.size(); ++i) w += (float)glutBitmapWidth(font, (unsigned char)s[i]);
    return w;
}
static void textCentered(float cx, float y, const std::string& s, void* font, float r, float gg, float b)
{
    text2D(cx - textWidth(s, font) * 0.5f, y, s, font, r, gg, b);
}
// Multi-line text: lines are separated by '|'
static int textLines(float cx, float y, const std::string& s, void* font, float lineH,
                     float r, float gg, float b)
{
    size_t start = 0; int n = 0;
    while (start <= s.size()) {
        size_t p = s.find('|', start);
        std::string line = s.substr(start, p == std::string::npos ? std::string::npos : p - start);
        textCentered(cx, y - n * lineH, line, font, r, gg, b);
        ++n;
        if (p == std::string::npos) break;
        start = p + 1;
    }
    return n;
}
// Word-wrapped paragraph, left aligned
static int paragraph(float x, float y, const std::string& s, void* font, float maxW, float lineH,
                     float r, float gg, float b)
{
    std::string line, word; int n = 0;
    std::string t = s + " ";
    for (size_t i = 0; i < t.size(); ++i) {
        if (t[i] == ' ') {
            std::string test = line.empty() ? word : line + " " + word;
            if (textWidth(test, font) > maxW && !line.empty()) {
                text2D(x, y - n * lineH, line, font, r, gg, b); ++n; line = word;
            } else line = test;
            word.clear();
        } else word += t[i];
    }
    if (!line.empty()) { text2D(x, y - n * lineH, line, font, r, gg, b); ++n; }
    return n;
}

// ======================================================================
// SECTION 5 - CAMPUS DATA
// ======================================================================
static void addBuilding(const char* name, float x, float z, float w, float d, float h,
                        float r, float gg, float b, int unlock)
{
    Building bd;
    bd.name = name; bd.x = x; bd.z = z; bd.w = w; bd.d = d; bd.h = h;
    bd.r = r; bd.g = gg; bd.b = b;
    bd.doorX = x; bd.doorZ = z + d * 0.5f + 2.5f;
    bd.unlockLevel = unlock;
    g.buildings.push_back(bd);
}
static void addNPC(const char* name, int building, float dx, float dz, float r, float gg, float b)
{
    NPC n;
    n.name = name; n.building = building;
    n.x = g.buildings[building].doorX + dx;
    n.z = g.buildings[building].doorZ + dz;
    n.r = r; n.g = gg; n.b = b; n.facing = 0.0f;
    g.npcs.push_back(n);
}

// Build every building, NPC, tree, bench, lamp and collision shape.
static void initializeCampus()
{
    g.buildings.clear(); g.npcs.clear();
    circles.clear(); walls.clear(); trees.clear(); benches.clear(); lamps.clear();

    //            name                          x     z    w    d    h     colour             unlock
    addBuilding("ACADEMIC BUILDING",          -26.0f, 34.0f, 14, 9,  8, 0.85f,0.55f,0.35f, 1);
    addBuilding("MATHEMATICS BUILDING",       -28.0f,-14.0f, 13, 9,  9, 0.35f,0.55f,0.90f, 1);
    addBuilding("CENTRAL LIBRARY",            -14.0f,-30.0f, 16,10,  9, 0.62f,0.40f,0.75f, 2);
    addBuilding("CSE LABORATORY",             -28.0f, 12.0f, 13, 9,  8, 0.25f,0.75f,0.75f, 3);
    addBuilding("EXAMINATION HALL",            28.0f, 12.0f, 15,10,  8, 0.90f,0.35f,0.35f, 4);
    addBuilding("CAFETERIA",                   26.0f, 34.0f, 12, 8,  5, 0.95f,0.75f,0.25f, 1);
    addBuilding("PROJECT LABORATORY",          28.0f,-14.0f, 14,10,  8, 0.35f,0.80f,0.45f, 5);
    addBuilding("GRADUATION AUDITORIUM",        0.0f,-44.0f, 22,10, 11, 0.95f,0.90f,0.80f, 6);

    // Teachers (one near every building)
    addNPC("Prof. Orion",   B_ACADEMIC,   4.5f, 0.5f, 0.55f,0.25f,0.20f);
    addNPC("Prof. Algebra", B_MATH,       4.5f, 0.5f, 0.15f,0.30f,0.65f);
    addNPC("Ms. Logica",    B_LIBRARY,    4.5f, 0.5f, 0.50f,0.20f,0.60f);
    addNPC("Dr. Compile",   B_CSE,        4.5f, 0.5f, 0.10f,0.50f,0.50f);
    addNPC("Examiner Kay",  B_EXAM,      -4.5f, 0.5f, 0.55f,0.10f,0.10f);
    addNPC("Chef Mira",     B_CAFE,       4.0f, 0.5f, 0.95f,0.95f,0.95f);
    addNPC("Supervisor Rao",B_PROJECT,   -4.5f, 0.5f, 0.15f,0.45f,0.25f);
    // The Dean stands on the graduation stage
    {
        NPC n; n.name = "Dean Aurelia"; n.building = B_AUDITORIUM;
        n.x = 0.0f; n.z = -47.0f; n.r = 0.1f; n.g = 0.1f; n.b = 0.35f; n.facing = 0.0f;
        g.npcs.push_back(n);
    }

    // Building collision boxes (auditorium is open-fronted: walls only)
    for (int i = 0; i < B_COUNT; ++i) {
        const Building& b = g.buildings[i];
        if (i == B_AUDITORIUM) {
            Rect back  = { b.x - b.w * 0.5f, b.z - b.d * 0.5f - 0.5f, b.x + b.w * 0.5f, b.z - b.d * 0.5f + 0.5f };
            Rect left  = { b.x - b.w * 0.5f - 0.5f, b.z - b.d * 0.5f, b.x - b.w * 0.5f + 0.5f, b.z + b.d * 0.5f };
            Rect right = { b.x + b.w * 0.5f - 0.5f, b.z - b.d * 0.5f, b.x + b.w * 0.5f + 0.5f, b.z + b.d * 0.5f };
            walls.push_back(back); walls.push_back(left); walls.push_back(right);
        } else {
            Rect r = { b.x - b.w * 0.5f, b.z - b.d * 0.5f, b.x + b.w * 0.5f, b.z + b.d * 0.5f };
            walls.push_back(r);
        }
    }

    // Fountain in the middle of the east lawn
    Circ f = { 14.0f, -30.0f, 3.0f };
    circles.push_back(f);

    // Trees (hand-placed rows + random scatter, kept off roads and buildings)
    srand(12345);
    for (int i = 0; i < 90 && (int)trees.size() < 46; ++i) {
        float x = (frand01() * 2.0f - 1.0f) * 47.0f;
        float z = (frand01() * 2.0f - 1.0f) * 47.0f;
        if (std::fabs(x) < 7.0f && z > -40.0f) continue;      // main road + plaza
        if (std::fabs(z) < 6.0f) continue;                     // cross road
        bool ok = true;
        for (size_t k = 0; k < g.buildings.size(); ++k) {
            const Building& b = g.buildings[k];
            if (std::fabs(x - b.x) < b.w * 0.5f + 5.0f && std::fabs(z - b.z) < b.d * 0.5f + 7.0f) ok = false;
        }
        if (distXZ(x, z, 14.0f, -30.0f) < 7.0f) ok = false;
        for (size_t k = 0; k < trees.size(); ++k)
            if (distXZ(x, z, trees[k].x, trees[k].z) < 6.0f) ok = false;
        if (!ok) continue;
        TreeInfo t = { x, z, 0.8f + frand01() * 0.6f };
        trees.push_back(t);
        Circ c = { x, z, 0.9f };
        circles.push_back(c);
    }
    // Lamps along the main road
    for (int i = 0; i < 6; ++i) {
        float z = 40.0f - i * 10.0f;
        Circ l1 = { -4.5f, z, 0.35f }, l2 = { 4.5f, z, 0.35f };
        lamps.push_back(l1); lamps.push_back(l2);
        circles.push_back(l1); circles.push_back(l2);
    }
    // Benches (decorative; the cafeteria rest area has several)
    BenchInfo bn[] = { {19,26,0},{24,26,0},{29,26,0},{-8,20,90},{8,20,-90},{-8,-8,90},{8,-8,-90},{20,-26,0},{8,-34,0} };
    for (size_t i = 0; i < sizeof(bn) / sizeof(bn[0]); ++i) {
        benches.push_back(bn[i]);
    }
}

// ======================================================================
// SECTION 6 - 3D RENDERING OF THE CAMPUS
// ======================================================================
static bool isLocked(int i) { return g.currentLevel < g.buildings[i].unlockLevel; }

// Barrier (tape) that blocks the entrance of a locked building
static Rect barrierRect(int i)
{
    const Building& b = g.buildings[i];
    float half = (i == B_AUDITORIUM) ? 9.5f : 2.8f;
    float zf = b.z + b.d * 0.5f + 1.4f;
    Rect r = { b.doorX - half, zf - 0.3f, b.doorX + half, zf + 0.3f };
    return r;
}

static void drawBarrier(int i)
{
    Rect r = barrierRect(i);
    float cx = (r.x0 + r.x1) * 0.5f, cz = (r.z0 + r.z1) * 0.5f, w = r.x1 - r.x0;
    box(r.x0, 0.6f, cz, 0.2f, 1.2f, 0.2f, 0.9f, 0.1f, 0.1f);
    box(r.x1, 0.6f, cz, 0.2f, 1.2f, 0.2f, 0.9f, 0.1f, 0.1f);
    int n = (int)(w / 0.8f);
    for (int k = 0; k < n; ++k) {
        float px = r.x0 + (k + 0.5f) * (w / n);
        bool red = (k % 2 == 0);
        box(px, 0.95f, cz, w / n, 0.22f, 0.1f, red ? 0.9f : 1.0f, red ? 0.1f : 1.0f, red ? 0.1f : 1.0f);
        box(px, 0.55f, cz, w / n, 0.22f, 0.1f, red ? 1.0f : 0.9f, red ? 1.0f : 0.1f, red ? 1.0f : 0.1f);
    }
    (void)cx;
}

// 3D text label fixed in the world, centred horizontally with glBitmap
static void worldText(float x, float y, float z, const std::string& s, void* font, float r, float gg, float b)
{
    glDisable(GL_LIGHTING);
    glColor3f(r, gg, b);
    glRasterPos3f(x, y, z);
    glBitmap(0, 0, 0, 0, -textWidth(s, font) * 0.5f, 0, 0);
    for (size_t i = 0; i < s.size(); ++i) glutBitmapCharacter(font, (unsigned char)s[i]);
    glEnable(GL_LIGHTING);
}

static void windowsFront(const Building& b, float zface, float dirZ, float r, float gg, float bl, bool skipDoor)
{
    int cols = (int)(b.w / 3.0f);
    int rows = (b.h > 7.5f) ? 2 : 1;
    for (int ry = 0; ry < rows; ++ry)
        for (int cx = 0; cx < cols; ++cx) {
            float x = b.x - b.w * 0.5f + (cx + 0.5f) * (b.w / cols);
            float y = (ry == 0 ? 3.6f : 6.4f);
            if (skipDoor && ry == 0 && std::fabs(x - b.x) < 2.0f) continue;
            box(x, y, zface + dirZ * 0.05f, 1.3f, 1.4f, 0.12f, r, gg, bl);
            box(x, y, zface + dirZ * 0.12f, 1.5f, 0.12f, 0.1f, 1, 1, 1);
        }
}
static void windowsSide(const Building& b, float xface, float dirX)
{
    int cols = (int)(b.d / 3.0f);
    int rows = (b.h > 7.5f) ? 2 : 1;
    for (int ry = 0; ry < rows; ++ry)
        for (int cz = 0; cz < cols; ++cz) {
            float z = b.z - b.d * 0.5f + (cz + 0.5f) * (b.d / cols);
            float y = (ry == 0 ? 3.6f : 6.4f);
            box(xface + dirX * 0.05f, y, z, 0.12f, 1.4f, 1.3f, 0.65f, 0.85f, 1.0f);
        }
}

static void drawAuditorium(const Building& b, bool locked)
{
    float x0 = b.x - b.w * 0.5f, x1 = b.x + b.w * 0.5f;
    float zb = b.z - b.d * 0.5f, zf = b.z + b.d * 0.5f;
    // floor, carpet, stage
    box(b.x, 0.05f, b.z, b.w, 0.1f, b.d, 0.55f, 0.45f, 0.35f);
    box(b.x, 0.12f, b.z + 1.0f, 3.0f, 0.06f, b.d - 3.0f, 0.75f, 0.1f, 0.15f);
    box(b.x, 0.45f, zb + 2.0f, b.w - 3.0f, 0.8f, 4.0f, 0.60f, 0.40f, 0.25f);
    box(b.x, 0.9f, zb + 0.9f, 6.0f, 0.15f, 0.4f, 0.9f, 0.8f, 0.2f);   // podium bar
    // walls
    box(b.x, 5.5f, zb, b.w, 11.0f, 1.0f, b.r, b.g, b.b);
    box(x0, 5.5f, b.z, 1.0f, 11.0f, b.d, b.r, b.g, b.b);
    box(x1, 5.5f, b.z, 1.0f, 11.0f, b.d, b.r, b.g, b.b);
    // banner behind the stage
    box(b.x, 6.0f, zb + 0.6f, 9.0f, 3.0f, 0.1f, 0.15f, 0.15f, 0.55f);
    // seats
    for (int row = 0; row < 3; ++row)
        for (int s = -4; s <= 4; ++s) {
            if (s == 0) continue;
            box(b.x + s * 1.8f, 0.45f, b.z + 1.5f + row * 1.8f, 1.2f, 0.7f, 0.9f, 0.2f, 0.25f, 0.6f);
        }
    // roof + front columns + dome
    box(b.x, 11.2f, b.z, b.w + 1.4f, 0.8f, b.d + 1.4f, 0.55f, 0.20f, 0.20f);
    for (int c = 0; c < 6; ++c) {
        float cx = x0 + 1.5f + c * ((b.w - 3.0f) / 5.0f);
        box(cx, 5.5f, zf, 1.0f, 11.0f, 1.0f, 0.97f, 0.95f, 0.9f);
    }
    glPushMatrix();
    glTranslatef(b.x, 11.4f, b.z);
    glColor3f(0.85f, 0.7f, 0.2f);
    glutSolidSphere(4.0, 16, 12);
    glPopMatrix();
    (void)locked;
}

static void drawBuildingDecor(int i, const Building& b)
{
    float zf = b.z + b.d * 0.5f;
    switch (i) {
    case B_ACADEMIC:        // clock tower
        box(b.x + 4.0f, b.h + 2.5f, b.z, 2.6f, 5.0f, 2.6f, 0.75f, 0.45f, 0.30f);
        coneUp(b.x + 4.0f, b.h + 5.0f, b.z, 2.0f, 3.0f, 0.55f, 0.15f, 0.15f);
        sphere(b.x + 4.0f, b.h + 3.0f, b.z + 1.35f, 0.8f, 1.0f, 1.0f, 0.9f);
        break;
    case B_MATH:            // floating math symbols
        worldText(b.x, b.h + 3.2f + 0.3f * std::sin(g.clock * 2.0f), b.z, "x^2 + y^2 = z^2", GLUT_BITMAP_HELVETICA_18, 1, 1, 0.3f);
        break;
    case B_LIBRARY:         // columns
        for (int c = -2; c <= 2; ++c) box(b.x + c * 3.0f, 3.5f, zf + 0.6f, 0.8f, 7.0f, 0.8f, 0.97f, 0.95f, 0.9f);
        box(b.x, 7.2f, zf + 0.6f, b.w, 0.5f, 1.4f, 0.97f, 0.95f, 0.9f);
        break;
    case B_CSE:             // antenna + dish
        box(b.x - 3.0f, b.h + 2.0f, b.z, 0.2f, 4.0f, 0.2f, 0.8f, 0.8f, 0.8f);
        sphere(b.x - 3.0f, b.h + 4.2f, b.z, 0.35f, 1.0f, 0.2f, 0.2f);
        glPushMatrix(); glTranslatef(b.x + 3.0f, b.h + 1.2f, b.z); glRotatef(g.clock * 40.0f, 0, 1, 0);
        glRotatef(-40.0f, 1, 0, 0); glColor3f(0.9f, 0.9f, 0.95f); glutSolidCone(1.6, 0.9, 14, 2); glPopMatrix();
        break;
    case B_EXAM:            // flagpole
        box(b.x + 5.5f, b.h + 2.5f, b.z, 0.15f, 5.0f, 0.15f, 0.8f, 0.8f, 0.8f);
        box(b.x + 6.3f, b.h + 4.3f + 0.1f * std::sin(g.clock * 4.0f), b.z, 1.6f, 0.9f, 0.05f, 0.9f, 0.2f, 0.2f);
        break;
    case B_CAFE:            // striped awning + chimney
        for (int k = 0; k < 6; ++k)
            box(b.x - 4.0f + k * 1.6f + 0.8f, 3.4f, zf + 1.2f, 1.6f, 0.15f, 2.4f, (k % 2) ? 1.0f : 0.9f, (k % 2) ? 1.0f : 0.25f, (k % 2) ? 1.0f : 0.25f);
        box(b.x + 3.5f, b.h + 1.0f, b.z - 1.0f, 1.0f, 2.0f, 1.0f, 0.6f, 0.3f, 0.2f);
        break;
    case B_PROJECT:         // glass tower + lab sign
        box(b.x - 3.5f, b.h + 2.0f, b.z, 3.0f, 4.0f, 3.0f, 0.4f, 0.9f, 0.9f);
        sphere(b.x + 3.0f, b.h + 0.8f, b.z, 1.0f, 0.9f, 0.9f, 0.3f);
        break;
    default: break;
    }
}

// All buildings: walls, roofs, windows, door, sign, lock tape.
static void drawBuildings()
{
    for (int i = 0; i < B_COUNT; ++i) {
        const Building& b = g.buildings[i];
        bool locked = isLocked(i);
        if (i == B_AUDITORIUM) {
            drawAuditorium(b, locked);
        } else {
            float zf = b.z + b.d * 0.5f, zb = b.z - b.d * 0.5f;
            box(b.x, b.h * 0.5f, b.z, b.w, b.h, b.d, b.r, b.g, b.b);
            box(b.x, b.h + 0.25f, b.z, b.w + 0.8f, 0.5f, b.d + 0.8f, b.r * 0.55f, b.g * 0.55f, b.b * 0.55f);
            box(b.x, 0.2f, b.z, b.w + 0.4f, 0.4f, b.d + 0.4f, 0.6f, 0.6f, 0.6f);   // plinth
            windowsFront(b, zf, 1.0f, 0.65f, 0.85f, 1.0f, true);
            windowsFront(b, zb, -1.0f, 0.65f, 0.85f, 1.0f, false);
            windowsSide(b, b.x - b.w * 0.5f, -1.0f);
            windowsSide(b, b.x + b.w * 0.5f, 1.0f);
            // door
            float dr = locked ? 0.8f : 0.2f, dg = locked ? 0.15f : 0.75f, db = 0.2f;
            box(b.x, 1.6f, zf + 0.05f, 2.4f, 3.2f, 0.2f, dr, dg, db);
            box(b.x, 3.4f, zf + 0.05f, 3.0f, 0.3f, 0.3f, 0.95f, 0.95f, 0.95f);
            box(b.x + 0.8f, 1.5f, zf + 0.2f, 0.15f, 0.15f, 0.1f, 1, 0.9f, 0.2f);
            // sign board
            box(b.x, b.h - 0.9f, zf + 0.2f, 9.0f, 1.1f, 0.2f, 0.12f, 0.12f, 0.18f);
        }
        drawBuildingDecor(i, b);
        // building name
        float ty = (i == B_AUDITORIUM) ? 13.2f : (b.h - 1.2f);
        float tz = b.z + b.d * 0.5f + 0.4f;
        worldText(b.x, ty, tz, b.name, GLUT_BITMAP_HELVETICA_18, 1.0f, 1.0f, 1.0f);
        if (locked) {
            drawBarrier(i);
            worldText(b.x, 3.9f, b.z + b.d * 0.5f + 1.6f, "LOCKED", GLUT_BITMAP_HELVETICA_18, 1.0f, 0.3f, 0.3f);
        }
    }
}

static void drawTree(const TreeInfo& t)
{
    float sway = std::sin(g.clock * 1.5f + t.x * 0.7f) * 2.5f;
    glPushMatrix();
    glTranslatef(t.x, 0, t.z);
    glScalef(t.s, t.s, t.s);
    box(0, 1.2f, 0, 0.5f, 2.4f, 0.5f, 0.45f, 0.28f, 0.15f);
    glTranslatef(0, 2.2f, 0);
    glRotatef(sway, 0, 0, 1);
    coneUp(0, 0.0f, 0, 2.0f, 2.8f, 0.15f, 0.60f, 0.25f);
    coneUp(0, 1.5f, 0, 1.6f, 2.5f, 0.20f, 0.70f, 0.30f);
    coneUp(0, 2.8f, 0, 1.1f, 2.1f, 0.25f, 0.78f, 0.35f);
    glPopMatrix();
}
static void drawLamp(const Circ& l)
{
    box(l.x, 2.0f, l.z, 0.14f, 4.0f, 0.14f, 0.2f, 0.2f, 0.25f);
    sphere(l.x, 4.15f, l.z, 0.32f, 1.0f, 0.95f, 0.6f);
}
static void drawBench(const BenchInfo& b)
{
    glPushMatrix();
    glTranslatef(b.x, 0, b.z);
    glRotatef(b.rot, 0, 1, 0);
    box(0, 0.5f, 0, 1.8f, 0.12f, 0.6f, 0.55f, 0.35f, 0.2f);
    box(0, 0.9f, -0.28f, 1.8f, 0.5f, 0.08f, 0.55f, 0.35f, 0.2f);
    box(-0.8f, 0.25f, 0, 0.1f, 0.5f, 0.5f, 0.2f, 0.2f, 0.2f);
    box(0.8f, 0.25f, 0, 0.1f, 0.5f, 0.5f, 0.2f, 0.2f, 0.2f);
    glPopMatrix();
}

static void drawFountain()
{
    box(14.0f, 0.4f, -30.0f, 6.0f, 0.8f, 6.0f, 0.7f, 0.7f, 0.75f);
    box(14.0f, 0.85f, -30.0f, 5.0f, 0.12f, 5.0f, 0.3f, 0.6f, 0.95f);
    box(14.0f, 1.4f, -30.0f, 0.8f, 1.6f, 0.8f, 0.75f, 0.75f, 0.8f);
    for (int k = 0; k < 4; ++k) {
        float a = g.clock * 2.0f + k * PI_F * 0.5f;
        sphere(14.0f + std::cos(a) * 0.5f, 2.3f + 0.3f * std::sin(a * 2.0f), -30.0f + std::sin(a) * 0.5f, 0.18f, 0.5f, 0.8f, 1.0f);
    }
}

// Ground, roads, paths, fence, gate and all scenery.
static void drawCampus()
{
    // grass
    groundQuad(-70, -70, 70, 70, -0.02f, 0.30f, 0.68f, 0.30f);
    groundQuad(-WORLD_HALF + 2, -WORLD_HALF + 2, WORLD_HALF - 2, WORLD_HALF - 2, 0.0f, 0.36f, 0.76f, 0.34f);
    // roads
    groundQuad(-3.0f, -38.0f, 3.0f, 50.0f, 0.02f, 0.28f, 0.28f, 0.30f);
    groundQuad(-40.0f, -3.0f, 40.0f, 3.0f, 0.03f, 0.28f, 0.28f, 0.30f);
    for (int k = 0; k < 20; ++k) {   // dashed centre line
        float z = 48.0f - k * 4.4f;
        if (z < -36.0f) break;
        groundQuad(-0.12f, z - 1.2f, 0.12f, z, 0.04f, 1.0f, 0.95f, 0.5f);
    }
    // walking paths to doors
    for (int i = 0; i < B_COUNT; ++i) {
        const Building& b = g.buildings[i];
        float zf = b.z + b.d * 0.5f;
        float px = b.doorX, pz = b.doorZ;
        if (i == B_AUDITORIUM) continue;
        float roadX = (px < 0) ? -3.0f : 3.0f;
        if (std::fabs(px) > 5.0f) {
            // horizontal path from road to door x, then vertical to the door
            float x0 = (px < 0) ? px : roadX, x1 = (px < 0) ? roadX : px;
            groundQuad(x0, pz - 1.0f, x1, pz + 1.0f, 0.05f, 0.85f, 0.78f, 0.58f);
        }
        groundQuad(px - 1.2f, zf, px + 1.2f, pz + 1.0f, 0.05f, 0.85f, 0.78f, 0.58f);
    }
    // plaza before the auditorium + red carpet
    groundQuad(-9.0f, -40.0f, 9.0f, -34.0f, 0.04f, 0.80f, 0.72f, 0.55f);
    groundQuad(-1.5f, -38.0f, 1.5f, -34.0f, 0.06f, 0.75f, 0.1f, 0.15f);

    // fence with gate opening at the south side
    for (int s = 0; s < 2; ++s) {
        float sign = s == 0 ? 1.0f : -1.0f;
        for (float p = -50.0f; p <= 50.01f; p += 4.0f) {
            box(p, 0.8f, sign * 50.0f, 0.2f, 1.6f, 0.2f, 0.9f, 0.9f, 0.95f);
            box(sign * 50.0f, 0.8f, p, 0.2f, 1.6f, 0.2f, 0.9f, 0.9f, 0.95f);
        }
        if (s == 0) {
            box(-27.5f, 1.2f, 50.0f, 45.0f, 0.1f, 0.1f, 0.9f, 0.9f, 0.95f);
            box( 27.5f, 1.2f, 50.0f, 45.0f, 0.1f, 0.1f, 0.9f, 0.9f, 0.95f);
        } else box(0, 1.2f, -50.0f, 100.0f, 0.1f, 0.1f, 0.9f, 0.9f, 0.95f);
        box(sign * 50.0f, 1.2f, 0.0f, 0.1f, 0.1f, 100.0f, 0.9f, 0.9f, 0.95f);
    }
    // main gate
    box(-5.5f, 3.5f, 50.0f, 1.6f, 7.0f, 1.6f, 0.80f, 0.25f, 0.25f);
    box( 5.5f, 3.5f, 50.0f, 1.6f, 7.0f, 1.6f, 0.80f, 0.25f, 0.25f);
    box(0.0f, 7.0f, 50.0f, 13.0f, 1.8f, 1.2f, 0.15f, 0.15f, 0.40f);
    worldText(0.0f, 6.6f, 50.7f, "WELCOME TO CAMPUS QUEST UNIVERSITY", GLUT_BITMAP_HELVETICA_18, 1.0f, 0.95f, 0.4f);
    worldText(0.0f, 6.6f, 49.3f, "WELCOME TO CAMPUS QUEST UNIVERSITY", GLUT_BITMAP_HELVETICA_18, 1.0f, 0.95f, 0.4f);
    sphere(-5.5f, 7.4f, 50.0f, 0.5f, 1.0f, 0.85f, 0.2f);
    sphere( 5.5f, 7.4f, 50.0f, 0.5f, 1.0f, 0.85f, 0.2f);

    drawFountain();
    for (size_t i = 0; i < trees.size(); ++i)   drawTree(trees[i]);
    for (size_t i = 0; i < lamps.size(); ++i)   drawLamp(lamps[i]);
    for (size_t i = 0; i < benches.size(); ++i) drawBench(benches[i]);
    // cafeteria tables
    for (int k = 0; k < 3; ++k) {
        float x = 18.0f + k * 5.0f;
        box(x, 0.8f, 28.0f, 1.4f, 0.1f, 1.4f, 0.9f, 0.9f, 0.9f);
        box(x, 0.4f, 28.0f, 0.2f, 0.8f, 0.2f, 0.4f, 0.4f, 0.4f);
        coneUp(x, 0.9f, 28.0f, 0.2f, 0.9f, 1.0f, 0.3f, 0.3f);
    }
}

// ---- characters -----------------------------------------------------
static void drawPerson(float x, float y, float z, float heading, float phase, bool walking,
                       float sr, float sg, float sb, float pr, float pg, float pb,
                       float hr, float hg, float hb, bool backpack, bool gradCap)
{
    flatDisc(x, z, 0.75f - std::min(y, 2.0f) * 0.15f, 0.07f, 0.0f, 0.0f, 0.0f, 0.35f);
    float swing = walking ? std::sin(phase) * 38.0f : 0.0f;
    float bob = walking ? std::fabs(std::sin(phase)) * 0.08f : 0.0f;
    glPushMatrix();
    glTranslatef(x, y + bob, z);
    glRotatef(heading, 0, 1, 0);
    // legs
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix();
        glTranslatef(0.2f * s, 0.95f, 0);
        glRotatef(swing * -s, 1, 0, 0);
        box(0, -0.45f, 0, 0.3f, 0.95f, 0.32f, pr, pg, pb);
        box(0, -0.95f, 0.06f, 0.32f, 0.16f, 0.45f, 0.15f, 0.12f, 0.1f);
        glPopMatrix();
    }
    // body
    box(0, 1.55f, 0, 0.85f, 0.95f, 0.45f, sr, sg, sb);
    // arms
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix();
        glTranslatef(0.55f * s, 1.95f, 0);
        glRotatef(swing * s, 1, 0, 0);
        box(0, -0.4f, 0, 0.22f, 0.85f, 0.25f, sr, sg, sb);
        sphere(0, -0.88f, 0, 0.14f, 1.0f, 0.8f, 0.65f);
        glPopMatrix();
    }
    // head
    sphere(0, 2.3f, 0, 0.38f, 1.0f, 0.80f, 0.65f);
    box(0, 2.58f, -0.04f, 0.72f, 0.2f, 0.72f, hr, hg, hb);              // hair
    sphere(-0.14f, 2.33f, 0.34f, 0.06f, 0.05f, 0.05f, 0.05f);            // eyes
    sphere( 0.14f, 2.33f, 0.34f, 0.06f, 0.05f, 0.05f, 0.05f);
    box(0, 2.18f, 0.37f, 0.14f, 0.03f, 0.04f, 0.7f, 0.2f, 0.2f);         // smile
    if (backpack) {
        box(0, 1.6f, -0.45f, 0.65f, 0.8f, 0.35f, 0.9f, 0.45f, 0.1f);
        box(0, 1.35f, -0.65f, 0.4f, 0.3f, 0.1f, 0.7f, 0.3f, 0.05f);
    }
    if (gradCap) {
        box(0, 2.82f, 0, 0.95f, 0.07f, 0.95f, 0.08f, 0.08f, 0.1f);
        box(0, 2.72f, 0, 0.5f, 0.15f, 0.5f, 0.08f, 0.08f, 0.1f);
        box(0.4f, 2.7f, 0.4f, 0.04f, 0.3f, 0.04f, 1.0f, 0.85f, 0.1f);
    }
    glPopMatrix();
}

static void drawPlayer()
{
    drawPerson(player.x, player.y, player.z, player.heading, player.walkPhase, player.moving,
               0.20f, 0.45f, 0.95f, 0.15f, 0.20f, 0.45f, 0.25f, 0.15f, 0.08f, true,
               g.state == GRADUATION || g.state == GAME_COMPLETE);
}

static void drawNPCs()
{
    for (size_t i = 0; i < g.npcs.size(); ++i) {
        const NPC& n = g.npcs[i];
        float wob = std::sin(g.clock * 1.2f + (float)i) * 3.0f;
        drawPerson(n.x, 0.0f, n.z, n.facing + wob, 0, false, n.r, n.g, n.b, 0.2f, 0.2f, 0.25f,
                   0.75f, 0.75f, 0.78f, false, false);
        // exclamation marker above teachers that have something to say
        int lvIdx = -1;
        for (int l = 0; l < 5; ++l) if (LEVEL_BUILDING[l] == n.building) lvIdx = l;
        bool active = (lvIdx >= 0 && g.currentLevel == lvIdx + 1) || (n.building == B_AUDITORIUM && g.currentLevel == 6);
        if (n.building == B_ACADEMIC && g.currentLevel == 1) active = true;
        if (active) {
            float by = 3.6f + 0.25f * std::sin(g.clock * 4.0f);
            sphere(n.x, by + 0.55f, n.z, 0.12f, 1.0f, 0.85f, 0.1f);
            box(n.x, by, n.z, 0.14f, 0.5f, 0.14f, 1.0f, 0.85f, 0.1f);
        }
    }
}

// Spinning golden arrow above the current objective building
static void drawObjectiveMarker()
{
    int target = (g.currentLevel <= 5) ? LEVEL_BUILDING[g.currentLevel - 1] : B_AUDITORIUM;
    const Building& b = g.buildings[target];
    float y = b.h + 6.0f + 0.7f * std::sin(g.clock * 3.0f);
    if (target == B_AUDITORIUM) y = 17.5f + 0.7f * std::sin(g.clock * 3.0f);
    glPushMatrix();
    glTranslatef(b.x, y, b.z + b.d * 0.5f - 1.0f);
    glRotatef(g.clock * 90.0f, 0, 1, 0);
    coneDown(0, 0, 0, 1.0f, 2.2f, 1.0f, 0.82f, 0.1f);
    box(0, 1.4f, 0, 0.7f, 1.4f, 0.7f, 1.0f, 0.82f, 0.1f);
    glPopMatrix();
}

static void drawConfetti()
{
    glDisable(GL_LIGHTING);
    glBegin(GL_QUADS);
    for (size_t i = 0; i < confetti.size(); ++i) {
        const Confetti& c = confetti[i];
        float s = 0.18f, ca = std::cos(c.spin) * s, sa = std::sin(c.spin) * s;
        glColor3f(c.r, c.g, c.b);
        glVertex3f(c.x - ca, c.y - sa, c.z);
        glVertex3f(c.x + sa, c.y - ca, c.z);
        glVertex3f(c.x + ca, c.y + sa, c.z);
        glVertex3f(c.x - sa, c.y + ca, c.z);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

// ======================================================================
// SECTION 7 - ACADEMIC SYSTEM (grades, CGPA, achievements)
// ----------------------------------------------------------------------
//  Challenge percentage = challenge score / maximum possible score * 100
//   80-100 = A+ (4.00)   75-79 = A (3.75)   70-74 = A- (3.50)
//   65-69  = B+ (3.25)   60-64 = B (3.00)   50-59 = C  (2.50)
//   below 50 = F (0.00)  -> module FAILED, must be retaken.
//  CGPA = sum(gradePoint * credits) / sum(credits) over attempted modules.
//  (CGPA never uses the raw game score.)
// ======================================================================
static void gradeFromPercent(int pct, std::string& grade, float& gp)
{
    if      (pct >= 80) { grade = "A+"; gp = 4.00f; }
    else if (pct >= 75) { grade = "A";  gp = 3.75f; }
    else if (pct >= 70) { grade = "A-"; gp = 3.50f; }
    else if (pct >= 65) { grade = "B+"; gp = 3.25f; }
    else if (pct >= 60) { grade = "B";  gp = 3.00f; }
    else if (pct >= 50) { grade = "C";  gp = 2.50f; }
    else                { grade = "F";  gp = 0.00f; }
}

static void calculateCGPA()
{
    float pts = 0.0f; int cr = 0, earned = 0, done = 0, total = 0;
    for (int i = 0; i < 5; ++i) {
        if (acad.attempted[i]) { pts += acad.gradePoint[i] * acad.credits[i]; cr += acad.credits[i]; }
        if (acad.passed[i])    { earned += acad.credits[i]; ++done; }
        total += acad.bestScore[i];
    }
    acad.cgpa = (cr > 0) ? pts / (float)cr : 0.0f;
    acad.creditsEarned = earned;
    acad.completedMissions = done;
    acad.totalScore = total;
    acad.semester = std::min(g.currentLevel, 5);
}

static void resetAcademic()
{
    acad.totalScore = 0; acad.semester = 1; acad.completedMissions = 0; acad.creditsEarned = 0;
    acad.cgpa = 0.0f; acad.currentGrade = "-";
    for (int i = 0; i < 5; ++i) {
        acad.attempted[i] = false; acad.passed[i] = false; acad.gradePoint[i] = 0.0f;
        acad.grade[i] = "-"; acad.credits[i] = LEVEL_CREDITS[i];
        acad.bestPercent[i] = 0; acad.bestScore[i] = 0; acad.achievement[i] = false;
    }
}

static void notify(const std::string& s) { g.notifyText = s; g.notifyTimer = 3.0f; }

// ======================================================================
// SECTION 8 - SAVE / LOAD  (plain text file next to the .exe)
// ======================================================================
static const char* SAVE_FILE = "campus_quest.sav";

static void saveGame()
{
    std::ofstream f(SAVE_FILE);
    if (!f) return;
    f << "CQ3D1\n" << g.currentLevel << "\n";
    for (int i = 0; i < 5; ++i)
        f << acad.attempted[i] << " " << acad.passed[i] << " " << acad.gradePoint[i] << " "
          << acad.grade[i] << " " << acad.bestPercent[i] << " " << acad.bestScore[i] << " "
          << acad.achievement[i] << "\n";
    f << acad.currentGrade << "\n";
}

static bool loadGame()
{
    std::ifstream f(SAVE_FILE);
    if (!f) return false;
    std::string magic; int lvl = 1;
    f >> magic >> lvl;
    if (magic != "CQ3D1" || lvl < 1 || lvl > 6) return false;
    resetAcademic();
    for (int i = 0; i < 5; ++i) {
        int a, p, ach;
        f >> a >> p >> acad.gradePoint[i] >> acad.grade[i] >> acad.bestPercent[i] >> acad.bestScore[i] >> ach;
        acad.attempted[i] = a != 0; acad.passed[i] = p != 0; acad.achievement[i] = ach != 0;
    }
    f >> acad.currentGrade;
    if (!f) { resetAcademic(); return false; }
    g.currentLevel = lvl;
    calculateCGPA();
    return true;
}

// ======================================================================
// SECTION 9 - PUZZLES  (question selection, answering, timer, results)
// ======================================================================
static void spawnConfetti(int n, float cx, float cy, float cz, float spread)
{
    for (int i = 0; i < n; ++i) {
        Confetti c;
        c.x = cx + (frand01() * 2 - 1) * spread; c.y = cy + frand01() * 4.0f;
        c.z = cz + (frand01() * 2 - 1) * spread;
        c.vx = (frand01() * 2 - 1) * 1.5f; c.vy = -1.5f - frand01() * 2.0f; c.vz = (frand01() * 2 - 1) * 1.5f;
        c.r = 0.4f + frand01() * 0.6f; c.g = 0.4f + frand01() * 0.6f; c.b = 0.4f + frand01() * 0.6f;
        c.spin = frand01() * 6.0f;
        if (confetti.size() < 700) confetti.push_back(c);
    }
}

static void updateConfetti(float dt)
{
    for (size_t i = 0; i < confetti.size();) {
        Confetti& c = confetti[i];
        c.x += c.vx * dt; c.y += c.vy * dt; c.z += c.vz * dt; c.spin += dt * 5.0f;
        c.vx += std::sin(g.clock * 3.0f + c.spin) * dt;
        if (c.y < 0.05f) { confetti[i] = confetti.back(); confetti.pop_back(); }
        else ++i;
    }
}

static int pickQuestion(int cat, int lo, int hi, std::vector<int>& used)
{
    std::vector<int> cand;
    for (int pass = 0; pass < 2 && cand.empty(); ++pass)
        for (size_t i = 0; i < bank.size(); ++i) {
            if (std::find(used.begin(), used.end(), (int)i) != used.end()) continue;
            if (cat != CAT_ANY && bank[i].cat != cat) continue;
            if (pass == 0 && (bank[i].diff < lo || bank[i].diff > hi)) continue;
            cand.push_back((int)i);
        }
    int pick = cand[rand() % cand.size()];
    used.push_back(pick);
    return pick;
}

static bool diffLess(const Question& a, const Question& b) { return a.diff < b.diff; }

static void buildChallenge(int levelIdx)
{
    static const int plan[5][7] = {
        { CAT_MATH, CAT_MATH, CAT_MATH, -1, -1, -1, -1 },
        { CAT_LOGIC, CAT_LOGIC, CAT_LOGIC, CAT_LOGIC, -1, -1, -1 },
        { CAT_CODE, CAT_CODE, CAT_CODE, CAT_CODE, CAT_CODE, -1, -1 },
        { CAT_MATH, CAT_LOGIC, CAT_CODE, CAT_MATH, CAT_LOGIC, CAT_CODE, -1 },
        { CAT_MATH, CAT_LOGIC, CAT_CODE, CAT_MATH, CAT_LOGIC, CAT_CODE, CAT_ANY }
    };
    static const int lo[5] = { 1, 1, 1, 1, 2 };
    static const int hi[5] = { 1, 2, 2, 3, 3 };
    std::vector<int> used;
    puzzle.set.clear();
    int n = LEVEL_QUESTIONS[levelIdx];
    for (int i = 0; i < n; ++i) {
        int l = lo[levelIdx], h = hi[levelIdx];
        if (levelIdx == 4 && i == 6) l = 3;                 // viva question is always hard
        puzzle.set.push_back(bank[pickQuestion(plan[levelIdx][i], l, h, used)]);
    }
    // Easier questions first (level 5 keeps the viva question last)
    if (levelIdx == 4) std::stable_sort(puzzle.set.begin(), puzzle.set.begin() + 6, diffLess);
    else               std::stable_sort(puzzle.set.begin(), puzzle.set.end(), diffLess);
    puzzle.maxScore = 0;
    for (size_t i = 0; i < puzzle.set.size(); ++i) puzzle.maxScore += puzzle.set[i].diff * 10;
}

// Start (or restart) the challenge of a level (levelIdx 0..4)
static void startPuzzle(int levelIdx)
{
    g.activeLevel = levelIdx;
    buildChallenge(levelIdx);
    puzzle.index = 0; puzzle.challengeScore = 0; puzzle.correctCount = 0;
    puzzle.qTimer = 0.0f; puzzle.hintShown = false; puzzle.hintUsed = false;
    puzzle.lastPicked = -1; puzzle.lastCorrect = false; puzzle.feedbackTimer = 0.0f; puzzle.pointsGained = 0;
    puzzle.timeLimit = LEVEL_TIME[levelIdx];
    g.timeLeft = (float)LEVEL_TIME[levelIdx] + g.coffeeBonus;
    if (g.coffeeBonus > 0.0f) notify("Coffee boost: +" + itos((int)g.coffeeBonus) + " seconds!");
    g.coffeeBonus = 0.0f;
    g.state = PUZZLE;
}

static bool flashActive()
{
    return puzzle.index < (int)puzzle.set.size() && !puzzle.set[puzzle.index].flash.empty() && puzzle.qTimer < 4.0f;
}

static void failLevel(int reason)
{
    g.lastFailReason = reason;
    g.lastFailLevel = g.activeLevel;
    g.state = LEVEL_FAILED;
}

static void completeLevel(int pct)
{
    int L = g.activeLevel;
    acad.attempted[L] = true; acad.passed[L] = true;
    gradeFromPercent(pct, acad.grade[L], acad.gradePoint[L]);
    acad.currentGrade = acad.grade[L];
    acad.bestPercent[L] = pct;
    acad.bestScore[L] = std::max(acad.bestScore[L], puzzle.challengeScore);
    acad.achievement[L] = (L <= 3);     // Math Master, Logic Explorer, Coding Champion, Exam Survivor
    if (g.currentLevel == L + 1) g.currentLevel = L + 2;
    g.lastFailLevel = -1;
    calculateCGPA();
    static const char* UNLOCK[5] = { "Central Library", "CSE Laboratory", "Examination Hall",
                                     "Project Laboratory", "Graduation Auditorium" };
    g.unlockText = std::string("NEW AREA UNLOCKED: ") + UNLOCK[L];
    g.unlockTimer = 5.0f;
    g.state = LEVEL_COMPLETE;
    spawnConfetti(120, player.x, 12.0f, player.z, 10.0f);
    saveGame();
}

// Called after the last question has been answered
static void finishChallenge()
{
    int L = g.activeLevel;
    int pct = puzzle.maxScore > 0 ? (puzzle.challengeScore * 100) / puzzle.maxScore : 0;
    std::string gr; float gp;
    gradeFromPercent(pct, gr, gp);
    if (gp > 0.0f) {                               // passed (C or better)
        completeLevel(pct);
    } else {                                       // failed module is recorded as F and must be retaken
        acad.attempted[L] = true; acad.passed[L] = false;
        acad.grade[L] = "F"; acad.gradePoint[L] = 0.0f; acad.bestPercent[L] = pct;
        acad.bestScore[L] = std::max(acad.bestScore[L], puzzle.challengeScore);
        acad.currentGrade = "F";
        calculateCGPA();
        failLevel(0);
        saveGame();
    }
}

static void nextQuestion()
{
    ++puzzle.index;
    if (puzzle.index >= (int)puzzle.set.size()) { finishChallenge(); return; }
    puzzle.qTimer = 0.0f; puzzle.hintShown = false; puzzle.lastPicked = -1;
    g.state = PUZZLE;
}

// choice 0..3
static void submitAnswer(int choice)
{
    if (g.state != PUZZLE || flashActive()) return;
    const Question& q = puzzle.set[puzzle.index];
    puzzle.lastPicked = choice;
    puzzle.lastCorrect = (choice == q.correct);
    if (puzzle.lastCorrect) {
        puzzle.pointsGained = q.diff * 10;
        puzzle.challengeScore += puzzle.pointsGained;
        ++puzzle.correctCount;
    } else {
        puzzle.pointsGained = -std::min(5, puzzle.challengeScore);
        puzzle.challengeScore = std::max(0, puzzle.challengeScore - 5);
    }
    puzzle.feedbackTimer = 0.0f;
    g.state = QUESTION_FEEDBACK;
}

static void useHint()
{
    if (g.state != PUZZLE || puzzle.hintShown || flashActive()) return;
    puzzle.hintShown = true;
    puzzle.hintUsed = true;
    puzzle.challengeScore = std::max(0, puzzle.challengeScore - 5);
}

static void updateTimer(float dt)
{
    if (g.state != PUZZLE) return;            // timer only runs while answering
    puzzle.qTimer += dt;
    g.timeLeft -= dt;
    if (g.timeLeft <= 0.0f) { g.timeLeft = 0.0f; failLevel(1); }
}

// ======================================================================
// SECTION 10 - PLAYER, COLLISION, CAMERA
// ======================================================================
static float groundHeightAt(float x, float z)
{
    const Building& a = g.buildings[B_AUDITORIUM];
    float zb = a.z - a.d * 0.5f;
    if (std::fabs(x - a.x) < a.w * 0.5f - 1.5f && z > zb && z < zb + 4.0f) return 0.85f;
    return 0.0f;
}

static bool circleRectHit(float x, float z, float rad, const Rect& r)
{
    float cx = clampf(x, r.x0, r.x1), cz = clampf(z, r.z0, r.z1);
    float dx = x - cx, dz = z - cz;
    return dx * dx + dz * dz < rad * rad;
}

// true when the player's circle at (x,z) would overlap something solid
static bool checkCollision(float x, float z)
{
    if (std::fabs(x) > 49.0f || std::fabs(z) > 49.0f) return true;
    for (size_t i = 0; i < walls.size(); ++i)
        if (circleRectHit(x, z, PLAYER_RADIUS, walls[i])) return true;
    for (size_t i = 0; i < circles.size(); ++i)
        if (distXZ(x, z, circles[i].x, circles[i].z) < circles[i].r + PLAYER_RADIUS) return true;
    if (player.y < 0.9f)                                   // benches can be jumped over
        for (size_t i = 0; i < benches.size(); ++i)
            if (distXZ(x, z, benches[i].x, benches[i].z) < 1.2f) return true;
    for (int i = 0; i < B_COUNT; ++i)
        if (isLocked(i) && circleRectHit(x, z, PLAYER_RADIUS, barrierRect(i))) return true;
    for (size_t i = 0; i < g.npcs.size(); ++i)
        if (distXZ(x, z, g.npcs[i].x, g.npcs[i].z) < 0.6f + PLAYER_RADIUS) return true;
    return false;
}

static void updatePlayer(float dt)
{
    // movement relative to the camera
    float fx = -std::sin(cam.yaw * DEG2RAD), fz = -std::cos(cam.yaw * DEG2RAD);
    float rx = -fz, rz = fx;                   // right vector
    float mx = 0, mz = 0;
    if (g.state == EXPLORING) {
        if (g.keys['w']) { mx += fx; mz += fz; }
        if (g.keys['s']) { mx -= fx; mz -= fz; }
        if (g.keys['d']) { mx += rx; mz += rz; }
        if (g.keys['a']) { mx -= rx; mz -= rz; }
    }
    float len = std::sqrt(mx * mx + mz * mz);
    player.moving = len > 0.001f;
    if (player.moving) {
        mx /= len; mz /= len;
        float speed = 9.0f, step = speed * dt;
        float nx = player.x + mx * step, nz = player.z + mz * step;
        if (!checkCollision(nx, player.z)) player.x = nx;
        if (!checkCollision(player.x, nz)) player.z = nz;
        float target = std::atan2(mx, mz) / DEG2RAD;       // heading: forward = (sin h, cos h)
        float diff = target - player.heading;
        while (diff > 180.0f) diff -= 360.0f;
        while (diff < -180.0f) diff += 360.0f;
        player.heading += diff * std::min(1.0f, dt * 14.0f);
        player.walkPhase += dt * 12.0f;
    } else player.walkPhase = 0.0f;

    // jumping and gravity
    float gy = groundHeightAt(player.x, player.z);
    player.vy -= 24.0f * dt;
    player.y += player.vy * dt;
    if (player.y <= gy) { player.y = gy; player.vy = 0.0f; player.onGround = true; }
    else player.onGround = false;
}

static float lookX = 0, lookY = 0, lookZ = 0;   // camera look-at target

static void updateCamera(float dt)
{
    float tx, ty, tz, desX, desY, desZ;
    if (g.state == MAIN_MENU) {                       // attract mode: slow orbit above the campus
        cam.yaw += dt * 8.0f;
        float a = cam.yaw * DEG2RAD;
        tx = 0; ty = 2; tz = 0;
        desX = std::sin(a) * 78.0f; desY = 34.0f; desZ = std::cos(a) * 78.0f;
    } else {
        if (g.state == GRADUATION || g.state == GAME_COMPLETE) cam.yaw += dt * 15.0f;
        float p = cam.pitch * DEG2RAD, a = cam.yaw * DEG2RAD;
        tx = player.x; ty = player.y + 1.8f; tz = player.z;
        desX = tx + std::sin(a) * std::cos(p) * cam.dist;
        desY = ty + std::sin(p) * cam.dist;
        desZ = tz + std::cos(a) * std::cos(p) * cam.dist;
        if (desY < 0.6f) desY = 0.6f;
    }
    if (!cam.initialised) { cam.cx = desX; cam.cy = desY; cam.cz = desZ; cam.initialised = true; }
    float k = std::min(1.0f, dt * 7.0f);
    cam.cx += (desX - cam.cx) * k; cam.cy += (desY - cam.cy) * k; cam.cz += (desZ - cam.cz) * k;
    // look-at target is stored in globals used by the display function
    lookX = tx; lookY = ty; lookZ = tz;
}

// ======================================================================
// SECTION 11 - TEACHERS, DIALOGUE AND INTERACTION
// ======================================================================
static std::string promptText, promptSub;

static int levelOfBuilding(int b)
{
    for (int l = 0; l < 5; ++l) if (LEVEL_BUILDING[l] == b) return l;
    return -1;
}

static const char* TIPS[5] = {
    "Tip: to solve 3x - 5 = 16, undo the steps in reverse order: add 5, then divide by 3.",
    "Tip: compare neighbours in a pattern. Look for differences, doubling, or alternating items.",
    "Tip: trace code line by line and write down every variable. A loop 'i < 5' stops when i becomes 5.",
    "Tip: a wrong answer costs 5 points, so only guess when you have narrowed the choices down.",
    "Tip: this project mixes maths, logic and code. The last question is your viva - stay calm!"
};
static const char* PRAISE[5] = {
    "Excellent work, freshman! You have the numbers on your side. The Library is now open to you.",
    "Brilliant detective work! Your logic is sharp. The CSE Laboratory is waiting for you.",
    "Well compiled! You think like a real programmer. Time to face the Examination Hall.",
    "You survived the finals! Now the Project Laboratory is ready for your final year project.",
    "Outstanding project and viva! The Graduation Auditorium is open - walk onto the stage!"
};

static void openDialogue(int ni)
{
    const NPC& n = g.npcs[ni];
    g.dialogueName = n.name;
    g.dialogueAfter = 0;
    int b = n.building, lv = levelOfBuilding(b);
    std::string t;
    if (b == B_ACADEMIC) {
        if (g.currentLevel == 1)
            t = "Welcome to Campus Quest University! Move with W/A/S/D, jump with SPACE, and press F near teachers "
                "and doors. Your first mission: go to the Mathematics Building and solve 3 problems before the "
                "timer runs out. Press H during a puzzle for a hint (costs 5 points).";
        else if (g.currentLevel <= 5)
            t = "Keep going! Your CGPA is " + ftos2(acad.cgpa) + " with " + itos(acad.creditsEarned) +
                " credits earned. Next stop: " + g.buildings[LEVEL_BUILDING[g.currentLevel - 1]].name + ".";
        else
            t = "You did it! Every module is complete. Head to the Graduation Auditorium and step onto the stage.";
    } else if (b == B_CAFE) {
        if (g.currentLevel <= 5 && g.coffeeBonus <= 0.0f) {
            g.coffeeBonus = 15.0f;
            t = "Fresh coffee, on the house! You will get +15 seconds on your next challenge. Study hard!";
        } else if (g.currentLevel <= 5)
            t = "You already have a coffee waiting for your next challenge. Go and use it!";
        else
            t = "Congratulations, almost-graduate! The first coffee as a graduate is on me.";
    } else if (b == B_AUDITORIUM) {
        if (g.currentLevel >= 6)
            t = "Welcome, future graduate! Step onto the glowing mark on the stage and we will begin the ceremony.";
        else
            t = "The ceremony is only for students who passed all five modules. Keep working - I will be here.";
    } else {
        if (g.currentLevel < lv + 1) {
            t = "This module is not open to you yet. Finish Level " + itos(g.currentLevel) +
                " first, then come back to me.";
        } else if (g.currentLevel == lv + 1) {
            if (g.lastFailLevel == lv)
                t = "Don't give up - every student fails once. Use the hint key (H) wisely, read each question slowly, and try again!";
            else
                t = "Level " + itos(lv + 1) + ": " + LEVEL_NAMES[lv] + ". Solve " + itos(LEVEL_QUESTIONS[lv]) +
                    " questions in " + itos(LEVEL_TIME[lv]) + " seconds. Press 1-4 to answer. " + TIPS[lv];
            g.dialogueAfter = 1;
        } else {
            t = PRAISE[lv];
        }
    }
    g.dialogueText = t;
    g.state = DIALOGUE;
}

struct Interact { int type; int idx; };   // type: 0 none, 1 teacher, 2 building door

static Interact findInteraction()
{
    Interact it = { 0, -1 };
    float best = 3.6f;
    for (size_t i = 0; i < g.npcs.size(); ++i) {
        float d = distXZ(player.x, player.z, g.npcs[i].x, g.npcs[i].z);
        if (d < best) { best = d; it.type = 1; it.idx = (int)i; }
    }
    if (it.type) return it;
    best = 4.2f;
    for (int i = 0; i < B_COUNT; ++i) {
        if (i == B_AUDITORIUM) continue;
        float d = distXZ(player.x, player.z, g.buildings[i].doorX, g.buildings[i].doorZ);
        if (d < best) { best = d; it.type = 2; it.idx = i; }
    }
    return it;
}

// Called every frame while exploring: sets the on-screen prompt, detects the graduation stage.
static void checkInteraction()
{
    promptText.clear(); promptSub.clear();
    Interact it = findInteraction();
    if (it.type == 1) {
        promptText = "Press F to interact"; promptSub = g.npcs[it.idx].name;
    } else if (it.type == 2) {
        if (isLocked(it.idx)) {
            promptText = "LOCKED - finish Level " + itos(g.buildings[it.idx].unlockLevel - 1) + " first";
            promptSub = g.buildings[it.idx].name;
        } else {
            promptText = "Press F to enter"; promptSub = g.buildings[it.idx].name;
        }
    }
    // graduation stage: walk onto the glowing mark once all five levels are done
    if (g.currentLevel >= 6 && distXZ(player.x, player.z, 0.0f, -45.3f) < 1.8f && player.y > 0.5f) {
        g.state = GRADUATION;
        g.certTimer = 0.0f;
        acad.achievement[4] = true;
        calculateCGPA();
        spawnConfetti(300, 0.0f, 14.0f, -42.0f, 14.0f);
        saveGame();
    }
}

static void interact()
{
    Interact it = findInteraction();
    if (it.type == 1) { openDialogue(it.idx); return; }
    if (it.type != 2) return;
    int i = it.idx;
    if (isLocked(i)) { notify("LOCKED - complete Level " + itos(g.buildings[i].unlockLevel - 1) + " first"); return; }
    int lv = levelOfBuilding(i);
    if (i == B_ACADEMIC || i == B_CAFE) { openDialogue(i); return; }
    if (lv >= 0) {
        if (g.currentLevel == lv + 1) startPuzzle(lv);
        else notify(std::string("Module already passed - grade ") + acad.grade[lv]);
    }
}

// ======================================================================
// SECTION 12 - 2D USER INTERFACE
// ======================================================================
static void* F_SMALL = GLUT_BITMAP_HELVETICA_12;
static void* F_MED   = GLUT_BITMAP_HELVETICA_18;
static void* F_BIG   = GLUT_BITMAP_TIMES_ROMAN_24;

static void strokeText(float cx, float y, const std::string& s, float scale, float r, float gg, float b, float lw)
{
    float w = 0;
    for (size_t i = 0; i < s.size(); ++i) w += (float)glutStrokeWidth(GLUT_STROKE_ROMAN, (unsigned char)s[i]);
    w *= scale;
    glLineWidth(lw);
    glColor3f(r, gg, b);
    glPushMatrix();
    glTranslatef(cx - w * 0.5f, y, 0);
    glScalef(scale, scale, 1.0f);
    for (size_t i = 0; i < s.size(); ++i) glutStrokeCharacter(GLUT_STROKE_ROMAN, (unsigned char)s[i]);
    glPopMatrix();
    glLineWidth(1.0f);
}

static void panel(float x, float y, float w, float h, float r, float gg, float b, float a)
{
    rect2D(x, y, w, h, r, gg, b, a);
    frame2D(x, y, w, h, 1.0f, 1.0f, 1.0f, 2.0f);
}

static std::string objectiveText()
{
    if (g.currentLevel > 5) return "FINAL GOAL: walk onto the glowing stage in the Graduation Auditorium!";
    int l = g.currentLevel - 1;
    return "Level " + itos(l + 1) + " - " + LEVEL_NAMES[l] + ": go to the " + g.buildings[LEVEL_BUILDING[l]].name + " and press F";
}

static void drawMap(float px, float py, float size, bool big)
{
    const float R = 52.0f;
    panel(px - 4, py - 4, size + 8, size + 8, 0.05f, 0.1f, 0.05f, 0.85f);
    rect2D(px, py, size, size, 0.30f, 0.62f, 0.30f, 1.0f);
    #define MAPX(wx) (px + ((wx) + R) / (2 * R) * size)
    #define MAPY(wz) (py + (1.0f - ((wz) + R) / (2 * R)) * size)
    // roads
    rect2D(MAPX(-3.0f), MAPY(50.0f), MAPX(3.0f) - MAPX(-3.0f), MAPY(-38.0f) - MAPY(50.0f), 0.35f, 0.35f, 0.38f, 1);
    rect2D(MAPX(-40.0f), MAPY(3.0f), MAPX(40.0f) - MAPX(-40.0f), MAPY(-3.0f) - MAPY(3.0f), 0.35f, 0.35f, 0.38f, 1);
    int target = (g.currentLevel <= 5) ? LEVEL_BUILDING[g.currentLevel - 1] : B_AUDITORIUM;
    for (int i = 0; i < B_COUNT; ++i) {
        const Building& b = g.buildings[i];
        float x0 = MAPX(b.x - b.w * 0.5f), x1 = MAPX(b.x + b.w * 0.5f);
        float y0 = MAPY(b.z + b.d * 0.5f), y1 = MAPY(b.z - b.d * 0.5f);
        bool lk = isLocked(i);
        rect2D(x0, y0, x1 - x0, y1 - y0, lk ? 0.45f : b.r, lk ? 0.45f : b.g, lk ? 0.45f : b.b, 1);
        if (i == target && (int)(g.clock * 3.0f) % 2 == 0)
            frame2D(x0 - 2, y0 - 2, x1 - x0 + 4, y1 - y0 + 4, 1.0f, 0.9f, 0.1f, 3.0f);
        if (big) {
            std::string nm = b.name;
            textCentered((x0 + x1) * 0.5f, (y0 + y1) * 0.5f - 4, nm.substr(0, nm.find(' ')), F_SMALL, 1, 1, 1);
        }
    }
    for (size_t i = 0; i < g.npcs.size(); ++i) {
        float nx = MAPX(g.npcs[i].x), ny = MAPY(g.npcs[i].z);
        rect2D(nx - 2, ny - 2, 4, 4, 1.0f, 0.9f, 0.2f, 1.0f);
    }
    // player arrow
    float hx = std::sin(player.heading * DEG2RAD), hy = -std::cos(player.heading * DEG2RAD);
    float cx = MAPX(player.x), cy = MAPY(player.z), s = big ? 12.0f : 8.0f;
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx + hx * s, cy + hy * s);
    glVertex2f(cx - hx * s * 0.6f - hy * s * 0.6f, cy - hy * s * 0.6f + hx * s * 0.6f);
    glVertex2f(cx - hx * s * 0.6f + hy * s * 0.6f, cy - hy * s * 0.6f - hx * s * 0.6f);
    glEnd();
    #undef MAPX
    #undef MAPY
    if (!big) text2D(px + 4, py + size + 8, "MAP (M = big map)", F_SMALL, 1, 1, 1);
}

static void drawHUD()
{
    float w = (float)g.winW, h = (float)g.winH;
    // --- academic panel (top-left)
    panel(10, h - 150, 255, 140, 0.05f, 0.08f, 0.2f, 0.72f);
    text2D(20, h - 32, "SCORE " + itos(acad.totalScore) + "   SEMESTER " + itos(acad.semester), F_MED, 1, 1, 0.5f);
    text2D(20, h - 56, "MISSIONS " + itos(acad.completedMissions) + "/5   CREDITS " + itos(acad.creditsEarned), F_SMALL, 1, 1, 1);
    text2D(20, h - 78, "CGPA " + ftos2(acad.cgpa) + "   GRADE " + acad.currentGrade, F_MED, 0.6f, 1, 0.6f);
    int lvShow = std::min(g.currentLevel, 5);
    text2D(20, h - 102, "LEVEL " + itos(lvShow) + (g.currentLevel > 5 ? " (all done)" : ""), F_SMALL, 1, 1, 1);
    text2D(20, h - 124, g.currentLevel > 5 ? "Graduation ready!" : std::string(LEVEL_NAMES[lvShow - 1]), F_SMALL, 0.8f, 0.9f, 1);
    // --- objective bar (top centre)
    std::string obj = objectiveText();
    float ow = std::min(w - 560.0f, 760.0f);
    panel(w * 0.5f - ow * 0.5f + 20, h - 62, ow, 52, 0.0f, 0.0f, 0.0f, 0.55f);
    paragraph(w * 0.5f - ow * 0.5f + 32, h - 30, "OBJECTIVE: " + obj, F_SMALL, ow - 24, 16, 1.0f, 0.9f, 0.4f);
    // --- distance + compass arrow to objective
    int target = (g.currentLevel <= 5) ? LEVEL_BUILDING[g.currentLevel - 1] : B_AUDITORIUM;
    const Building& tb = g.buildings[target];
    float dx = tb.doorX - player.x, dz = tb.doorZ - player.z;
    float ang = (std::atan2(dx, dz) / DEG2RAD) - (cam.yaw + 180.0f);   // relative to what the camera sees
    float ca = ang * DEG2RAD;
    float ax = w - 60, ay = h - 70;
    rect2D(ax - 36, ay - 36, 72, 72, 0, 0, 0, 0.5f);
    glColor3f(1.0f, 0.85f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(ax + std::sin(ca) * 28, ay + std::cos(ca) * 28);
    glVertex2f(ax + std::sin(ca + 2.5f) * 18, ay + std::cos(ca + 2.5f) * 18);
    glVertex2f(ax + std::sin(ca - 2.5f) * 18, ay + std::cos(ca - 2.5f) * 18);
    glEnd();
    textCentered(ax, ay - 52, itos((int)std::sqrt(dx * dx + dz * dz)) + " m", F_SMALL, 1, 1, 1);
    // --- minimap
    drawMap(w - 190, 20, 170, false);
    // --- controls help
    text2D(10, 10, "WASD move | SPACE jump | F interact | Arrows/Mouse camera | +/- zoom | M map | P pause | ESC save+exit", F_SMALL, 1, 1, 1);
    // --- interaction prompt
    if (g.state == EXPLORING && !promptText.empty()) {
        float pw = 420;
        panel(w * 0.5f - pw * 0.5f, 90, pw, 62, 0.0f, 0.0f, 0.0f, 0.7f);
        textCentered(w * 0.5f, 128, promptText, F_MED, 1, 1, 0.4f);
        textCentered(w * 0.5f, 104, promptSub, F_SMALL, 1, 1, 1);
    }
    // --- notifications
    if (g.notifyTimer > 0.0f) {
        float a = std::min(1.0f, g.notifyTimer);
        panel(w * 0.5f - 250, h * 0.5f + 140, 500, 40, 0.1f, 0.1f, 0.4f, 0.8f * a);
        textCentered(w * 0.5f, h * 0.5f + 153, g.notifyText, F_MED, 1, 1, 1);
    }
    if (g.unlockTimer > 0.0f && g.state == EXPLORING) {
        float a = std::min(1.0f, g.unlockTimer);
        panel(w * 0.5f - 280, h * 0.5f + 190, 560, 44, 0.0f, 0.4f, 0.1f, 0.85f * a);
        textCentered(w * 0.5f, h * 0.5f + 205, g.unlockText, F_MED, 1, 1, 0.6f);
    }
}

static void drawDialogue()
{
    float w = (float)g.winW;
    float bw = std::min(w - 80.0f, 860.0f), bx = w * 0.5f - bw * 0.5f;
    panel(bx, 30, bw, 190, 0.05f, 0.08f, 0.25f, 0.92f);
    rect2D(bx, 190, 260, 30, 0.9f, 0.7f, 0.1f, 1.0f);
    text2D(bx + 12, 198, g.dialogueName, F_MED, 0.1f, 0.1f, 0.2f);
    paragraph(bx + 20, 165, g.dialogueText, F_MED, bw - 40, 24, 1, 1, 1);
    text2D(bx + bw - 330, 42, g.dialogueAfter ? "Press F to begin the challenge" : "Press F to continue", F_SMALL, 1, 0.9f, 0.4f);
}

static void drawPuzzle(bool feedback)
{
    float w = (float)g.winW, h = (float)g.winH;
    rect2D(0, 0, w, h, 0.0f, 0.0f, 0.1f, 0.6f);
    float pw = std::min(w - 40.0f, 900.0f), ph = 560.0f;
    float px = w * 0.5f - pw * 0.5f, py = std::max(5.0f, (h - ph) * 0.5f), top = py + ph;
    panel(px, py, pw, ph, 0.07f, 0.10f, 0.28f, 0.95f);
    const Question& q = puzzle.set[std::min(puzzle.index, (int)puzzle.set.size() - 1)];
    static const char* CATN[3] = { "MATHEMATICS", "LOGIC & PATTERNS", "PROGRAMMING" };
    std::string stage = "Question " + itos(puzzle.index + 1) + "/" + itos((int)puzzle.set.size());
    if (g.activeLevel == 4 && puzzle.index == 6) stage = "FINAL VIVA QUESTION";
    static const char* DN[4] = { "", "EASY", "MEDIUM", "HARD" };
    text2D(px + 24, top - 34, "LEVEL " + itos(g.activeLevel + 1) + " - " + LEVEL_NAMES[g.activeLevel], F_MED, 1, 0.9f, 0.4f);
    text2D(px + pw - 24 - textWidth(stage, F_MED), top - 34, stage, F_MED, 1, 1, 1);
    text2D(px + 24, top - 58, std::string(CATN[q.cat]) + "  |  " + DN[q.diff] + "  |  worth " + itos(q.diff * 10) + " pts", F_SMALL, 0.7f, 0.85f, 1);
    // timer bar
    float frac = clampf(g.timeLeft / std::max((float)puzzle.timeLimit, g.timeLeft), 0.0f, 1.0f);
    bool low = g.timeLeft < 15.0f;
    rect2D(px + 24, top - 86, pw - 48, 14, 0.2f, 0.2f, 0.25f, 1);
    rect2D(px + 24, top - 86, (pw - 48) * frac, 14, low ? 0.95f : 0.2f, low ? 0.2f : 0.8f, 0.2f, 1);
    std::string tm = "TIME " + itos((int)std::ceil(g.timeLeft)) + "s";
    text2D(px + pw - 24 - textWidth(tm, F_MED), top - 112, tm, F_MED, low ? 1.0f : 0.9f, low ? 0.4f : 0.9f, low ? 0.4f : 0.9f);
    text2D(px + 24, top - 112, "CHALLENGE SCORE " + itos(puzzle.challengeScore) + " / " + itos(puzzle.maxScore), F_MED, 0.6f, 1, 0.6f);

    if (!feedback && flashActive()) {
        textCentered(w * 0.5f, top - 200, "MEMORISE THIS!   (" + itos((int)std::ceil(4.0f - puzzle.qTimer)) + ")", F_MED, 1, 0.9f, 0.4f);
        strokeText(w * 0.5f, top - 290, q.flash, 0.28f, 1.0f, 1.0f, 1.0f, 3.0f);
        return;
    }
    textLines(w * 0.5f, top - 165, q.text, F_BIG, 32, 1, 1, 1);
    // answer boxes
    static const char* KEYS[4] = { "1", "2", "3", "4" };
    for (int i = 0; i < 4; ++i) {
        float by = top - 330 - i * 60;
        float r = 0.15f, gg = 0.22f, b = 0.5f;
        if (feedback) {
            if (i == q.correct) { r = 0.1f; gg = 0.6f; b = 0.2f; }
            else if (i == puzzle.lastPicked) { r = 0.75f; gg = 0.1f; b = 0.1f; }
        }
        rect2D(px + 60, by, pw - 120, 48, r, gg, b, 1.0f);
        frame2D(px + 60, by, pw - 120, 48, 0.8f, 0.8f, 1.0f, 1.0f);
        text2D(px + 78, by + 16, std::string(KEYS[i]) + "  -  " + q.opt[i], F_BIG, 1, 1, 1);
    }
    if (!feedback) {
        if (puzzle.hintShown) paragraph(px + 40, top - 232, "HINT: " + q.hint, F_MED, pw - 80, 22, 1.0f, 0.9f, 0.3f);
        else text2D(px + 40, top - 232, "Press H for a hint (-5 points)   |   R restart challenge   |   P pause", F_SMALL, 0.7f, 0.8f, 1);
    } else {
        // visual feedback + floating score
        float t = puzzle.feedbackTimer;
        if (puzzle.lastCorrect) {
            textCentered(w * 0.5f, top - 232, "CORRECT!", F_BIG, 0.3f, 1.0f, 0.4f);
            strokeText(w * 0.5f + 150, top - 215 + t * 60.0f, "+" + itos(puzzle.pointsGained), 0.25f, 0.3f, 1.0f, 0.4f, 3.0f);
        } else {
            textCentered(w * 0.5f, top - 232, "WRONG!  Correct answer: " + itos(q.correct + 1), F_BIG, 1.0f, 0.3f, 0.3f);
            if (puzzle.pointsGained < 0)
                strokeText(w * 0.5f + 230, top - 215 + t * 60.0f, itos(puzzle.pointsGained), 0.25f, 1.0f, 0.3f, 0.3f, 3.0f);
        }
        text2D(px + pw - 330, py + 14, "Press ENTER for the next question", F_SMALL, 1, 0.9f, 0.4f);
    }
}

static void drawEndPanel(bool success)
{
    float w = (float)g.winW, h = (float)g.winH;
    rect2D(0, 0, w, h, 0, 0, 0, 0.55f);
    float pw = 700, ph = 420, px = w * 0.5f - pw * 0.5f, py = h * 0.5f - ph * 0.5f;
    panel(px, py, pw, ph, success ? 0.05f : 0.25f, success ? 0.25f : 0.05f, 0.1f, 0.95f);
    int L = g.activeLevel;
    if (success) {
        strokeText(w * 0.5f, py + ph - 70, "MISSION COMPLETE!", 0.30f, 1.0f, 0.9f, 0.3f, 3.0f);
        textCentered(w * 0.5f, py + ph - 105, std::string("Level ") + itos(L + 1) + " - " + LEVEL_NAMES[L], F_MED, 1, 1, 1);
        int pct = acad.bestPercent[L];
        textCentered(w * 0.5f, py + ph - 145, "Result: " + itos(pct) + "%   Grade " + acad.grade[L] + "   (" + ftos2(acad.gradePoint[L]) + " grade points)", F_BIG, 0.6f, 1, 0.6f);
        textCentered(w * 0.5f, py + ph - 180, "Credits +" + itos(LEVEL_CREDITS[L]) + "   Total credits " + itos(acad.creditsEarned) + "   CGPA " + ftos2(acad.cgpa), F_MED, 1, 1, 1);
        textCentered(w * 0.5f, py + ph - 212, "Challenge score " + itos(puzzle.challengeScore) + " / " + itos(puzzle.maxScore) + "   Correct " + itos(puzzle.correctCount) + "/" + itos((int)puzzle.set.size()), F_SMALL, 0.8f, 0.9f, 1);
        if (acad.achievement[L < 4 ? L : 3] && L < 4)
            textCentered(w * 0.5f, py + ph - 245, std::string("ACHIEVEMENT UNLOCKED: ") + ACH_NAMES[L], F_MED, 1, 0.8f, 0.2f);
        textCentered(w * 0.5f, py + ph - 285, g.unlockText, F_MED, 0.5f, 1, 0.6f);
        paragraph(px + 40, py + 105, std::string(g.npcs[LEVEL_BUILDING[L]].name) + ": \"" + PRAISE[L] + "\"", F_SMALL, pw - 80, 18, 0.9f, 0.9f, 1);
        textCentered(w * 0.5f, py + 28, "Press ENTER to return to the campus", F_MED, 1, 0.9f, 0.4f);
    } else {
        bool tu = (g.lastFailReason == 1);
        strokeText(w * 0.5f, py + ph - 70, tu ? "TIME UP!" : "MODULE FAILED", 0.34f, 1.0f, 0.4f, 0.4f, 3.0f);
        textCentered(w * 0.5f, py + ph - 110, std::string("Level ") + itos(L + 1) + " - " + LEVEL_NAMES[L], F_MED, 1, 1, 1);
        if (tu) textCentered(w * 0.5f, py + ph - 150, "You ran out of time. Score so far: " + itos(puzzle.challengeScore) + " / " + itos(puzzle.maxScore), F_BIG, 1, 0.8f, 0.8f);
        else textCentered(w * 0.5f, py + ph - 150, "Result: " + itos(acad.bestPercent[L]) + "%  - Grade F. This module must be retaken to graduate.", F_BIG, 1, 0.8f, 0.8f);
        paragraph(px + 40, py + ph - 200, std::string(g.npcs[LEVEL_BUILDING[L]].name) +
                  ": \"Don't be discouraged. Use hints wisely, read each question slowly, and try again - you can do this!\"", F_MED, pw - 80, 24, 0.9f, 0.9f, 1);
        textCentered(w * 0.5f, py + 80, "R or ENTER = retry the challenge", F_MED, 1, 0.9f, 0.4f);
        textCentered(w * 0.5f, py + 48, "C = return to the campus", F_MED, 1, 1, 1);
    }
}

static void drawPause()
{
    float w = (float)g.winW, h = (float)g.winH;
    rect2D(0, 0, w, h, 0, 0, 0.1f, 0.75f);
    strokeText(w * 0.5f, h * 0.5f + 60, "PAUSED", 0.4f, 1, 1, 1, 3.0f);
    textCentered(w * 0.5f, h * 0.5f, "P = resume     V = save game     ESC = save and exit", F_MED, 1, 0.9f, 0.4f);
    textCentered(w * 0.5f, h * 0.5f - 30, "The timer is stopped while the game is paused.", F_SMALL, 0.8f, 0.9f, 1);
}

static void drawMainMenu()
{
    float w = (float)g.winW, h = (float)g.winH;
    rect2D(0, 0, w, h, 0.0f, 0.0f, 0.15f, 0.45f);
    strokeText(w * 0.5f, h - 150, "CAMPUS QUEST 3D", 0.62f, 1.0f, 0.88f, 0.25f, 5.0f);
    textCentered(w * 0.5f, h - 185, "Journey to Graduation", F_BIG, 1, 1, 1);
    const char* items[3] = { "NEW GAME", "CONTINUE", "QUIT" };
    for (int i = 0; i < 3; ++i) {
        bool dis = (i == 1 && !g.hasSave), sel = (i == g.menuSel);
        float by = h * 0.5f - 10 - i * 56;
        rect2D(w * 0.5f - 170, by, 340, 44, sel ? 0.9f : 0.1f, sel ? 0.6f : 0.15f, sel ? 0.1f : 0.35f, 0.9f);
        frame2D(w * 0.5f - 170, by, 340, 44, 1, 1, 1, 2.0f);
        textCentered(w * 0.5f, by + 14, items[i], F_BIG, dis ? 0.5f : 1.0f, dis ? 0.5f : 1.0f, dis ? 0.5f : 1.0f);
    }
    textCentered(w * 0.5f, h * 0.5f - 200, "W/S or UP/DOWN to choose, ENTER to select", F_MED, 1, 1, 0.7f);
    textCentered(w * 0.5f, 78, "Explore the campus, solve maths, logic and C++ challenges, raise your CGPA and graduate!", F_SMALL, 1, 1, 1);
    textCentered(w * 0.5f, 56, "W/A/S/D move | SPACE jump | F interact | H hint | M map | P pause | R restart challenge | +/- zoom | ESC exit", F_SMALL, 0.8f, 0.9f, 1);
}

static void drawGraduationCeremony(bool final)
{
    float w = (float)g.winW, h = (float)g.winH;
    rect2D(0, 0, w, h, 0, 0, 0, final ? 0.35f : 0.45f);
    float pw = 760, ph = 470, px = w * 0.5f - pw * 0.5f, py = h * 0.5f - ph * 0.5f + 10;
    rect2D(px, py, pw, ph, 0.98f, 0.94f, 0.80f, 0.97f);
    frame2D(px + 8, py + 8, pw - 16, ph - 16, 0.75f, 0.55f, 0.1f, 5.0f);
    frame2D(px + 16, py + 16, pw - 32, ph - 32, 0.75f, 0.55f, 0.1f, 1.5f);
    strokeText(w * 0.5f, py + ph - 80, "CERTIFICATE OF GRADUATION", 0.2f, 0.45f, 0.25f, 0.05f, 3.0f);
    textCentered(w * 0.5f, py + ph - 112, "Campus Quest University", F_BIG, 0.3f, 0.2f, 0.1f);
    textCentered(w * 0.5f, py + ph - 142, "certifies that the Student has successfully completed all five academic modules", F_MED, 0.2f, 0.2f, 0.2f);
    textCentered(w * 0.5f, py + ph - 185, "FINAL CGPA: " + ftos2(acad.cgpa) + " / 4.00", F_BIG, 0.1f, 0.4f, 0.1f);
    textCentered(w * 0.5f, py + ph - 215, "Total score: " + itos(acad.totalScore) + "     Credits earned: " + itos(acad.creditsEarned), F_MED, 0.2f, 0.2f, 0.2f);
    textCentered(w * 0.5f, py + ph - 250, "ACHIEVEMENTS", F_MED, 0.55f, 0.35f, 0.05f);
    for (int i = 0; i < 5; ++i)
        textCentered(w * 0.5f, py + ph - 275 - i * 20, std::string(acad.achievement[i] ? "[x] " : "[ ] ") + ACH_NAMES[i], F_SMALL, 0.2f, 0.2f, 0.2f);
    float pulse = 0.6f + 0.4f * std::sin(g.clock * 6.0f);
    textCentered(w * 0.5f, py + 44, "CONGRATULATIONS! YOU HAVE GRADUATED!", F_BIG, 0.75f * pulse, 0.1f, 0.1f);
    if (final) textCentered(w * 0.5f, py - 20, "R = play again      ESC = exit", F_MED, 1, 1, 0.6f);
    else       textCentered(w * 0.5f, py - 20, "Press ENTER to continue", F_MED, 1, 1, 0.6f);
}

// ======================================================================
// SECTION 13 - GAME SETUP, UPDATE LOOP, RENDER, INPUT
// ======================================================================
static bool saveExists()
{
    std::ifstream f(SAVE_FILE);
    return f.good();
}

static void placePlayerAtGate()
{
    player.x = 0.0f; player.z = 44.0f; player.y = 0.0f; player.vy = 0.0f;
    player.heading = 180.0f; player.walkPhase = 0.0f; player.moving = false; player.onGround = true;
    cam.yaw = 0.0f; cam.pitch = 24.0f; cam.dist = 11.0f; cam.initialised = false;
}

static void initializeGame(bool loadSaved)
{
    resetAcademic();
    g.currentLevel = 1; g.activeLevel = 0; g.timeLeft = 0; g.coffeeBonus = 0.0f;
    g.notifyTimer = 0; g.unlockTimer = 0; g.showMap = false; g.lastFailLevel = -1; g.lastFailReason = 0;
    g.certTimer = 0; g.dialogueAfter = 0;
    confetti.clear();
    placePlayerAtGate();
    puzzle.set.clear(); puzzle.index = 0; puzzle.challengeScore = 0; puzzle.maxScore = 0; puzzle.timeLimit = 100;
    puzzle.hintShown = false; puzzle.feedbackTimer = 0.0f; puzzle.qTimer = 0.0f; puzzle.lastPicked = -1;
    if (loadSaved && loadGame()) notify("Game loaded - welcome back!");
    else { calculateCGPA(); notify("Welcome! Talk to Prof. Orion at the Academic Building."); }
    g.state = EXPLORING;
}

static void updateGame(float dt)
{
    g.clock += dt;
    if (g.notifyTimer > 0) g.notifyTimer -= dt;
    if (g.unlockTimer > 0) g.unlockTimer -= dt;
    // camera control with arrow keys
    if (g.state != MAIN_MENU && g.state != PAUSED) {
        if (g.special[GLUT_KEY_LEFT])  cam.yaw   -= 100.0f * dt;
        if (g.special[GLUT_KEY_RIGHT]) cam.yaw   += 100.0f * dt;
        if (g.special[GLUT_KEY_UP])    cam.pitch += 45.0f * dt;
        if (g.special[GLUT_KEY_DOWN])  cam.pitch -= 45.0f * dt;
        cam.pitch = clampf(cam.pitch, 5.0f, 70.0f);
    }
    if (g.state == PAUSED) return;
    switch (g.state) {
    case EXPLORING:
        updatePlayer(dt);
        checkInteraction();
        break;
    case PUZZLE:
        updateTimer(dt);
        break;
    case QUESTION_FEEDBACK:
        puzzle.feedbackTimer += dt;
        break;
    case LEVEL_COMPLETE: case GRADUATION: case GAME_COMPLETE:
        updatePlayer(dt);                       // keeps gravity / animation consistent
        if (g.state != LEVEL_COMPLETE) {
            g.certTimer += dt;
            if (g.clock - std::floor(g.clock) < dt * 1.5f) spawnConfetti(40, player.x, 13.0f, player.z, 14.0f);
        }
        break;
    default:
        updatePlayer(dt);
        break;
    }
    updateConfetti(dt);
    updateCamera(dt);
}

static void timerFunc(int)
{
    static int last = 0;
    int now = glutGet(GLUT_ELAPSED_TIME);
    if (last == 0) last = now;
    float dt = (now - last) / 1000.0f;
    last = now;
    if (dt > 0.05f) dt = 0.05f;
    updateGame(dt);
    glutPostRedisplay();
    glutTimerFunc(16, timerFunc, 0);
}

static void setupLights()
{
    GLfloat pos[4]  = { 0.4f, 1.0f, 0.3f, 0.0f };
    GLfloat amb[4]  = { 0.50f, 0.50f, 0.55f, 1.0f };
    GLfloat dif[4]  = { 0.85f, 0.82f, 0.75f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
}

static void display()
{
    glClearColor(0.53f, 0.79f, 0.95f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (g.winH > 0) ? (float)g.winW / (float)g.winH : 1.0f;
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(58.0, aspect, 0.5, 400.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(cam.cx, cam.cy, cam.cz, lookX, lookY, lookZ, 0, 1, 0);
    setupLights();

    drawCampus();
    drawBuildings();
    drawNPCs();
    drawPlayer();
    if (g.state == EXPLORING || g.state == DIALOGUE || g.state == MAIN_MENU) drawObjectiveMarker();
    if (g.currentLevel >= 6) {                         // glowing mark on the graduation stage
        float pulse = 0.5f + 0.5f * std::sin(g.clock * 4.0f);
        flatDisc(0.0f, -45.3f, 1.6f, 0.88f, 1.0f, 0.85f, 0.1f, 0.5f + 0.4f * pulse);
    }
    drawConfetti();

    begin2D();
    switch (g.state) {
    case MAIN_MENU: drawMainMenu(); break;
    case EXPLORING: drawHUD(); if (g.showMap) { float s = std::min(g.winW, g.winH) - 140.0f; rect2D(0, 0, g.winW, g.winH, 0, 0, 0, 0.5f); drawMap(g.winW * 0.5f - s * 0.5f, g.winH * 0.5f - s * 0.5f, s, true); } break;
    case DIALOGUE:  drawHUD(); drawDialogue(); break;
    case PUZZLE:    drawPuzzle(false); break;
    case QUESTION_FEEDBACK: drawPuzzle(true); break;
    case LEVEL_COMPLETE: drawEndPanel(true); break;
    case LEVEL_FAILED:   drawEndPanel(false); break;
    case PAUSED:    drawHUD(); drawPause(); break;
    case GRADUATION: drawGraduationCeremony(false); break;
    case GAME_COMPLETE: drawGraduationCeremony(true); break;
    }
    end2D();
    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    if (h < 1) h = 1;
    g.winW = w; g.winH = h;
    glViewport(0, 0, w, h);
}

static void quitGame()
{
    if (g.state != MAIN_MENU && g.state != GAME_COMPLETE) saveGame();
    std::exit(0);
}

static void menuSelect()
{
    if (g.menuSel == 0) initializeGame(false);
    else if (g.menuSel == 1) { if (g.hasSave) initializeGame(true); }
    else std::exit(0);
}

static void keyboardDown(unsigned char k, int, int)
{
    if (k >= 'A' && k <= 'Z') k = (unsigned char)(k + 32);
    g.keys[k] = true;
    if (k == 27) { quitGame(); return; }
    switch (g.state) {
    case MAIN_MENU:
        if (k == 'w') g.menuSel = (g.menuSel + 2) % 3;
        if (k == 's') g.menuSel = (g.menuSel + 1) % 3;
        if (k == 13 || k == ' ') menuSelect();
        break;
    case EXPLORING:
        if (k == 'f') interact();
        else if (k == ' ' && player.onGround) { player.vy = 9.0f; player.onGround = false; }
        else if (k == 'm') g.showMap = !g.showMap;
        else if (k == 'p') { g.prevState = g.state; g.state = PAUSED; }
        else if (k == '+' || k == '=') cam.dist = clampf(cam.dist - 1.0f, 4.0f, 24.0f);
        else if (k == '-' || k == '_') cam.dist = clampf(cam.dist + 1.0f, 4.0f, 24.0f);
        break;
    case DIALOGUE:
        if (k == 'f' || k == 13 || k == ' ') {
            if (g.dialogueAfter) { int lv = std::min(g.currentLevel, 5) - 1; startPuzzle(lv); }
            else g.state = EXPLORING;
        }
        break;
    case PUZZLE:
        if (k >= '1' && k <= '4') submitAnswer(k - '1');
        else if (k == 'h') useHint();
        else if (k == 'r') startPuzzle(g.activeLevel);
        else if (k == 'p') { g.prevState = g.state; g.state = PAUSED; }
        break;
    case QUESTION_FEEDBACK:
        if (k == 13 || k == ' ' || k == 'f') nextQuestion();
        break;
    case LEVEL_COMPLETE:
        if (k == 13 || k == ' ') g.state = EXPLORING;
        break;
    case LEVEL_FAILED:
        if (k == 'r' || k == 13) startPuzzle(g.activeLevel);
        else if (k == 'c') g.state = EXPLORING;
        break;
    case PAUSED:
        if (k == 'p') g.state = g.prevState;
        else if (k == 'v') { saveGame(); notify("Game saved."); }
        break;
    case GRADUATION:
        if (k == 13 || k == ' ') { g.state = GAME_COMPLETE; spawnConfetti(200, player.x, 13.0f, player.z, 14.0f); }
        break;
    case GAME_COMPLETE:
        if (k == 'r') { initializeGame(false); }
        break;
    default: break;
    }
}

static void keyboardUp(unsigned char k, int, int)
{
    if (k >= 'A' && k <= 'Z') k = (unsigned char)(k + 32);
    g.keys[k] = false;
}

static void specialDown(int k, int, int)
{
    if (k < 256) g.special[k] = true;
    if (g.state == MAIN_MENU) {
        if (k == GLUT_KEY_UP)   g.menuSel = (g.menuSel + 2) % 3;
        if (k == GLUT_KEY_DOWN) g.menuSel = (g.menuSel + 1) % 3;
    }
}
static void specialUp(int k, int, int) { if (k < 256) g.special[k] = false; }

static int mouseLastX = 0, mouseLastY = 0;
static bool mouseDrag = false;
static void mouseButton(int button, int st, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) { mouseDrag = (st == GLUT_DOWN); mouseLastX = x; mouseLastY = y; }
}
static void mouseMove(int x, int y)
{
    if (!mouseDrag || g.state == MAIN_MENU || g.state == PAUSED) return;
    cam.yaw   += (x - mouseLastX) * 0.4f;
    cam.pitch = clampf(cam.pitch + (y - mouseLastY) * 0.3f, 5.0f, 70.0f);
    mouseLastX = x; mouseLastY = y;
}

int main(int argc, char** argv)
{
    std::srand((unsigned)std::time(0));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1100, 680);
    glutInitWindowPosition(60, 40);
    glutCreateWindow("CAMPUS QUEST 3D: Journey to Graduation");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    std::memset(&g.keys, 0, sizeof(g.keys));
    std::memset(&g.special, 0, sizeof(g.special));
    g.winW = 1100; g.winH = 680; g.clock = 0.0f; g.menuSel = 0; g.state = MAIN_MENU; g.prevState = MAIN_MENU;
    g.currentLevel = 1; g.notifyTimer = 0; g.unlockTimer = 0; g.lastFailLevel = -1; g.coffeeBonus = 0;
    g.showMap = false; g.certTimer = 0;
    cam.yaw = 20.0f; cam.pitch = 24.0f; cam.dist = 11.0f; cam.initialised = false;

    buildQuestionBank();
    initializeCampus();
    resetAcademic();
    placePlayerAtGate();
    cam.yaw = 20.0f; cam.initialised = false;
    g.hasSave = saveExists();
    g.menuSel = g.hasSave ? 1 : 0;

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMove);
    glutIgnoreKeyRepeat(1);
    glutTimerFunc(16, timerFunc, 0);
    glutMainLoop();
    return 0;
}
