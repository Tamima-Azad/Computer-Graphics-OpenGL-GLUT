#include<windows.h>
#include<GL/glut.h>
#include<stdlib.h>
#include<bits/stdc++.h>
using namespace std;

float moveX=0.0, moveY=0.0, moveZ=0.0;
float angleX=0.0, angleY=0.0, angleZ=0.0;
float scale=1.0;

// Cube function
void cube(float x, float y, float z, float sx, float sy, float sz)
{
    glBegin(GL_QUADS);
    // Front
    glColor3f(1.0,0.0,0.0);
    glVertex3f(x-sx,y-sy,z+sz);
    glVertex3f(x+sx,y-sy,z+sz);
    glVertex3f(x+sx,y+sy,z+sz);
    glVertex3f(x-sx,y+sy,z+sz);

    // Back
    glColor3f(0.0,1.0,0.0);
    glVertex3f(x-sx,y-sy,z-sz);
    glVertex3f(x-sx,y+sy,z-sz);
    glVertex3f(x+sx,y+sy,z-sz);
    glVertex3f(x+sx,y-sy,z-sz);

    // Left
    glColor3f(0.0,0.0,1.0);
    glVertex3f(x-sx,y-sy,z-sz);
    glVertex3f(x-sx,y-sy,z+sz);
    glVertex3f(x-sx,y+sy,z+sz);
    glVertex3f(x-sx,y+sy,z-sz);

    // Right
    glColor3f(1.0,1.0,0.0);
    glVertex3f(x+sx,y-sy,z-sz);
    glVertex3f(x+sx,y+sy,z-sz);
    glVertex3f(x+sx,y+sy,z+sz);
    glVertex3f(x+sx,y-sy,z+sz);

    // Top
    glColor3f(1.0,0.0,1.0);
    glVertex3f(x-sx,y+sy,z-sz);
    glVertex3f(x-sx,y+sy,z+sz);
    glVertex3f(x+sx,y+sy,z+sz);
    glVertex3f(x+sx,y+sy,z-sz);

    // Bottom
    glColor3f(0.0,1.0,1.0);
    glVertex3f(x-sx,y-sy,z-sz);
    glVertex3f(x+sx,y-sy,z-sz);
    glVertex3f(x+sx,y-sy,z+sz);
    glVertex3f(x-sx,y-sy,z+sz);

    glEnd();
}


void drawTable()
{
    // Table top
    cube(0,25,0,40,5,25);

    // Leg 1
    cube(-32,0,-18,5,25,5);

    // Leg 2
   // cube(32,0,-18,5,25,5);

    // Leg 3
  //  cube(-32,0,18,5,25,5);

    // Leg 4
    //cube(32,0,18,5,25,5);
}


void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    // Move camera backward
    glTranslatef(0.0,0.0,-180.0);

    glPushMatrix();

    // Translation
    glTranslatef(moveX,moveY,moveZ);

    // Rotation
    glRotatef(angleX,1.0,0.0,0.0);
    glRotatef(angleY,0.0,1.0,0.0);
    glRotatef(angleZ,0.0,0.0,1.0);

    // Scaling
    glScalef(scale,scale,scale);

    // Draw table
    drawTable();

    glPopMatrix();

    glFlush();
}


void keyboard(unsigned char key,int x,int y)
{
    switch(key)
    {
        // Move left
        case 'a':
        case 'A':
            moveX-=5;
            break;

        // Move right
        case 'd':
        case 'D':
            moveX+=5;
            break;

        // Move up
        case 'w':
        case 'W':
            moveY+=5;
            break;

        // Move down
        case 's':
        case 'S':
            moveY-=5;
            break;

        // Move forward
        case 'q':
        case 'Q':
            moveZ+=10;
            break;

        // Move backward
        case 'e':
        case 'E':
            moveZ-=10;
            break;

        // Rotate X
        case 'x':
        case 'X':
            angleX+=10;
            break;

        // Rotate Y
        case 'y':
        case 'Y':
            angleY+=10;
            break;

        // Rotate Z
        case 'z':
        case 'Z':
            angleZ+=10;
            break;

        // Increase scale
        case '+':
        case '=':
            scale+=0.1;
            break;

        // Decrease scale
        case '-':
            scale-=0.1;

            if(scale<0.1)
                scale=0.1;

            break;

        // Reset
        case 'r':
        case 'R':
            moveX=0.0;
            moveY=0.0;
            moveZ=0.0;

            angleX=0.0;
            angleY=0.0;
            angleZ=0.0;

            scale=1.0;
            break;

        // ESC
        case 27:
            exit(0);
            break;
    }

    glutPostRedisplay();
}


void init()
{
    glClearColor(1.0,1.0,1.0,1.0);

    glEnable(GL_DEPTH_TEST);

    // Projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(
        60.0,
        1.0,
        1.0,
        500.0
    );

    // Model View
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


int main(int argc,char **argv)
{
    glutInit(&argc,argv);

    glutInitWindowPosition(0,0);
    glutInitWindowSize(1000,1000);

    glutInitDisplayMode(
        GLUT_SINGLE |
        GLUT_RGB |
        GLUT_DEPTH
    );

    glutCreateWindow("3D Table Transformation");

    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);

    glutMainLoop();

    return 0;
}
