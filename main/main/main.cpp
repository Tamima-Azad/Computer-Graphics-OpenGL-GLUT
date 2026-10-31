#include <windows.h>
#include <GL/glut.h>
#include <stdlib.h>
#include <bits/stdc++.h>
#include <cmath>
#include <string>
#include <sstream>

using namespace std;

// Player Variables
int playerLane = 1; // 0 = Bottom Lane, 1 = Middle Lane, 2 = Top Lane
float playerX = -170.0f; // Player position on X-axis
float laneY[3] = {-60.0f, 0.0f, 60.0f};

// Game State Variables
bool gameOver = false;
int score = 0;
float roadOffset = 0.0f;

struct EnemyCar {
    float x;
    int lane;
    float speed;
    bool active;
    float r, g, b;
};

const int MAX_ENEMIES = 4;
EnemyCar enemies[MAX_ENEMIES];


void drawFilledCircle(float cx, float cy, float r)
{
    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i += 15)
    {
        float rad = i * 3.14159265f / 180.0f;
        glVertex2f(cx + r * cos(rad), cy + r * sin(rad));
    }
    glEnd();
}

// Render text on screen (C++98 compatible)
void drawText(string text, float x, float y, void* font = GLUT_BITMAP_HELVETICA_18)
{
    glRasterPos2f(x, y);
    for (size_t i = 0; i < text.length(); i++)
    {
        glutBitmapCharacter(font, text[i]);
    }
}

// Draw Car Function (Player & Enemy)
void drawCar(float cx, float cy, float r, float g, float b, bool isPlayer = false)
{
    // Lower Body Fill
    glColor3f(r, g, b);
    glBegin(GL_POLYGON);
    glVertex2d(cx - 35, cy + 0);
    glVertex2d(cx + 35, cy + 0);
    glVertex2d(cx + 35, cy + 5);
    glVertex2d(cx + 33, cy + 15);
    glVertex2d(cx - 33, cy + 15);
    glVertex2d(cx - 35, cy + 5);
    glEnd();

    // Roof Fill
    glColor3f(r * 0.7f, g * 0.7f, b * 0.7f);
    glBegin(GL_POLYGON);
    glVertex2d(cx - 33, cy + 15);
    glVertex2d(cx + 33, cy + 15);
    glVertex2d(cx + 20, cy + 18);
    glVertex2d(cx + 10, cy + 25);
    glVertex2d(cx - 10, cy + 25);
    glVertex2d(cx - 20, cy + 18);
    glEnd();

    // Glass Windows Fill
    glColor3f(0.55f, 0.85f, 0.95f);
    glBegin(GL_POLYGON);
    glVertex2d(cx - 18, cy + 18);
    glVertex2d(cx - 10, cy + 23);
    glVertex2d(cx - 2, cy + 23);
    glVertex2d(cx - 2, cy + 18);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2d(cx + 2, cy + 18);
    glVertex2d(cx + 10, cy + 23);
    glVertex2d(cx + 18, cy + 18);
    glVertex2d(cx + 2, cy + 18);
    glEnd();

    // Lights
    if (isPlayer)
    {
        glColor3f(1.0f, 0.9f, 0.0f); // Headlight facing Right
        glBegin(GL_POLYGON);
        glVertex2d(cx + 30, cy + 7);
        glVertex2d(cx + 33, cy + 7);
        glVertex2d(cx + 33, cy + 10);
        glVertex2d(cx + 30, cy + 10);
        glEnd();
    }
    else
    {
        glColor3f(1.0f, 0.9f, 0.0f); // Headlight facing Left
        glBegin(GL_POLYGON);
        glVertex2d(cx - 33, cy + 7);
        glVertex2d(cx - 30, cy + 7);
        glVertex2d(cx - 30, cy + 10);
        glVertex2d(cx - 33, cy + 10);
        glEnd();
    }

    // Wheels
    glColor3f(0.1f, 0.1f, 0.1f);
    drawFilledCircle(cx - 20, cy, 8);
    drawFilledCircle(cx + 20, cy, 8);

    glColor3f(0.8f, 0.8f, 0.8f);
    drawFilledCircle(cx - 20, cy, 4);
    drawFilledCircle(cx + 20, cy, 4);

    // Outline
    glColor3f(0, 0, 0);
    glLineWidth(1.5);
    glBegin(GL_LINE_LOOP);
    glVertex2d(cx - 35, cy + 0);
    glVertex2d(cx + 35, cy + 0);
    glVertex2d(cx + 35, cy + 5);
    glVertex2d(cx + 33, cy + 15);
    glVertex2d(cx + 20, cy + 18);
    glVertex2d(cx + 10, cy + 25);
    glVertex2d(cx - 10, cy + 25);
    glVertex2d(cx - 20, cy + 18);
    glVertex2d(cx - 33, cy + 15);
    glVertex2d(cx - 35, cy + 5);
    glEnd();
}

// Reset / Initialize Enemies
void spawnEnemy(int index)
{
    enemies[index].x = 280.0f + (rand() % 150);
    enemies[index].lane = rand() % 3;
    enemies[index].speed = 2.0f + static_cast<float>(rand() % 20) / 10.0f;
    enemies[index].active = true;

    int colorType = rand() % 4;
    if (colorType == 0) { enemies[index].r = 0.1f; enemies[index].g = 0.5f; enemies[index].b = 0.9f; }
    else if (colorType == 1) { enemies[index].r = 0.2f; enemies[index].g = 0.8f; enemies[index].b = 0.2f; }
    else if (colorType == 2) { enemies[index].r = 0.9f; enemies[index].g = 0.8f; enemies[index].b = 0.1f; }
    else { enemies[index].r = 0.7f; enemies[index].g = 0.2f; enemies[index].b = 0.8f; }
}

void resetGame()
{
    playerLane = 1;
    playerX = -170.0f;
    score = 0;
    gameOver = false;
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        spawnEnemy(i);
        enemies[i].x += i * 120;
    }
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Top Grass
    glColor3f(0.2f, 0.65f, 0.2f);
    glBegin(GL_POLYGON);
    glVertex2d(-250, 250);
    glVertex2d(250, 250);
    glVertex2d(250, 100);
    glVertex2d(-250, 100);
    glEnd();

    // Bottom Grass
    glColor3f(0.15f, 0.55f, 0.15f);
    glBegin(GL_POLYGON);
    glVertex2d(-250, -100);
    glVertex2d(250, -100);
    glVertex2d(250, -250);
    glVertex2d(-250, -250);
    glEnd();

    // 3-Lane Road
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_POLYGON);
    glVertex2d(-250, 100);
    glVertex2d(250, 100);
    glVertex2d(250, -100);
    glVertex2d(-250, -100);
    glEnd();

    // Solid Outer Lines
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(4.0);
    glBegin(GL_LINES);
    glVertex2d(-250, 100); glVertex2d(250, 100);
    glVertex2d(-250, -100); glVertex2d(250, -100);
    glEnd();

    // Dashed Moving Lane Dividers
    glLineWidth(2.5);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    for (float i = -270; i < 270; i += 40)
    {
        float startX = i - roadOffset;
        glVertex2d(startX, 30);
        glVertex2d(startX + 20, 30);

        glVertex2d(startX, -30);
        glVertex2d(startX + 20, -30);
    }
    glEnd();

    // Draw Enemies
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemies[i].active)
        {
            drawCar(enemies[i].x, laneY[enemies[i].lane], enemies[i].r, enemies[i].g, enemies[i].b, false);
        }
    }

    // Draw Player Car
    drawCar(playerX, laneY[playerLane], 0.85f, 0.12f, 0.12f, true);

    // Score & Controls Instructions UI
    glColor3f(1.0f, 1.0f, 1.0f);
    stringstream ss;
    ss << "Score: " << score;
    drawText(ss.str(), -230, 220);
    drawText("UP/DOWN: Lanes | RIGHT: Move Forward | LEFT: Brake", -180, 200, GLUT_BITMAP_HELVETICA_12);

    // Game Over Overlay
    if (gameOver)
    {
        glColor3f(1.0f, 0.0f, 0.0f);
        drawText("GAME OVER!", -60, 30, GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText("Press 'R' to Restart", -65, -10, GLUT_BITMAP_HELVETICA_18);
    }

    glFlush();
}

void timer(int val)
{
    if (!gameOver)
    {
        roadOffset += 4.0f;
        if (roadOffset >= 40.0f) roadOffset = 0.0f;

        for (int i = 0; i < MAX_ENEMIES; i++)
        {
            if (enemies[i].active)
            {
                enemies[i].x -= enemies[i].speed;

                // Collision Detection
                if (enemies[i].lane == playerLane && abs(enemies[i].x - playerX) < 60.0f)
                {
                    gameOver = true;
                }

                if (enemies[i].x < -290)
                {
                    spawnEnemy(i);
                    score += 10;
                }
            }
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void specialKeys(int key, int x, int y)
{
    if (!gameOver)
    {
        if (key == GLUT_KEY_UP && playerLane < 2)
        {
            playerLane++;
        }
        else if (key == GLUT_KEY_DOWN && playerLane > 0)
        {
            playerLane--;
        }
        else if (key == GLUT_KEY_RIGHT && playerX < 180.0f)
        {
            playerX += 10.0f; // Forward / Speed up
        }
        else if (key == GLUT_KEY_LEFT && playerX > -220.0f)
        {
            playerX -= 10.0f; // Brake / Move backward
        }
    }
}
void keyboard(unsigned char key, int x, int y)
{
        if (key == 'w' and playerLane < 2)
        {
            playerLane++;
        }
        else if (key == 's' and playerLane > 0)
        {
            playerLane--;
        }
        else if (key == 'd')
        {
            if(playerX < 180)playerX += 10.0f;
        }
        else if (key == 'a')
        {
            if(playerX >= -220)
            playerX -= 10.0f;
        }
        else if(key == 'r' or key == 'R')
            resetGame();
        glutPostRedisplay();

}

void init()
{
    glOrtho(-250.0, 250.0, -250.0, 250.0, -250.0, 250.0);
    resetGame();
    srand(time(0));
}

int main(int a, char **b)
{
    glutInit(&a, b);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(700, 700);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutCreateWindow("3-Lane Car Game with Brake & Acceleration");

    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(0, timer, 0);
    glutMainLoop();

    return 0;
}
