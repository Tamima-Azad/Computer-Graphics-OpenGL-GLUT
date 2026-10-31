#include<windows.h>
#include<GL/glut.h>
#include<stdlib.h>
#include<bits/stdc++.h>
using namespace std;
//float xt,xe,yt,ye;

void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(20.0,20.0,20.0);
    glBegin(GL_POINTS);


        float xt=2,yt=0,xe=7,ye=4;
        //cin>>xt>>yt>>xe>>ye;
        //vertical
      /*for(int i=0;i<=xe;i++){
            glVertex2f(i, ye);
        }*/
//Horizontal
       /*for(int i=0;i<=ye;i++){
            glVertex2f(xt,i);
        }*/
//diagonal
     /*  for(int i=0;i<=ye;i++){
            xt++;
            glVertex2f(xt,i);
        }*/
//direct line drawing
        /*float m=(ye-yt)/(xe-xt);
        float b=yt-m*xt;
        for(int i=xt + 1;i<=xe;i++){
            glVertex2f(i,round(yt));
            yt=m*i+b;

        }*/
//DDA
      /* float m=(ye-yt)/(xe-xt);
        if(abs(m)<1 and xt>xe){
            swap(xt,xe);
            swap(yt,ye);

        }
        glVertex2f(xt,yt);
        if(abs(m)<1){
            float x=xt+1;
            float y=yt+m;
            for(int i=x;i<=xe;i++){
                glVertex2f(i,round(y));
                y=y+m;
            }
        }*/
        float m=(ye-yt)/(xe-xt);
        if((abs(m>1) and yt>ye) or (abs(m)<1 and xt>xe)){
            swap(xt,xe);
            swap(yt,ye);
        }
        glVertex2f(xt,yt);
        if(abs(m)>1){
            m=1/m;
            float y=yt+1;
            float x=xt;
             for(int i=y;i<=ye;i++){

                glVertex2f(round(x),i);
                y=y+1;
            }
        }
        else if(abs(m)<1){
            float x=xt+1;
            float y=yt+m;

            for(int i=x;i<=xe;i++){

                glVertex2f(i,round(y));
                y=y+m;
            }
        }

        //glVertex2f(0, 0);
        //glVertex2f(9,20);
    glEnd();
    glFlush();
}
void displayName()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1,1,1);

    glBegin(GL_LINES);

    float x = -20;

    // A
    glVertex3f(x+0,0,0);
    glVertex3f(x+2,10,0);
    glVertex3f(x+2,10,0);
    glVertex3f(x+4,0,0);
    glVertex3f(x+1,5,0);
    glVertex3f(x+3,5,0);

    // Z
    glVertex3f(x+6,10,0);
    glVertex3f(x+10,10,0);
    glVertex3f(x+10,10,0);
    glVertex3f(x+6,0,0);
    glVertex3f(x+6,0,0);
    glVertex3f(x+10,0,0);

    // A
    glVertex3f(x+12,0,0);
    glVertex3f(x+14,10,0);
    glVertex3f(x+14,10,0);
    glVertex3f(x+16,0,0);
    glVertex3f(x+13,5,0);
    glVertex3f(x+15,5,0);

    // D
    glVertex3f(x+18,0,0);
    glVertex3f(x+18,10,0);
    glVertex3f(x+18,10,0);
    glVertex3f(x+22,8,0);
    glVertex3f(x+22,8,0);
    glVertex3f(x+22,2,0);
    glVertex3f(x+22,2,0);
    glVertex3f(x+18,0,0);
    // 6
    glVertex3f(x+0,-12,0);
    glVertex3f(x+0,-2,0);
    glVertex3f(x+0,-2,0);
    glVertex3f(x+4,-2,0);
    glVertex3f(x+0,-7,0);
    glVertex3f(x+4,-7,0);
    glVertex3f(x+4,-7,0);
    glVertex3f(x+4,-12,0);
    glVertex3f(x+4,-12,0);
    glVertex3f(x+0,-12,0);

    // 0
    glVertex3f(x+8,-2,0);
    glVertex3f(x+8,-12,0);
    glVertex3f(x+8,-12,0);
    glVertex3f(x+12,-12,0);
    glVertex3f(x+12,-12,0);
    glVertex3f(x+12,-2,0);
    glVertex3f(x+12,-2,0);
    glVertex3f(x+8,-2,0);

    glEnd();

    glFlush();
}

void init()
{
    glOrtho(-50.0, 50.0, -50.0, 50.0, -50.0, 50.0);
}
int main(int a, char **b)
{

    //cin>>xt>>yt>>xe>>ye;
    glutInit(&a, b);
    glutInitWindowPosition(0,0);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutCreateWindow("Basic Objects......");
    init();
    //glutDisplayFunc(display);
    glutDisplayFunc(displayName);
    glutMainLoop();
}
