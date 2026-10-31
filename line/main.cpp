#include<windows.h>
#include<GL/glut.h>
#include<stdlib.h>
#include<bits/stdc++.h>
using namespace std;

void display(){


    //Rectangle
   // glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.0,1.0,0.0);
    glPointSize(2.0);
    //glBegin(GL_POINTS);
    glBegin(GL_LINES);

    float x =-30;
    glVertex3f(0+x,0,0);
    glVertex3f(0+x,5,0);

    glVertex3f(0+x,5,0);
    glVertex3f(10+x,7,0);

    glVertex3f(10+x,7,0);
    glVertex3f(40+x,7,0);

    glVertex3f(10+x,7,0);
    glVertex3f(20+x,12,0);

    glVertex3f(12+x,7,0);
    glVertex3f(20+x,11,0);

   glVertex3f(20+x,11,0);
    glVertex3f(24+x,11,0);

    glVertex3f(24+x,11,0);
    glVertex3f(24+x,7,0);

    glVertex3f(26+x,7,0);
    glVertex3f(26+x,11,0);

    glVertex3f(26+x,11,0);
    glVertex3f(30+x,11,0);

    glVertex3f(30+x,11,0);
    glVertex3f(38+x,7,0);

    glVertex3f(20+x,12,0);
    glVertex3f(30+x,12,0);

    glVertex3f(30+x,12,0);
    glVertex3f(40+x,7,0);

    glVertex3f(30+x,12,0);
    glVertex3f(40+x,7,0);

    glVertex3f(40+x,7,0);
    glVertex3f(50+x,5,0);

    glVertex3f(50+x,5,0);
    glVertex3f(50+x,0,0);

    glVertex3f(50+x,0,0);
    glVertex3f(51+x,0,0);

    glVertex3f(51+x,0,0);
    glVertex3f(51+x,-1,0);

    glVertex3f(25+x,7,0);
    glVertex3f(25+x,-1,0);

    glVertex3f(0+x,0,0);
    glVertex3f(-1+x,0,0);

    glVertex3f(-1+x,0,0);
    glVertex3f(-1+x,-1,0);

    glVertex3f(-1+x,-1,0);
    glVertex3f(5+x,-1,0);

    glVertex3f(51+x,-1,0);
    glVertex3f(45+x,-1,0);

    glVertex3f(10+x,-1,0);
    glVertex3f(40+x,-1,0);

   // glVertex3f(26+x,5,0);
  //  glVertex3f(26+x,5,0);

    glVertex3f(-1+x,0,0);
    glVertex3f(2+x,0,0);

    glVertex3f(2+x,0,0);
    glVertex3f(2+x,-1,0);

    glVertex3f(50+x,0,0);
    glVertex3f(48+x,0,0);

    glVertex3f(48+x,0,0);
    glVertex3f(48+x,-1,0);

    glVertex3f(x,3,0);
    glVertex3f(x+1,3,0);

     glVertex3f(x+1,3,0);
    glVertex3f(x+1,2,0);

    glVertex3f(x+1,2,0);
    glVertex3f(x+1,1,0);

    glEnd();
    glFlush();


    //Circle

    glBegin(GL_POINTS);
    glColor3f(1,0,0);
      float r=2.5;
      float xc=-22.5;
      for(float i=-r;i<=r;i++){

        float y=sqrt(r*r-i*i);
        glVertex2f(xc+i, y-1);
        glVertex2f(xc+i, -y-1);
      }
    glEnd();
    glFlush();


    glBegin(GL_POINTS);
    glColor3f(0,0,0);
      float rr=1;
      float xcc=-22.5;
      for(float i=-rr;i<=rr;i++){

        float y=sqrt(rr*rr-i*i);
        glVertex2f(xcc+i, y-1);
        glVertex2f(xcc+i, -y-1);
      }
    glEnd();
    glFlush();

        glBegin(GL_POINTS);
    glColor3f(0,0,0);
      float r3=1;
      float x3=12.5;
      for(float i=-r3;i<=r3;i++){

        float y=sqrt(r3*r3-i*i);
        glVertex2f(x3+i, y-1);
        glVertex2f(x3+i, -y-1);
      }
    glEnd();
    glFlush();

     glBegin(GL_POINTS);
    glColor3f(1,0,0);
      float r4=2.5;
      float x4=12.5;
      for(float i=-r4;i<=r4;i++){

        float y=sqrt(r4*r4-i*i);
        glVertex2f(x4+i, y-1);
        glVertex2f(x4+i, -y-1);
      }
    glEnd();
    glFlush();
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
    glutCreateWindow("Direct Circle");
    init();
    glutDisplayFunc(display);
    glutMainLoop();

    return 0;
}
