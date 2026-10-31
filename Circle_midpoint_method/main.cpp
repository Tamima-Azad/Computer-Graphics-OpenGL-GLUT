#include<windows.h>
#include<GL/glut.h>
#include<stdlib.h>
#include<bits/stdc++.h>
using namespace std;

void display(){
   // glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
  // glColor3f(1.0,1.0,1.0);
    glColor3f(0.0,0.0,0.0);
    glPointSize(5.0);
    glBegin(GL_POINTS);

    float r=24,x=0,y=r,xk=0,yk=r;
    float pk=1-r;
     for(float k=1;x<y;k++){
            if(pk>=0){
                x++;
                yk--;
            }
            else{
                x++;
                yk=yk;
            }
        pk=pk+2*(x)+(yk*yk-y*y)-(yk-y)+1;
        y=yk;
        glVertex2f(x, y);
        glVertex2f(y,x);
        glVertex2f(y,-x);
        glVertex2f(x,-y);
        glVertex2f(-x,-y);
        glVertex2f(-y,-x);
        glVertex2f(-y,x);
        glVertex2f(-x,y);
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
