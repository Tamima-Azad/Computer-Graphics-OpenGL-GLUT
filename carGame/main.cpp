#include<windows.h>
#include<GL/glut.h>
#include<stdlib.h>
#include<bits/stdc++.h>
using namespace std;

float angle =0.0f,wheelAngle=0.0f;
float tx=0.0f,ty=0.0f ,sx=1.0f,sy=1.0f;
float bgX = 0.0f;


void circle(float cx,float cy, float r){
      glBegin(GL_POLYGON);

    for(int i=0; i<360; i++)
    {
       // float deg =i*3.1416/180.0;
        float deg =i;

        float x = cx + r * cos(deg);
        float y = cy + r * sin(deg);

        glVertex2f(x,y);
    }

    glEnd();

}
/*void car(float x,float y,float z){
   glColor3f(0.8, 0.0, 0.0);

    glBegin(GL_QUADS);

        // Main body
        glVertex3f(x - 8, y - 5, z);
        glVertex3f(x + 8, y - 5, z);
        glVertex3f(x + 8, y + 3, z);
        glVertex3f(x - 8, y + 3, z);

    glEnd();
        glColor3f(0.0,0.0,0.0);
        circle(-5,-9,2);
    circle(5,-9,2);

}*/
void wheel(float cx, float cy, float r, float angle)
{
    // Black wheel
    glColor3f(0.0, 0.0, 0.0);
    circle(cx, cy, r);

    // Wheel hub
    glColor3f(0.7, 0.7, 0.7);
    circle(cx, cy, r * 0.3);

    // Spoke
    glPushMatrix();

        glTranslatef(cx, cy, 0);
        glRotatef(angle, 0, 0, 1);

        glColor3f(1.0, 1.0, 1.0);

        glBegin(GL_LINES);

            glVertex2f(0, 0);
            glVertex2f(r * 0.8, 0);

        glEnd();

    glPopMatrix();
}
void car(float x, float y, float z)
{
    // =========================
    // Car Body
    // =========================
    glColor3f(0.8, 0.0, 0.0);

    glBegin(GL_QUADS);

        // Main body
        glVertex3f(x - 15, y - 5, z);
        glVertex3f(x + 15, y - 5, z);
        glVertex3f(x + 15, y + 3, z);
        glVertex3f(x - 15, y + 3, z);

    glEnd();


    // =========================
    // Car Top / Roof
    // =========================
    glColor3f(0.9, 0.1, 0.1);

    glBegin(GL_QUADS);

        glVertex3f(x - 10, y + 3, z);
        glVertex3f(x + 8,  y + 3, z);
        glVertex3f(x + 4,  y + 9, z);
        glVertex3f(x - 6,  y + 9, z);

    glEnd();


    // =========================
    // Left Window
    // =========================
    glColor3f(0.2, 0.6, 0.9);

    glBegin(GL_QUADS);

        glVertex3f(x - 5.5, y + 4, z);
        glVertex3f(x - 0.5, y + 4, z);
        glVertex3f(x - 2,   y + 8, z);
        glVertex3f(x - 5.5, y + 8, z);

    glEnd();


    // =========================
    // Right Window
    // =========================
    glBegin(GL_QUADS);

        glVertex3f(x + 0.5, y + 4, z);
        glVertex3f(x + 7,   y + 4, z);
        glVertex3f(x + 4,   y + 8, z);
        glVertex3f(x + 0.5, y + 8, z);

    glEnd();

    wheel(x - 9, y - 6, 3, wheelAngle);
wheel(x + 9, y - 6, 3, wheelAngle);

    // =========================
    // Front Light
    // =========================
    glColor3f(1.0, 1.0, 0.0);

    glBegin(GL_QUADS);

        glVertex3f(x + 14, y - 2, z);
        glVertex3f(x + 15, y - 2, z);
        glVertex3f(x + 15, y,     z);
        glVertex3f(x + 14, y,     z);

    glEnd();


    // =========================
    // Back Light
    // =========================
    glColor3f(1.0, 0.0, 0.0);

    glBegin(GL_QUADS);

        glVertex3f(x - 15, y - 2, z);
        glVertex3f(x - 14, y - 2, z);
        glVertex3f(x - 14, y,     z);
        glVertex3f(x - 15, y,     z);

    glEnd();
}

void display(){

        glClear(GL_COLOR_BUFFER_BIT);
                glPushMatrix();
               glScalef(sx,sy,1.0f);
        glPushMatrix();
        glTranslatef(bgX, 0.0f, 0.0f);

//sky
        glColor3f(0.2,0.7,1);
    glPointSize(5.0);
    glBegin(GL_QUADS);
{
    glVertex3f(-500,15,0);
    glVertex3f(500,15,0);
    glVertex3f(500,500,0);
    glVertex3f(-500,500,0);

}
glEnd();
   glColor3f(1.0,1.0,1.0);
    float xc=-10,yc=40;

    circle(xc,yc,3);
    circle(xc+3,yc+1,4);
    circle(xc+7,yc,3);
    circle(xc+4,yc-1,3);

     xc=38,yc=40;
    circle(xc,yc,3);
    circle(xc+3,yc+1,4);
    circle(xc+7,yc,3);
    circle(xc+4,yc-1,3);

//grean area
    glColor3f(0.0,0.9,0.3);
    glPointSize(5.0);
    glBegin(GL_QUADS);
{
    glVertex3f(-500,15,0);
    glVertex3f(500,15,0);
    glVertex3f(500,10,0);
    glVertex3f(-500,10,0);

}
glEnd();

//road
        glColor3f(0.9,1.7,0.9);
    glPointSize(5.0);
    glBegin(GL_QUADS);
{
    glVertex3f(-500,10,0);
    glVertex3f(500,10,0);
    glVertex3f(500,-20,0);
    glVertex3f(-500,-20,0);

}
glEnd();

//green area

    glColor3f(0.0,0.9,0.3);
    glPointSize(5.0);
    glBegin(GL_QUADS);
{
    glVertex3f(-500,-20,0);
    glVertex3f(500,-20,0);
    glVertex3f(500,-500,0);
    glVertex3f(-500,-500,0);

}
glEnd();
glPopMatrix();

    //glColor3f(0.7,0.1,0.4
    glPushMatrix();
    glTranslatef(tx,ty,0.0f);
    glRotatef(angle, 0.0f,0.0f,1.0f);

    car(0,0,0);
    glPopMatrix();
    glEnd();

     glPopMatrix();
    glFlush();
}
void keyboard(unsigned char key,int x,int y){
    switch (key){
      //right
case 'd':
    if(tx<20.0f){
            tx+=2.0f;
    }
    else {
            bgX -=2.0f;
        float xc=bgX,yc=40;
    circle(xc,yc,3);
    circle(xc+3,yc+1,4);
    circle(xc+7,yc,3);
    circle(xc+4,yc-1,3);
    }
    wheelAngle -=20.0f;
    break;
    //left
    case 'a':
        if(tx >-20.0f) tx-=2.0f;
        else bgX +=2.0f;
    wheelAngle +=20.0f;
    break;
    //up
    case 'w':

        if(ty <13.0f){
                 ty+=2.0f;
                 if(ty>13.0f) ty=13.0f;
        }
               break;

        //down
        case 's':
            if(ty>-11.0f){
                      ty-=2.0f;
                      if(ty<-11.0f) ty=-11.0f;
            }
            break;

            //rotate right
        case 'r':
            angle+=10.0f;
             wheelAngle -=20.0f;
            break;
            //rotate left
        case 'l':
            angle -=10.0f;
             wheelAngle +=20.0f;
            break;
            //scale up

case '+':
    sx*=1.1f;
    sy*=1.1f;
    break;
    //scale down
    case '-':
    sx*=0.9f;
    sy*=0.9f;
    break;
case 'q':
    exit(0);
    break;
    }
    glutPostRedisplay();
}
void init()
{
    glClearColor(1.0, 1.0, 1.0, 1.0);

    glOrtho(-50.0, 50.0, -50.0, 50.0, -50.0, 50.0);
}
int main(int a, char **b)
{
    glutInit(&a, b);
    glutInitWindowPosition(0,0);
    glutInitWindowSize(500, 500);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutCreateWindow("Car Drive");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMainLoop();

    return 0;
}
