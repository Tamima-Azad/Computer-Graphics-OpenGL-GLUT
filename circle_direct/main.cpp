#include<windows.h>
#include<GL/glut.h>
#include<stdlib.h>
#include<bits/stdc++.h>
using namespace std;

void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.0,0.0,0.0);
    glPointSize(5.0);
    glBegin(GL_POINTS);

      float r=20;
      for(float i=-r;i<=r;i++){

        float y=sqrt(r*r-i*i);
        glVertex2f(i, y);
        glVertex2f(i, -y);
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
