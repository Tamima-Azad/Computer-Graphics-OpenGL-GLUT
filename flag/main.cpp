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
    glPointSize(5.0);
    glBegin(GL_POINTS);
     float x= -40;
    for(int i=0;i<=20+2;i++){
        glVertex2f(0,i-2);
        glVertex2f(-2+i,0);
    }
    for(int i=0;i<=20;i++){
        glVertex2f(20,i);
        glVertex2f(i,20);
    }
     for(int i=0;i<=20;i++){
        glVertex2f(0,-i);
        glVertex2f(-2,-i);
        glVertex2f(-2,i);
        //glVertex2f(i,20);
    }
     for(int i=0;i<=20;i++){
        glVertex2f(-10+i,-20);

       // glVertex2f(-2,i);
        //glVertex2f(i,20);
    }
         for(int i=0;i<=16;i++){
        glVertex2f(-8+i,-22);
    }
        for(int i=0;i<=12;i++){
        glVertex2f(-6+i,-24);
    }

    glEnd();
    glFlush();


    //Circle

  glPointSize(2.0);
    glBegin(GL_POINTS);
    glColor3f(1,0,0);

    float r=5,xx=0,y=r,xk=0,yk=r;
    float cx = 10, cy = 10;
    float pk=1-r;
     for(float k=1;xx<y;k++){
            if(pk>=0){
                xx++;
                yk--;
            }
            else{
                xx++;
                yk=yk;
            }
        pk=pk+2*(xx)+(yk*yk-y*y)-(yk-y)+1;
        y=yk;
        glVertex2f(cx+xx, cy+y);
        glVertex2f(cx+y,cy+xx);
        glVertex2f(cx+y,cy-xx);
        glVertex2f(cx+xx,cy-y);
        glVertex2f(cx-xx,cy-y);
        glVertex2f(cx-y,cy-xx);
        glVertex2f(cx-y,cy+xx);
        glVertex2f(cx-xx,cy+y);
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
