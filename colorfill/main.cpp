#include<windows.h>
#include<GL/glut.h>
#include<stdlib.h>
#include<bits/stdc++.h>
using namespace std;

void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.0,1.0,1.0);
    glPointSize(5.0);
    glBegin(GL_POLYGON);
{
    glVertex3f(0,0,0);
    glVertex3f(5,0,0);

    //glVertex3f(0,0,0);
    //glVertex3f(3,6,0);

    //glVertex3f(5,0,0);
    glVertex3f(3,6,0);
}
    glEnd();
    glFlush();

//rectangle
    glColor3f(1.0,0.0,1.0);
    glBegin(GL_QUADS);
{

    float c=-20;
    glVertex3f(c+0,c+0,0);
    glVertex3f(c+0,c+40,0);
    glVertex3f(c+15,c+40,0);
    glVertex3f(c+15,c+0,0);
}
    glEnd();
    glFlush();
//circle
   glColor3f(0.0,0.0,1.0);
    glBegin(GL_POLYGON);

      float r=20,z=20;
      for(float i=-r;i<=r;i++){

        float y=sqrt(r*r-i*i);
        glVertex2f(z+i, z+y);
        glVertex2f(z+i, z-y);
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
    glutInitWindowSize(1500, 1500);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutCreateWindow("Direct Circle");
    init();
    glutDisplayFunc(display);
    glutMainLoop();

    return 0;
}
