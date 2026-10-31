#include<windows.h>
#include<GL/glut.h>
#include<stdlib.h>
#include<bits/stdc++.h>
using namespace std;

void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0,1.0,1.0);
   glPointSize(2.0);
    glBegin(GL_POINTS);

        float xt=20,yt=10,xd=30,yd=18,yk=yt;
        float delx=xd-xt;

        float dely=yd-yt;
        float pk=2*dely-delx;
        if(pk<0){
            pk=pk+2*dely;
        }else{
            pk=pk+2*dely-2*delx;
        }

     for(float xk=xt;xk<=xd; ){
          //  cout<<pk<<endl;
          glVertex2f(xk, yt);
           cout<<" pk= "<<pk<<"  ("<<xk<<","<<yk<<")"<<endl;
            if(pk<0){
                xk++;
                yk=yk;
               // pk=pk+2*dely;
            }
            else{
                xk++;
                yk++;
               // pk=pk+2*dely-2*delx;
            }
        pk=pk+2*dely-2*delx*(yk-yt);
        yt=yk;
       // y=yk;
         // cout<<" pk= "<<pk<<"  ("<<xk<<","<<yk<<")"<<endl;
           //glVertex2f(xk, yk);
        /*glVertex2f(y,x);
        glVertex2f(y,-x);
        glVertex2f(x,-y);
        glVertex2f(-x,-y);
        glVertex2f(-y,-x);
        glVertex2f(-y,x);
        glVertex2f(-x,y);*/
      }

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
    glutInitWindowSize(500, 500);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutCreateWindow("Direct Circle");
    init();
    glutDisplayFunc(display);
    glutMainLoop();

    return 0;
}
