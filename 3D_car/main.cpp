#include<windows.h>
#include<GL/glut.h>
#include<stdlib.h>
#include<bits/stdc++.h>
using namespace std;

float moveX=0.0,moveY=0.0,moveZ=0.0;
float angleX=0.0,angleY=0.0,angleZ=0.0;
float scale=1.0;

float angle =0.0f,wheelAngle=0.0f;
float tx=0.0f,ty=0.0f ,sx=1.0f,sy=1.0f;
void circle(float cx,float cy, float r){
      glBegin(GL_POLYGON);
    for(int i=0; i<360; i++)
    {
        float deg =i;
        float x = cx + r * cos(deg);
        float y = cy + r * sin(deg);
        glVertex3f(x,y,3);
       // glVertex3f(x,y,-4);
    }
    glEnd();

          glBegin(GL_POLYGON);
    for(int i=0; i<360; i++)
    {
        float deg =i;
        float x = cx + r * cos(deg);
        float y = cy + r * sin(deg);
       // glVertex3f(x,y,4);
        glVertex3f(x,y,-3);
    }
    glEnd();
}
void wheel(float cx, float cy, float r, float angle)
{
    // Black wheel
    glColor3f(0.0, 0.0, 0.0);
    circle(cx, cy, r);
    // Wheel hub
    glColor3f(0.7, 0.7, 0.8);
    circle(cx, cy, r * 0.3);
    // Spoke
    glPushMatrix();
    glTranslatef(cx, cy, 1);
        glRotatef(angle, 0, 0, 1);
        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_POLYGON);
        glVertex3f(0,0,4);
        glVertex3f(r*0.8,0,4);

        glEnd();
    glPopMatrix();
}

void leg(float x,float y,float z,float sx,float sy,float sz){

         //leg front
    glBegin(GL_QUADS);
    glColor3f(1.0,0.0,0.0);
    {
    glVertex3f(x-sx,y-sy,z+sz);
    glVertex3f(x+sx,y-sy,z+sz);
    glVertex3f(x+sx,y+sy,z+sz);
    glVertex3f(x-sx,y+sy,z+sz);
    }
    // Back
    glColor3f(0.0,1.0,0.0);
    glVertex3f(x-sx,y-sy,z-sz);
    glVertex3f(x+sx,y-sy,z-sz);
    glVertex3f(x+sx,y+sy,z-sz);
    glVertex3f(x-sx,y+sy,z-sz);
    //down
     glColor3f(0.0,0.0,1.0);
    glVertex3f(x-sx,y-sy,z-sz);
    glVertex3f(x-sx,y-sy,z+sz);
    glVertex3f(x+sx,y-sy,z+sz);
    glVertex3f(x+sx,y-sy,z-sz);

        //up
     glColor3f(1.0,1.0,0.0);
    glVertex3f(x-sx,y+sy,z-sz);
    glVertex3f(x-sx,y+sy,z+sz);
    glVertex3f(x+sx,y+sy,z+sz);
    glVertex3f(x+sx,y+sy,z-sz);
    //left
         glColor3f(0.0,1.0,1.0);
    glVertex3f(x-sx,y-sy,z-sz);
    glVertex3f(x-sx,y-sy,z+sz);
    glVertex3f(x-sx,y+sy,z+sz);
    glVertex3f(x-sx,y+sy,z-sz);

        //right
    glColor3f(1.0,0.0,1.0);
    glVertex3f(x+sx,y-sy,z-sz);
    glVertex3f(x+sx,y-sy,z+sz);
    glVertex3f(x+sx,y+sy,z+sz);
    glVertex3f(x+sx,y+sy,z-sz);

    glEnd();
}

void drawPolygon(){
    //carbody
    leg(0,-2,0,15,5,3);
//car roof front
       glColor3f(0.9, 0.1, 0.1);
    glBegin(GL_QUADS);
        glVertex3f(- 10, 3, 4);
        glVertex3f(+ 8,  3, 4);
        glVertex3f(+ 4,  9, 4);
        glVertex3f(- 6,  9, 4);
    glEnd();
//carroof back
           glColor3f(0.1, 0.9, 0.1);
    glBegin(GL_QUADS);
        glVertex3f(- 10, 3, -4);
        glVertex3f(+ 8,  3, -4);
        glVertex3f(+ 4,  9, -4);
        glVertex3f(- 6,  9, -4);
    glEnd();

    //car roof left
               glColor3f(0.1, 0.1, 0.9);
    glBegin(GL_QUADS);
        glVertex3f(- 10, 3, 4);
        glVertex3f(- 10, 3, -4);
        glVertex3f(- 6,  9, -4);
        glVertex3f(- 6,  9, 4);
    glEnd();
//car roof right
                   glColor3f(0.9, 0.9, 0.1);
    glBegin(GL_QUADS);
        glVertex3f(8, 3, 4);
        glVertex3f(8, 3, -4);
        glVertex3f(4,  9, -4);
        glVertex3f(4,  9, 4);
    glEnd();

    //car roof top
                   glColor3f(0.1, 0.9, 0.9);
    glBegin(GL_QUADS);
        glVertex3f(- 6,  9, -4);
        glVertex3f(- 6,  9, 4);
        glVertex3f(4,  9, 4);
        glVertex3f(4,  9, -4);
    glEnd();
    //car left window
    glColor3f(0.2, 0.6, 0.9);
    glBegin(GL_POLYGON);
        glVertex3f(- 5.5, 4, 4);
        glVertex3f(- 0.5, 4, 4);
        glVertex3f(- 2,   8, 4);
        glVertex3f(- 5.5, 8, 4);
    glEnd();



        wheel(- 7, - 9, 3, wheelAngle);
wheel(+ 7, -9, 3, wheelAngle);

glFlush();
}


void display(){
           glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glColor3f(0.2,0.7,1);
    glPointSize(5.0);
    glBegin(GL_QUADS);
{
    glVertex3f(-1000,15,0);
    glVertex3f(1000,15,0);
    glVertex3f(1000,50,0);
    glVertex3f(-1000,50,0);
}
glEnd();
    glColor3f(0.0,0.9,0.3);
    glPointSize(5.0);
    glBegin(GL_QUADS);
{
    glVertex3f(-1000,15,0);
    glVertex3f(1000,15,0);
    glVertex3f(1000,10,0);
    glVertex3f(-1000,10,0);
}
glEnd();
        glColor3f(0.9,1.7,0.9);
    glPointSize(5.0);
    glBegin(GL_QUADS);
{
    glVertex3f(-1000,10,0);
    glVertex3f(1000,10,0);
    glVertex3f(1000,-20,0);
    glVertex3f(-1000,-20,0);
}
glEnd();
    glColor3f(0.0,0.9,0.3);
    glPointSize(5.0);
    glBegin(GL_QUADS);
{
    glVertex3f(-1000,-20,0);
    glVertex3f(1000,-20,0);
    glVertex3f(1000,-50,0);
    glVertex3f(-1000,-50,0);
}
glEnd();

 glLoadIdentity();
    glTranslatef(0.0,0.0,-80.0);
    glPushMatrix();
    glTranslatef(-55.0,0.0,-80.0);

    glScalef(0.6,0.6,0.6);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0,0.0,0.0);
    glTranslatef(moveX,moveY,moveZ);

    glRotatef(angleX,1.0,0.0,0.0);
    glRotatef(angleY,0.0,1.0,0.0);
    glRotatef(angleZ,0.0,0.0,1.0);

    glScalef(scale,scale,scale);

    drawPolygon();
   // car(10,10,20);
    glPopMatrix();
    glFlush();
    /*
    glPushMatrix();
    glTranslatef(tx,ty,0.0f);
    glRotatef(angle, 0.0f,0.0f,1.0f);
    glScalef(sx,sy,1.0f);
    car(0,0,0);
    glPopMatrix();
    glFlush();*/
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
