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
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QPoint>
#include <QOpenGLDebugLogger>
#include "glsolution.hpp"
#include "box_solver.hpp"
#include "coregl.hpp"

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    GLWidget(QWidget *parent);
    ~GLWidget();

    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent *);
    void closeEvent ( QCloseEvent * event );

    static bool useAlpha;
    static unsigned int gl_count, gl_pos;
    static std::vector < figure > gl_solution;
    static std::vector < vector_int > gl_solution_pos;
    static std::mutex glSolutionMutex;
    static void setGLSolution(unsigned int count, const std::vector<figure> &sol,
                              const std::vector<vector_int> &sol_pos);

signals:
    void stopComputing();
    void showStatus();
public slots:
    void draw();
private slots:
    void updateBox();
    void onMessageLogged(const QOpenGLDebugMessage &message);
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

    int frame;
    QElapsedTimer time;
    char fps_str[100];

    CoreGL *coregl_;                    // shader + VAO manager (Phase A)
    QMatrix4x4 viewMatrix_;             // View matrix for 3D rendering
    QOpenGLDebugLogger* m_debugLogger;  // OpenGL debug logger
    bool debugMode_;                    // Debug mode flag

    /* Neon mode (from Tetris3D) */
    bool m_neonMode = true;             // true = neon glow, false = solid lit
    float m_neonIntensity = 0.2f;       // glow brightness (from Tetris3D)
    float m_lightPosition[3] = {0.5f, 1.0f, 0.3f};  // configurable light direction

    /* Instance data ready flag — set in updateBox() on solver thread, checked in draw() */
    std::atomic<bool> m_dataReady{false};

    void restorePerspectiveProjection();
    void setOrthographicProjection();
    void renderBitmapString(
        float x,
        float y,
        float z,
        void *font,
        char *string);
    void setupProjectionMatrix();       // Projection matrix setup function
public:
    void setDebugMode(bool debug) { debugMode_ = debug; }
};

