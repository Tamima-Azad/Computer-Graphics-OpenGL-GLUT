#include<windows.h>
#include<GL/glut.h>
#include<stdlib.h>
#include<bits/stdc++.h>
using namespace std;

float moveX=0.0,moveY=0.0,moveZ=0.0;
float angleX=0.0,angleY=0.0,angleZ=0.0;
float scale=1.0;


void drawPolygon(){
    //front
     glColor3f(1.0,0.0,0.0);
    glBegin(GL_POLYGON);
    {
        glVertex3f(0,0,20);
        glVertex3f(25,0,20);
        glVertex3f(25,20,20);
        glVertex3f(0,20,20);
    }
    glEnd();
    //back
         glColor3f(0.0,1.0,0.0);
    glBegin(GL_POLYGON);
    {
        glVertex3f(0,0,-20);
        glVertex3f(25,0,-20);
        glVertex3f(25,20,-20);
        glVertex3f(0,20,-20);
    }
    glEnd();

    //left
         glColor3f(0.0,0.0,1.0);
    glBegin(GL_POLYGON);
    {
        glVertex3f(0,0,20);
        glVertex3f(0,20,20);
        glVertex3f(0,20,-20);
        glVertex3f(0,0,-20);
    }
    glEnd();
//right
             glColor3f(1.0,1.0,0.0);
    glBegin(GL_POLYGON);
    {
        glVertex3f(25,0,20);
        glVertex3f(25,0,-20);
        glVertex3f(25,20,-20);
        glVertex3f(25,20,20);
    }
    glEnd();

    //up
                 glColor3f(0.0,1.0,1.0);
    glBegin(GL_POLYGON);
    {
        glVertex3f(0,20,20);
        glVertex3f(25,20,20);
        glVertex3f(25,20,-20);
        glVertex3f(0,20,-20);
    }
    glEnd();
    //down
                     glColor3f(1.0,0.0,1.0);
    glBegin(GL_POLYGON);
    {
        glVertex3f(0,0,20);
        glVertex3f(25,0,20);
        glVertex3f(25,0,-20);
        glVertex3f(0,0,-20);
    }
    glEnd();



glFlush();
}

void display(){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();


    glTranslatef(0.0,0.0,-80.0);
    glPushMatrix();
    glTranslatef(-55.0,0.0,-80.0);

    glScalef(0.6,0.6,0.6);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(45.0,0.0,0.0);
    glTranslatef(moveX,moveY,moveZ);

    glRotatef(angleX,1.0,0.0,0.0);
    glRotatef(angleY,0.0,1.0,0.0);
    glRotatef(angleZ,0.0,0.0,1.0);

    glScalef(scale,scale,scale);

    drawPolygon();
    glPopMatrix();

    glFlush();
}

void keyboard(unsigned char key,int x,int y){
    switch (key){
        //move left
    case 'a':
    case 'A':
        moveX=moveX-5;
        break;
        //move right
    case 'd':
    case 'D':
        moveX=moveX+5;
        break;
        //move up
    case 'w':
    case 'W':
        moveY+=5;
        break;
        //move down
    case 's':
    case 'S':
        moveY-=5;
        break;
        //move forward
    case 'q':
    case 'Q':
        moveZ+=10;
        break;
        //move backward
    case 'e':
    case 'E':
        moveZ-=10;
        break;
        //rotation

    case 'x':
    case 'X':
        angleX+=10;
        break;

    case 'y':
    case 'Y':
        angleY+=10;
        break;

    case 'z':
    case 'Z':
        angleZ+=10;
        break;
        //scaling
    case '=':
        scale+=0.1;
        break;
    case '-':
        scale-=0.1;
        if(scale< 0.1) scale=0.1;
        break;
        //reset
    case 'r':
    case 'R':
        moveX=0.0,moveY=0.0,moveZ=0.0;
        angleX=0.0,angleY=0.0,angleZ=0.0;
        scale=1.0;
        break;
case 27:
    exit(0);
    break;
    }
    glutPostRedisplay();
}
void init()
{
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(100.0,1.0,1.0,500.0);
    /*
    Fov=field of view,aspect ratio=w/H,Near,Far
    */
    //model view
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}
int main(int a, char **b)
{
    glutInit(&a, b);
    glutInitWindowPosition(0,0);
    glutInitWindowSize(1000, 1000);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    glutCreateWindow("3D Table Transformation");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
