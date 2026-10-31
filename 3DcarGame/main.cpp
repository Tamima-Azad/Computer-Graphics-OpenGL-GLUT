
#include <windows.h>
#include <GL/glut.h>
#include <bits/stdc++.h>
using namespace std;

float carX = 0.0f;
float roadMove = 0.0f;
float speed = 0.35f;
int score = 0;
bool gameOver = false;

float obsX[3] = {-2.0f, 0.0f, 2.0f};
float obsZ[3] = {-15.0f, -30.0f, -45.0f};

void drawCube(float x, float y, float z,
              float sx, float sy, float sz,
              float r, float g, float b)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    glColor3f(r, g, b);
    glutSolidCube(1.0);
    glPopMatrix();
}

void drawWheel(float x, float y, float z)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(90, 0, 1, 0);
    glColor3f(0.08f, 0.08f, 0.08f);
    glutSolidTorus(0.10, 0.22, 12, 20);
    glPopMatrix();
}

void drawCar(float x, float z, bool player)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    if (player)
        glRotatef(0, 0, 1, 0);

    float r = player ? 0.0f : 0.9f;
    float g = player ? 0.35f : 0.1f;
    float b = player ? 1.0f : 0.1f;

    // Main body
    drawCube(0, 0.65f, 0, 1.3f, 0.45f, 2.0f,
             r, g, b);

    // Upper body
    drawCube(0, 1.02f, -0.15f, 0.85f, 0.45f, 1.0f,
             0.65f, 0.85f, 1.0f);

    // Front windshield
    drawCube(0, 1.04f, -0.68f, 0.72f, 0.30f, 0.08f,
             0.08f, 0.15f, 0.22f);

    // Headlights
    drawCube(-0.42f, 0.72f, -1.02f,
             0.22f, 0.13f, 0.06f, 1, 1, 0.5f);
    drawCube(0.42f, 0.72f, -1.02f,
             0.22f, 0.13f, 0.06f, 1, 1, 0.5f);

    // Wheels
    drawWheel(-0.68f, 0.40f, -0.62f);
    drawWheel(0.68f, 0.40f, -0.62f);
    drawWheel(-0.68f, 0.40f, 0.62f);
    drawWheel(0.68f, 0.40f, 0.62f);

    glPopMatrix();
}

void drawRoad()
{
    // Grass
    drawCube(0, -0.15f, -15, 40, 0.2f, 65,
             0.1f, 0.48f, 0.12f);

    // Asphalt road
    drawCube(0, -0.02f, -15, 7.5f, 0.15f, 65,
             0.22f, 0.22f, 0.24f);

    // Road borders
    drawCube(-3.65f, 0.08f, -15,
             0.12f, 0.04f, 65, 1, 1, 1);
    drawCube(3.65f, 0.08f, -15,
             0.12f, 0.04f, 65, 1, 1, 1);

    // Moving lane markings
    for (int i = 0; i < 15; i++)
    {
        float z = -48.0f + i * 4.0f + roadMove;

        if (z > 8.0f)
            z -= 60.0f;

        drawCube(-1.25f, 0.08f, z,
                 0.09f, 0.04f, 1.8f, 1, 1, 1);

        drawCube(1.25f, 0.08f, z,
                 0.09f, 0.04f, 1.8f, 1, 1, 1);
    }

    // Roadside trees
    for (int i = 0; i < 12; i++)
    {
        float z = -45.0f + i * 5.0f + roadMove;

        if (z > 8.0f)
            z -= 60.0f;

        for (int side = -1; side <= 1; side += 2)
        {
            float x = side * 5.0f;

            drawCube(x, 0.7f, z, 0.3f, 1.4f, 0.3f,
                     0.38f, 0.18f, 0.05f);

            glPushMatrix();
            glTranslatef(x, 1.8f, z);
            glColor3f(0.0f, 0.55f, 0.08f);
            glutSolidSphere(0.8, 12, 12);
            glPopMatrix();
        }
    }
}

void drawText(float x, float y, string text)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1, 1, 1);
    glRasterPos2f(x, y);

    for (int i = 0; i < (int)text.size(); i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,
                            text[i]);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(0, 5.5, 9,
              0, 0.5, -8,
              0, 1, 0);

    drawRoad();

    // Player car
    drawCar(carX, 2.0f, true);

    // Opponent cars
    for (int i = 0; i < 3; i++)
        drawCar(obsX[i], obsZ[i], false);

    drawText(20, 565, "3D CAR RACING");
    drawText(20, 535, "Score: " + to_string(score));
    drawText(20, 505, "Arrow Keys: Move | R: Restart");

    if (gameOver)
    {
        drawText(300, 320, "GAME OVER!");
        drawText(265, 285, "Press R to Play Again");
    }

    glutSwapBuffers();
}

void update(int value)
{
    if (!gameOver)
    {
        roadMove += speed;

        if (roadMove >= 4.0f)
            roadMove -= 4.0f;

        for (int i = 0; i < 3; i++)
        {
            obsZ[i] += speed;

            // Collision detection
            if (fabs(carX - obsX[i]) < 1.25f &&
                fabs(2.0f - obsZ[i]) < 1.45f)
            {
                gameOver = true;
            }

            // Recycle opponent car
            if (obsZ[i] > 5.0f)
            {
                obsZ[i] = -40.0f;
                obsX[i] = (rand() % 3 - 1) * 2.0f;
                score += 10;

                if (speed < 0.8f)
                    speed += 0.01f;
            }
        }
    }

    glutPostRedisplay();
    glutTimerFunc(30, update, 0);
}

void specialKeys(int key, int x, int y)
{
    if (gameOver)
        return;

    if (key == GLUT_KEY_LEFT && carX > -2.0f)
        carX -= 0.25f;

    if (key == GLUT_KEY_RIGHT && carX < 2.0f)
        carX += 0.25f;

    if (key == GLUT_KEY_UP && speed < 0.8f)
        speed += 0.04f;

    if (key == GLUT_KEY_DOWN && speed > 0.15f)
        speed -= 0.04f;

    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{
    if (key == 'r' || key == 'R')
    {
        carX = 0.0f;
        roadMove = 0.0f;
        speed = 0.35f;
        score = 0;
        gameOver = false;

        obsZ[0] = -15.0f;
        obsZ[1] = -30.0f;
        obsZ[2] = -45.0f;

        obsX[0] = -2.0f;
        obsX[1] = 0.0f;
        obsX[2] = 2.0f;
    }

    if (key == 27)
        exit(0);

    glutPostRedisplay();
}

void init()
{
    glClearColor(0.45f, 0.72f, 0.95f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 800.0 / 600.0, 0.1, 200);

    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv)
{
    srand((unsigned)time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB |
                        GLUT_DEPTH);

    glutInitWindowSize(800, 600);
    glutCreateWindow("3D Car Racing Game");

    init();

    glutDisplayFunc(display);
    glutSpecialFunc(specialKeys);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(30, update, 0);

    glutMainLoop();

    return 0;
}

