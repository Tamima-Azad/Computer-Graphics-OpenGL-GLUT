#include<windows.h>
#include<GL/glut.h>
#include<stdlib.h>
#include<bits/stdc++.h>
using namespace std;

float angle =0.0f;
float tx=0.0f,ty=0.0f ,sx=1.0f,sy=1.0f;



void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    glPushMatrix();
    glTranslatef(tx,ty,0.0f);
    glRotatef(angle, 0.0f,0.0f,1.0f);
    glScalef(sx,sy,0.0f);

/*    glColor3f(0.0,1.0,1.0);
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
    glFlush();*/

//rectangle
    glColor3f(0.7,0.4,1.0);
    glBegin(GL_QUADS);
{

    float c=-20;
    glVertex3f(c+0,c+0,0);
    glVertex3f(c+0,c+40,0);
    glVertex3f(c+15,c+40,0);
    glVertex3f(c+15,c+0,0);
}
    glEnd();
    glPopMatrix();
/*circle
   glColor3f(0.0,0.0,1.0);
    glBegin(GL_POLYGON);

      float r=20,z=20;
      for(float i=-r;i<=r;i++){

        float y=sqrt(r*r-i*i);
        glVertex2f(z+i, z+y);
        glVertex2f(z+i, z-y);
      }

    glEnd();*/
    glFlush();
}

void keyboard(unsigned char key,int x,int y){
    switch (key){
case 'r':
    angle +=10.0f;
    break;
    case 'l':
    angle -=10.0f;
    break;
case 't':
    tx+=10.0f;
    break;
    case 'y':
    tx-=10.0f;
    break;
case 's':
    sx*=1.1f;
    sy*=1.1f;
    break;
    case 'd':
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
    glutInitWindowSize(1500, 1500);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutCreateWindow("Direct Circle");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMainLoop();

    return 0;
}
