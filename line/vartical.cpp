
#include <windows.h>
#include <GL/glut.h>
#include <stdlib.h>

int width = 640;
int height = 480;


int x1 = 320;
int y1 = 100;
int x2 = 320;
int y2 = 400;


void drawLineDDA(int x1, int y1, int x2, int y2)
{
    int dx = x2 - x1;
    int dy = y2 - y1;

    int steps = abs(dy);

    float xIncrement = 0;
    float yIncrement = (float)dy / steps;

    float x = x1;
    float y = y1;

    glBegin(GL_POINTS);

    for (int i = 0; i <= steps; i++)
    {
        glVertex2i((int)x, (int)y);

        x += xIncrement;
        y += yIncrement;
    }

    glEnd();
}

// Display function
void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.0f, 0.0f, 0.0f);

    drawLineDDA(x1, y1, x2, y2);

    glFlush();
}


void init()
{
    glClearColor(1.0, 1.0, 1.0, 1.0);

    // Set 2D projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluOrtho2D(0, width, 0, height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Make points visible
    glPointSize(2.0);
}

// Main function
int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    // Single buffer and RGB color mode
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);

    glutInitWindowSize(width, height);
    glutInitWindowPosition(100, 100);

    glutCreateWindow("Vertical Line Drawing using DDA Algorithm");

    init();

    glutDisplayFunc(display);

    glutMainLoop();

    return 0;
}

