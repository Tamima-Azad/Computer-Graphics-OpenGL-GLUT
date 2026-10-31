#include<windows.h>
#include<GL/glut.h>
#include<stdlib.h>
#include<bits/stdc++.h>
using namespace std;

void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0,1.0,1.0);
    glBegin(GL_POINTS);

        float xt=2,yt=0,xe=7,ye=4;
        //vertical
      /* for(int i=0;i<=xe;i++){
            glVertex2f(i, ye);
        }*/
//Horizontal
       /*for(int i=0;i<=ye;i++){
            glVertex2f(xt,i);
        }*/
//diagonal
       /*for(int i=0;i<=ye;i++){
            xt++;
            glVertex2f(xt,i);
        }*/
//direct line drawing
       /* float m=(ye-yt)/(xe-xt);
        float b=yt-m*xt;
        for(int i=xt + 1;i<=xe;i++){
            glVertex2f(i,round(yt));
            yt=m*i+b;

        }*/
//DDA
        float m=(ye-yt)/(xe-xt);
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
        }




        //glVertex2f(0, 0);
        //glVertex2f(9,20);
    glEnd();
    glFlush();
}
void init()
{
    glOrtho(-50.0, 50.0, -50.0, 50.0, -50.0, 50.0);
}
int main(int a, char **b)
{
    glutInit(&a, b);
    glutInitWindowPosition(0,0);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutCreateWindow("Basic Objects......");
    init();
    glutDisplayFunc(display);
    glutMainLoop();
}
