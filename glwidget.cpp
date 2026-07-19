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
#include "coregl.hpp"
#include <glsolution.hpp>
#include <boost/foreach.hpp>
#include "box_solver.hpp"
#include "box_solution.hpp"
#include "glcube.hpp"
#include "glhalfcube.hpp"

extern box_solver gl_box_solver;

GLWidget::GLWidget(QWidget * parent)
: QOpenGLWidget(parent)
    ,
sphi(0.0),
stheta(0.0),
sdepth(10),
sdistance(1.0),
zNear(1.0), zFar(100.0), aspect(5.0 / 4.0), xcam(0), ycam(0), leftButton(false), middleButton(false), rightButton(false), zoomIn(false), zoomOut(false)
{
    setWindowTitle(tr("OpenGL Puzzle Box"));

    light0Position[0] = -1;
    light0Position[1] = 1;
    light0Position[2] = 1;
    light0Position[3] = 0;
    theta[0] = 0.0;
    theta[1] = 0.0;
    theta[2] = 0.0;
    frame = 0;
    debugMode_ = false;

    QObject::connect(&gl_box_solver, SIGNAL(drawBox()), this, SLOT(updateBox()));
    QObject::connect(this, SIGNAL(stopComputing()), &gl_box_solver, SLOT(stopComputing()));
    QObject::connect(this, SIGNAL(showStatus()), &gl_box_solver, SLOT(updateStatus()));

    time.start();  // start before any resize/draw can fire (fixes valgrind uninit timer)
}

GLWidget::~GLWidget()
{
    delete coregl_;
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    const char* version = (const char*)glGetString(GL_VERSION);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* vendor = (const char*)glGetString(GL_VENDOR);

    qDebug() << "OpenGL Version:" << (version ? version : "Unknown");
    qDebug() << "OpenGL Renderer:" << (renderer ? renderer : "Unknown");
    qDebug() << "OpenGL Vendor:" << (vendor ? vendor : "Unknown");

    // Enable depth testing and face culling for proper 3D rendering
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);

    // Auto-buffer swap is handled by QOpenGLWidget
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glPolygonOffset(1.0, 1.0);
    // Depth test (used by shader rendering)
    glDepthFunc(GL_LEQUAL);

    // Shade model (affects fragment interpolation in shaders too)
    glShadeModel(GL_SMOOTH);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    memset(fps_str, 0, sizeof(fps_str));

    // Neon mode defaults (from Tetris3D: glowIntensity=0.2, bg=0.05,0.05,0.08)
    m_neonMode = true;
    m_neonIntensity = 0.2f;
    m_lightPosition[0] = 0.5f;
    m_lightPosition[1] = 1.0f;
    m_lightPosition[2] = 0.3f;

    // Initialize OpenGL debug logger
    m_debugLogger = new QOpenGLDebugLogger(this);
    if (m_debugLogger->initialize()) {
        connect(m_debugLogger, &QOpenGLDebugLogger::messageLogged,
                this, &GLWidget::onMessageLogged);

        m_debugLogger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
        m_debugLogger->enableMessages();
    }

    // Phase A: create shader + VAO manager (coregl)
    coregl_ = new CoreGL();
    coregl_->init();

    if (!coregl_->isValid()) {
        qCritical("CoreGL: initialization failed. OpenGL context may not be properly set up.");
        return;
    }


    // Set up initial projection and view matrices
    setupProjectionMatrix();
}


void GLWidget::paintGL()
{
    // Always clear the framebuffer here — draw() may return early
    // (e.g., invalid viewport) and without this clear the old frame
    // persists as "ghosts" visible in neon mode with additive blending.
    // Disable blending during clear to avoid additive accumulation artifacts
    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);

    draw();
}

void GLWidget::resizeGL(int width, int height)
{
    xsize = width;
    ysize = height;
    aspect = (float) xsize / (float) ysize;
    glViewport(0, 0, xsize, ysize);

    // Update projection matrix on resize
    setupProjectionMatrix();

    draw();
}

void GLWidget::setupProjectionMatrix()
{
    // Create a proper perspective projection matrix
    QMatrix4x4 projection;
    projection.perspective(45.0f, aspect, 0.1f, 100.0f);

    // Create a view matrix (camera position)
    QMatrix4x4 view;
    view.translate(0.0f, 0.0f, -5.0f);  // Move camera back

    // Set both matrices in CoreGL
    if (coregl_) {
        coregl_->setMatricesPv(projection, view);
    }
}

void GLWidget::draw()
{
    //if (!QWindow::isExposed())
    //    return;

    //display callback
    //clear frame buffer and z buffer
    //rotate cube
    //and draw

    // Validate viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    if (debugMode_) {
        qInfo() << "Viewport: x=" << viewport[0] << " y=" << viewport[1]
                << " width=" << viewport[2] << " height=" << viewport[3];
    }

    if (viewport[2] <= 0 || viewport[3] <= 0) {
        qWarning() << "ERROR: Invalid viewport size!";
        return;
    }

    // Create a proper perspective projection matrix
    QMatrix4x4 projection;
    projection.perspective(45.0f, aspect, 0.1f, 100.0f);

    // Create a view matrix — camera position only (no rotation; rotation is on the model)
    QMatrix4x4 view;
    view.translate(0.0f, 0.0f, -sdepth);

    // Check OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        qWarning() << "OpenGL Error before draw:" << error;
    }

    // Background: dark blue-gray in neon mode (Tetris3D), black in solid
    if (m_neonMode) {
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    } else {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }
    glDepthMask(GL_TRUE);

    // Disable blending during clear to avoid additive accumulation artifacts
    glDisable(GL_BLEND);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);

    // Blend mode: additive for neon (glow accumulation), standard alpha for solid
    if (m_neonMode) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);              // Additive blending for glow
        glDisable(GL_CULL_FACE);                        // Disable culling — needed for transparent geometry
    } else {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard alpha blending
        // Note: culling disabled for transparent figures (alpha=0.2) — faces disappear if culled
        glDisable(GL_CULL_FACE);
    }

    // Pass matrices to CoreGL for use by objects
    coregl_->setMatricesPv(projection, view);

    // Build model matrix: rotation (stheta on X, sphi on Z) + identity translation
    // Matches qt-fix: glRotatef(-stheta, 1,0,0); glRotatef(sphi, 0,0,1);
    QMatrix4x4 modelMat;
    modelMat.setToIdentity();
    modelMat.rotate(-stheta, 1.0f, 0.0f, 0.0f);
    modelMat.rotate(sphi, 0.0f, 0.0f, 1.0f);
    coregl_->setModelMatrix(modelMat);

    // Pass light position uniform (from Tetris3D: configurable light direction)
    coregl_->setLightPosition(m_lightPosition[0], m_lightPosition[1], m_lightPosition[2]);
    // Pass rendering mode uniform (0 = neon, 1 = solid)
    coregl_->setRenderingMode(m_neonMode ? 0 : 1);
    // Pass neon intensity uniform
    coregl_->setNeonIntensity(m_neonIntensity);


    // Reset instance counters / collect instance data — unless updateBox()
    // already did this on the solver thread (m_dataReady).  Without this
    // guard the solver thread's data gets overwritten by a stale draw()
    // call, causing "ghost" artifacts in neon mode.
    if (!m_dataReady.load(std::memory_order_acquire)) {
        coregl_->resetInstanceCounters();

        BOOST_FOREACH(globject & o, objs) {
            o.draw();
        }

        // Set instance data for all objects at once
        if (coregl_->cube_count + coregl_->halfcube_count > 0) {
            if (coregl_->cube_count > 0) {
                coregl_->setCubeInstanceData(coregl_->cube_positions, coregl_->cube_colors, coregl_->cube_rotations, coregl_->cube_count);
            }
            if (coregl_->halfcube_count > 0) {
                coregl_->setHalfCubeInstanceData(coregl_->halfcube_positions, coregl_->halfcube_colors, coregl_->halfcube_rotations, coregl_->halfcube_count);
            }
        }
    }
    m_dataReady.store(false, std::memory_order_release);

    // Draw
    if (coregl_->cube_count > 0) {
        coregl_->drawCubes(coregl_->cube_count);
    }
    if (coregl_->halfcube_count > 0) {
        coregl_->drawHalfCubes(coregl_->halfcube_count);
    }

    frame++;
    if (time.elapsed() >= 10) {
        snprintf(fps_str, sizeof(fps_str), "FPS: %6.2f", frame * 1000.0 / time.elapsed());
        time.restart();
        frame = 0;
    }

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

        // Clamp vertical rotation to -180 to 180
        const float maxAngle = 180.0f;
        if (stheta > maxAngle) {
            stheta = maxAngle;
        } else if (stheta < -maxAngle) {
            stheta = -maxAngle;
        }

        // Clamp horizontal rotation to prevent extreme wrapping
        const float maxPhi = 360.0f;
        if (sphi > maxPhi) {
            sphi -= 360.0f;
        } else if (sphi < -maxPhi) {
            sphi += 360.0f;
        }
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
        if (coregl_) {
            coregl_->setScaleFactor(sdistance);
        }
    }

    anchor = e->pos();
    update();
}

void GLWidget::wheelEvent(QWheelEvent * e)
{
    e->angleDelta().y() > 0 ? sdepth += 0.1f : sdepth -= 0.1f;
    update();
}


void GLWidget::keyPressEvent(QKeyEvent * e)
{
    switch (e->key()) {
        case Qt::Key_S:
            emit showStatus();
            break;
        case Qt::Key_A:
            // Toggle neon mode (from Tetris3D: A key toggles between neon glow and solid lit)
            m_neonMode = !m_neonMode;
            update();
            break;
        case Qt::Key_D:
            if (coregl_) {
                static int debugMode = 0;
                debugMode = (debugMode + 1) % 4;
                coregl_->setDebugMode(debugMode);
                update();
            }
            break;
        case Qt::Key_Left:
            if (!box_solution::empty()) {
                if (gl_pos > 0) {
                    gl_pos--;
                }
                box_solution::solution_list[gl_pos].to_vector(gl_solution, gl_solution_pos);
                gl_count = gl_solution.size();
                updateBox();
            }
            break;
        case Qt::Key_Right:
            if (!box_solution::empty()) {
                if (gl_pos < box_solution::solution_list_size() - 1) {
                    gl_pos++;
                }
                box_solution::solution_list[gl_pos].to_vector(gl_solution, gl_solution_pos);
                gl_count = gl_solution.size();
                updateBox();
            }
            break;
        case Qt::Key_Escape:
            close();
            break;
        default:
            QWidget::keyPressEvent(e);
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
    // Snapshot shared state under mutex to avoid data race with worker thread.
    unsigned int count;
    std::vector<figure> solution;
    std::vector<vector_int> solution_pos;
    {
        std::lock_guard<std::mutex> lock(glSolutionMutex);
        count = gl_count;
        solution = gl_solution;
        solution_pos = gl_solution_pos;
    }

    objs.clear();
    glsolution *sol = new glsolution(count, solution, solution_pos);
    sol->propagateCoreGL(coregl_);
    objs.push_back(sol);

    if (objs.empty()) {
        qWarning() << "No objects created in updateBox!";
        return;
    }

    // Collect instance data on the solver thread BEFORE update().
    // This avoids a data race: without this, paintGL() on the main thread
    // reads instance data arrays that are still being written by the solver
    // thread, causing stale geometry to render as "ghosts" (visible with
    // additive blending in neon mode).
    coregl_->resetInstanceCounters();
    BOOST_FOREACH(globject & o, objs) {
        o.draw();
    }

    // OpenGL context is only current inside paintGL()/initializeGL()/resizeGL().
    // updateBox() is called from solver thread via queued signal — context NOT current.
    makeCurrent();
    if (coregl_->cube_count > 0) {
        coregl_->setCubeInstanceData(coregl_->cube_positions, coregl_->cube_colors, coregl_->cube_rotations, coregl_->cube_count);
    }
    if (coregl_->halfcube_count > 0) {
        coregl_->setHalfCubeInstanceData(coregl_->halfcube_positions, coregl_->halfcube_colors, coregl_->halfcube_rotations, coregl_->halfcube_count);
    }
    m_dataReady.store(true, std::memory_order_release);
    doneCurrent();

    update();
}

void GLWidget::onMessageLogged(const QOpenGLDebugMessage &message)
{
    // We only care about actual errors, not performance warnings or notifications
    if (message.severity() == QOpenGLDebugMessage::HighSeverity ||
        message.type() == QOpenGLDebugMessage::ErrorType) {

        qCritical() << "GL ERROR:" << message.message();
        qCritical() << "GL ERROR ID:" << message.id();
    }
}

bool GLWidget::useAlpha = true;
unsigned int GLWidget::gl_pos = 0;
unsigned int GLWidget::gl_count = 0;
std::vector < figure > GLWidget::gl_solution(0);
std::vector < vector_int > GLWidget::gl_solution_pos(0);
std::mutex GLWidget::glSolutionMutex;

void GLWidget::setGLSolution(unsigned int count, const std::vector<figure> &sol,
                             const std::vector<vector_int> &sol_pos)
{
    std::lock_guard<std::mutex> lock(glSolutionMutex);
    gl_count = count;
    gl_solution = sol;
    gl_solution_pos = sol_pos;
}

