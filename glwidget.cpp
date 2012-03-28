/*
   Copyright 2012 efalex <email>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


#include "glwidget.hpp"
#include <GL/glut.h>
#include <glsolution.hpp>
#include <boost/foreach.hpp>
#include "box_solver.hpp"
#include <boost/numeric/ublas/io.hpp>
#include "box_solution.hpp"

extern box_solver gl_box_solver;

GLWidget::GLWidget(QWidget * parent)
:  QGLWidget(QGLFormat(QGL::AlphaChannel | QGL::Rgba | QGL::DepthBuffer | QGL::DoubleBuffer), parent)
    ,
sphi(0.0),
stheta(0.0),
sdepth(10),
sdistance(1.0),
zNear(1.0), zFar(100.0), aspect(5.0 / 4.0), xcam(0), ycam(0), leftButton(false), middleButton(false), rightButton(false), zoomIn(false), zoomOut(false)
{
    setWindowTitle(tr("OpenGL Puzzle Box"));
    makeCurrent();

    light0Position[0] = -1;
    light0Position[1] = 1;
    light0Position[2] = 1;
    light0Position[3] = 0;
    theta[0] = 0.0;
    theta[1] = 0.0;
    theta[2] = 0.0;

    QObject::connect(&gl_box_solver, SIGNAL(drawBox()), this, SLOT(updateBox()));
    QObject::connect(this, SIGNAL(stopComputing()), &gl_box_solver, SLOT(stopComputing()));
    QObject::connect(this, SIGNAL(showStatus()), &gl_box_solver, SLOT(updateStatus()));
}

void GLWidget::initializeGL()
{
    setAutoBufferSwap(false);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glPolygonOffset(1.0, 1.0);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_COLOR_MATERIAL);

//    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_ALPHA | GLUT_MULTISAMPLE);
//    glutInitWindowSize(500, 500);
//    glutCreateWindow("rotating cube");
    GLfloat mat_colormap[] = { 16.0, 48.0, 79.0 };
    // индексы рассеянного, диффузного, зеркального цвета материала
    GLfloat mat_shininess[] = { 20.0 };
    // сила зеркального отражения материала}

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    //glDepthFunc(GL_LESS);
    glDepthFunc(GL_LEQUAL);

    //glShadeModel(GL_FLAT);
    glShadeModel(GL_SMOOTH);

    //glBlendFunc (GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    if (useAlpha) {
        //glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
        glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, mat_colormap);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
        glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
    } else {
        //glColorMaterial(GL_FRONT, GL_DIFFUSE);
        glMaterialfv(GL_FRONT, GL_COLOR_INDEXES, mat_colormap);
        glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
        glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }
    glEnable(GL_NORMALIZE);
    time.start();
    *fps_str = '\0';
}

/*void GLWidget::paintEvent(QPaintEvent *)
{
    draw();
}*/

void GLWidget::paintGL()
{
    draw();
}

void GLWidget::resizeGL(int width, int height)
{
    xsize = width;
    ysize = height;
    aspect = (float) xsize / (float) ysize;
    glViewport(0, 0, xsize, ysize);
    draw();
}

void GLWidget::draw()
{

    //display callback
    //clear frame buffer and z buffer
    //rotate cube
    //and draw

    //clear viewport buffer (color and depth buffers)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();           //replace current matrix w/ identity..
    // else see shadow/flicker of last image


    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(64.0, aspect, zNear, zFar);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glLightfv(GL_LIGHT0, GL_POSITION, light0Position);
//    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.0);
//    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.2);
//    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.4);

    glTranslatef(0.0, 0.0, -sdepth);
    glRotatef(-stheta, 1.0, 0.0, 0.0);
    glRotatef(sphi, 0.0, 0.0, 1.0);

    //multiply current matrix by rotation matrix, theta, x, y, z
    glRotatef(theta[0], 1.0, 0.0, 0.0);
    glRotatef(theta[1], 0.0, 1.0, 0.0);
    glRotatef(theta[2], 0.0, 0.0, 1.0);


    GLboolean lighting;

    glGetBooleanv(GL_LIGHTING, &lighting);

    double a = 0.5;
    if (lighting)
        glDisable(GL_LIGHTING);

    glBegin(GL_LINES);
    //glColor3d(0.5, 0, 0);
    glColor3d(1, 0, 0);
    glVertex3d(0, 0, 0);
    glVertex3d(a, 0, 0);
    glEnd();

    glBegin(GL_LINES);
    //glColor3d(0, 0.5, 0);
    glColor3d(0, 1, 0);
    glVertex3d(0, 0, 0);
    glVertex3d(0, a, 0);
    glEnd();

    glBegin(GL_LINES);
    //glColor3d(0,0,0.5);
    glColor3d(0, 0, 1);
    glVertex3d(0, 0, 0);
    glVertex3d(0, 0, a);
    glEnd();

    if (lighting)
        glEnable(GL_LIGHTING);

    BOOST_FOREACH(glsolution & o, objs) {
        o.scale = sdistance;
        o.draw();
    }

#if 0
    //glDisable(GL_BLEND);
    //glEnable(GL_DEPTH_TEST);
    glPushMatrix();
    GLfloat arr[16] = {
        0, 0, 1, 0,
        1, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 1
    };
    GLfloat *x1 = &arr[0];
    GLfloat *y1 = &arr[1];
    GLfloat *z1 = &arr[2];
    GLfloat *x2 = &arr[4];
    GLfloat *y2 = &arr[5];
    GLfloat *z2 = &arr[6];
    GLfloat *x3 = &arr[8];
    GLfloat *y3 = &arr[9];
    GLfloat *z3 = &arr[10];
    *x3 = *y1 * *z2 - *z1 * *y2;
    *y3 = *z1 * *x2 - *x1 * *z2;
    *z3 = *x1 * *y2 - *y1 * *x2;
    /*   for (int r = 0; r < 3 ; ++r) {
       for (int c = 0; c < 3; ++c) {
       printf("%4.2f ", arr[c+4*r] );
       }
       printf("\n");
       }
       printf("\n"); */

    glMultMatrixf(arr);

    //glTranslatef(sdistance * 0.0, sdistance * 0.0,sdistance * -3.0);
    glColor4f(0.5, 0.5, 0.5, 0.9);
    //SolidHalfCube();
    glColor4f(1.0, 1.0, 1.0, 0.9);
    //WiredHalfCube();

    glPopMatrix();

    /*glPushMatrix();

       glTranslatef(sdistance * 0.0, sdistance * 0.0,sdistance * -1.0);
       glColor4f(0.7, 0.7, 0.7, 0.2);
       SolidHalfCube();
       glColor4f(1.0, 1.0, 1.0, 0.4);
       WiredHalfCube();

       glPopMatrix(); */


    /*
       glPushMatrix();

       glColor4f(0.2, 0.2, 0.2, 0.9);
       glutSolidCube(1.0);
       glColor4f(0.4, 0.4, 0.4, 0.4);
       glutWireCube(1.0);

       glPopMatrix();
     */


    glPushMatrix();
    glTranslatef(sdistance * -1, sdistance * -1, sdistance * -1);

    glPushMatrix();
    glTranslatef(sdistance * 0.0, sdistance * 0.0, sdistance * 0.0);
    glColor4f(0.2, 0.2, 0.2, 0.9);
    glutSolidCube(1.0);
    glColor4f(0.4, 0.4, 0.4, 0.4);
    glutWireCube(1.0);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(sdistance * 1.0, sdistance * 0.0, sdistance * 0.0);
    glColor4f(0.5, 0.0, 0.0, 0.9);
    glutSolidCube(1.0);
    glColor4f(1.0, 0.0, 0.0, 0.9);
    glutWireCube(1.0);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(sdistance * 0.0, sdistance * 1.0, sdistance * 0.0);
    glColor4f(0.0, 0.5, 0.0, 0.9);
    glutSolidCube(1.0);
    glColor4f(0.0, 1.0, 0.0, 0.9);
    glutWireCube(1.0);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(sdistance * 0.0, sdistance * 0.0, sdistance * 1.0);
    glColor4f(0.0, 0.0, 0.5, 0.9);
    glutSolidCube(1.0);
    glColor4f(0.0, 0.0, 1.0, 0.9);
    glutWireCube(1.0);
    glPopMatrix();

    glPopMatrix();


    glPopMatrix();
    //colorcube();
#endif
    // Code to compute frames per second
    frame++;
    //time = glutGet(GLUT_ELAPSED_TIME);
    if (time.elapsed() >= 10) {
        snprintf(fps_str, sizeof(fps_str), "FPS: %6.2f", frame * 1000.0 / time.elapsed());
        time.restart();
        frame = 0;
    }
    //printf("%s\n",s);
    setOrthographicProjection();
    void *font = GLUT_BITMAP_8_BY_13;
    glPushMatrix();
    glLoadIdentity();
    glColor3f(1.0, 1.0, 1.0);
    renderBitmapString(30, 30, 0, font, fps_str);
    glPopMatrix();

    restorePerspectiveProjection();

    //glFlush();                  //probably a good idea but no effect on this program

    //glutSwapBuffers();          //put image to be drawn in buffer for display
    swapBuffers();

}

void GLWidget::mousePressEvent(QMouseEvent * e)
{
    anchor = e->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent * e)
{
    QPoint diff = e->pos() - anchor;
    if (e->buttons() & Qt::LeftButton) {
        sphi += (float) diff.x() / 4.0;
        stheta -= (float) diff.y() / 4.0;
    } else if (e->buttons() & Qt::MiddleButton) {
        sdepth -= (float) diff.y() / 10.0;
    } else if (e->buttons() & Qt::RightButton) {
        sdistance -= (float) diff.y() / 20.0;
        if (sdistance < 1) {
            sdistance = 1;
        }
        if (sdistance > 10) {
            sdistance = 10;
        }
    }

    anchor = e->pos();
    draw();
}

void GLWidget::wheelEvent(QWheelEvent * e)
{
    e->delta() > 0 ? sdepth += 0.1f : sdepth -= 0.1f;
    draw();
}

void GLWidget::restorePerspectiveProjection()
{

    glMatrixMode(GL_PROJECTION);
    // restore previous projection matrix
    glPopMatrix();

    // get back to modelview mode
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    if (useAlpha) {
        glEnable(GL_BLEND);
    }
}

void GLWidget::setOrthographicProjection()
{

    // switch to projection mode
    glMatrixMode(GL_PROJECTION);

    // save previous matrix which contains the
    //settings for the perspective projection
    glPushMatrix();

    // reset matrix
    glLoadIdentity();

    // set a 2D orthographic projection
    gluOrtho2D(0, xsize, ysize, 0);

    // switch back to modelview mode
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    if (useAlpha) {
        glDisable(GL_BLEND);
    }
}

void GLWidget::renderBitmapString(float x, float y, float z, void *font, char *string)
{

    char *c;
    glRasterPos3f(x, y, z);
    for (c = string; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}

void GLWidget::keyPressEvent(QKeyEvent * e)
{
    switch (e->key()) {
        case Qt::Key_S:
            emit showStatus();
            break;
        case Qt::Key_A:
            useAlpha = !useAlpha;
            initializeGL();
            draw();
            break;
        case Qt::Key_Left:
            if (box_solution::solution_list.size()) {
                if (gl_pos > 0) {
                    gl_pos--;
                }
                box_solution::solution_list[gl_pos].to_vector(gl_solution, gl_solution_pos);
                gl_count = gl_solution.size();
                std::cout << "Solution: " << gl_pos << std::endl;
                std::cout << box_solution::solution_list[gl_pos] << std::endl;
                updateBox();
            }
            break;
        case Qt::Key_Right:
            if (box_solution::solution_list.size()) {
                if (gl_pos < box_solution::solution_list.size() - 1) {
                    gl_pos++;
                }
                box_solution::solution_list[gl_pos].to_vector(gl_solution, gl_solution_pos);
                gl_count = gl_solution.size();
                std::cout << "Solution: " << gl_pos << std::endl;
                std::cout << box_solution::solution_list[gl_pos] << std::endl;
                updateBox();
            }
            break;
        case Qt::Key_Escape:
            close();
            break;
        default:
            QGLWidget::keyPressEvent(e);
            break;
    }
}

void GLWidget::closeEvent(QCloseEvent * event)
{
    emit stopComputing();
    gl_box_solver.wait();
    event->accept();
}

void GLWidget::updateBox()
{
    objs.resize(0);
    glsolution *sol = new glsolution(gl_count, gl_solution, gl_solution_pos);
    objs.push_back(sol);
    draw();
}

bool GLWidget::useAlpha = true;
unsigned int GLWidget::gl_pos = 0;
unsigned int GLWidget::gl_count = 0;
std::vector < figure > GLWidget::gl_solution(0);
std::vector < vector_int > GLWidget::gl_solution_pos(0);

