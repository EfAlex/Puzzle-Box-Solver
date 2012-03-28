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


#include "common.hpp"
#include <QtOpenGL>
#include "glsolution.hpp"
#include "box_solver.hpp"

class GLWidget : public QGLWidget
{
    Q_OBJECT

public:
    GLWidget(QWidget *parent);

    //void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);
    //void mouseDoubleClickEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    //void timerEvent(QTimerEvent *);
    void wheelEvent(QWheelEvent *);
    void closeEvent ( QCloseEvent * event );

    static bool useAlpha;
    static unsigned int gl_count, gl_pos;
    static std::vector < figure > gl_solution;
    static std::vector < vector_int > gl_solution_pos;

signals:
    void stopComputing();
    void showStatus();
public slots:
    void draw();
private slots:
    void updateBox();
protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    void keyPressEvent(QKeyEvent *);
private:
    boost::ptr_vector <glsolution> objs;
    QPoint anchor;
    /* Viewer state */
    GLfloat theta[3];
    float sphi, stheta;
    float sdepth;
    float sdistance;
    float zNear, zFar;
    float aspect;
    float xcam, ycam;
    long xsize, ysize;
    int downX, downY;
    bool leftButton, middleButton, rightButton, zoomIn, zoomOut;
    GLfloat light0Position[4];

// variables to compute frames per second
    int frame;
    QTime time;
    char fps_str[100];

    void restorePerspectiveProjection();
    void setOrthographicProjection();
    void renderBitmapString(
        float x,
        float y,
        float z,
        void *font,
        char *string);
};

