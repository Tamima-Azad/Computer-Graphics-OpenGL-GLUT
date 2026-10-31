#include<windows.h>
#include<GL/glut.h>
#include<stdlib.h>
#include<bits/stdc++.h>
using namespace std;

void circle(float cx, float cy, float r)
{
    glBegin(GL_POLYGON);

    for(int i=0; i<360; i++)
    {

        float x = cx + r * cos(i);
        float y = cy + r * sin(i);

        glVertex2f(x,y);
    }

    glEnd();
}
void display(){
    glClear(GL_COLOR_BUFFER_BIT);

     glColor3f(0.2,0.7,1);
    glPointSize(5.0);
    glBegin(GL_QUADS);
{
    glVertex3f(-50,2,0);
    glVertex3f(50,2,0);
    glVertex3f(50,50,0);
    glVertex3f(-50,50,0);

}
    glEnd();
    glFlush();
  // glColor3f(1.0,1.0,1.0);
    glColor3f(1.0,0.65,0.05);
    circle(-32,34,5);

     glColor3f(1.0,0.8,0.1);
      glBegin(GL_LINES);

        glVertex2f(-32,41);
        glVertex2f(-32,45);

        glVertex2f(-32,27);
        glVertex2f(-32,23);

        glVertex2f(-39,34);
        glVertex2f(-43,34);

        glVertex2f(-25,34);
        glVertex2f(-21,34);

        glVertex2f(-37,39);
        glVertex2f(-40,42);

        glVertex2f(-27,39);
        glVertex2f(-24,42);

        glVertex2f(-37,29);
        glVertex2f(-40,26);
         glVertex2f(-27,29);
        glVertex2f(-24,26);

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

     glColor3f(0.2,0.65,0.1);
        glBegin(GL_POLYGON);
{
    float xp=-50;
    while(1){

    glVertex3f(xp,2,0);
    glVertex3f(xp+10,2,0);
    glVertex3f(xp+5,6,0);
    xp+=25;
    if(xp>80){
            break;
    }

    }

   // glVertex3f(23,0,0);
}
    glEnd();
    glFlush();
         glColor3f(0.7,1.0,0.0);
    glBegin(GL_QUADS);
{
        glVertex2f(-50,-50);
        glVertex2f(50,-50);
        glVertex2f(50,2);
        glVertex2f(-50,2);

}
    glEnd();
    glFlush();

    glColor3f(1.0,1.0,1.0);
    glPointSize(5.0);
    glBegin(GL_QUADS);
{
    glVertex3f(20,0,0);
    glVertex3f(23,-0.5,0);
    glVertex3f(23,6,0);
    glVertex3f(20,6,0);

}
    glEnd();
    glFlush();

        glBegin(GL_POLYGON);
{
    glVertex3f(20,6,0);
    glVertex3f(23,6,0);
    glVertex3f(21.5,8,0);
   // glVertex3f(23,0,0);
}
    glEnd();
    glFlush();

    glColor3f(0.0,0.0,1.0);
    glBegin(GL_QUADS);
{
    glVertex3f(21,0,0);
    glVertex3f(22,0,0);
    glVertex3f(22,3.5,0);
    glVertex3f(21,3.5,0);
   // glVertex3f(23,0,0);
}
    glEnd();
    glFlush();
glColor3f(0.5,0.5,0.0);
        glBegin(GL_QUADS);
{
    glVertex3f(23,-0.5,0);
    glVertex3f(26,-0.5,0);
    glVertex3f(26,6,0);
    glVertex3f(23,6,0);
   // glVertex3f(23,0,0);
}
    glEnd();
    glFlush();

    glColor3f(1.0,0.0,0.0);
        glBegin(GL_QUADS);
{

    glVertex3f(23,5,0);
    glVertex3f(27,5,0);
    glVertex3f(24.5,8,0);
    glVertex3f(20.5,8,0);

   // glVertex3f(21,8,0);

   // glVertex3f(23,0,0);
}
    glEnd();
    glFlush();

    glBegin(GL_QUADS);

    {
  glVertex3f(20.5,8,0);
    glVertex3f(21.5,7,0);
    glVertex3f(20,5,0);
    glVertex3f(19,5,0);
   // glVertex3f(21,8,0);

   // glVertex3f(23,0,0);
}
    glEnd();
    glFlush();


glColor3f(0.5,0.6,0.0);
        glBegin(GL_QUADS);
{
    glVertex3f(10,-0.5,0);
    glVertex3f(14,-0.5,0);
    glVertex3f(14,10,0);
    glVertex3f(10,10,0);
   // glVertex3f(23,0,0);
}
    glEnd();
    glFlush();

    glColor3f(0.0,1.0,0.0);
    glBegin(GL_POLYGON);
    float tx=18,ty=16;
    float r=8.5,x=0,y=r,xk=0,yk=r;
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
        glVertex2f(tx+x, ty+y);
        glVertex2f(tx+y,ty+x);
        glVertex2f(tx+y,ty-x);
        glVertex2f(tx+x,ty-y);
        glVertex2f(tx-x,ty-y);
        glVertex2f(tx-y,ty-x);
        glVertex2f(tx-y,ty+x);
        glVertex2f(tx-x,ty+y);
       // glVertex3f(8,0,0);

      }

    glEnd();


    glColor3f(0.0,1.0,0.0);
    glBegin(GL_POLYGON);
    tx=6,ty=16;
    r=8.5,x=0,y=r,xk=0,yk=r;
    pk=1-r;
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
        glVertex2f(tx+x, ty+y);
        glVertex2f(tx+y,ty+x);
        glVertex2f(tx+y,ty-x);
        glVertex2f(tx+x,ty-y);
        glVertex2f(tx-x,ty-y);
        glVertex2f(tx-y,ty-x);
        glVertex2f(tx-y,ty+x);
        glVertex2f(tx-x,ty+y);
       // glVertex3f(8,0,0);

      }

    glEnd();


    glColor3f(0.0,1.0,0.0);
    glBegin(GL_POLYGON);
     tx=12,ty=24;
     r=8.5,x=0,y=r,xk=0,yk=r;
     pk=1-r;
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
        glVertex2f(tx+x, ty+y);
        glVertex2f(tx+y,ty+x);
        glVertex2f(tx+y,ty-x);
        glVertex2f(tx+x,ty-y);
        glVertex2f(tx-x,ty-y);
        glVertex2f(tx-y,ty-x);
        glVertex2f(tx-y,ty+x);
        glVertex2f(tx-x,ty+y);
       // glVertex3f(8,0,0);

      }

    glEnd();
    glFlush();

glColor3f(0.82,0.68,0.38);

glBegin(GL_POLYGON);

    glVertex2f(-50,-50);
    glVertex2f(-50,-40);

    glVertex2f(-35,-35);
    glVertex2f(-20,-30);
    glVertex2f(-5,-24);
    glVertex2f(8,-18);
    glVertex2f(18,-10);

    // Door er samne road sesh
    glVertex2f(21,-0);

    glVertex2f(22,-0);

    glVertex2f(20,-10);
    glVertex2f(5,-18);
    glVertex2f(-10,-25);
    glVertex2f(-25,-32);
    glVertex2f(-40,-38);
    glVertex2f(-50,-43);

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
    glutInitWindowSize(1000, 1000);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutCreateWindow("Village Scenery");
    init();
    glutDisplayFunc(display);
    glutMainLoop();

    return 0;
}
