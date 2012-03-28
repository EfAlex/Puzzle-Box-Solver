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

//#include "box_solver.hpp"
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include "glwidget.hpp"
#include "box_solver.hpp"
#include <GL/glut.h>

box_solver gl_box_solver;

int main(int argc, char **argv)
{
    /*
    box_solver b;
    b.run();
    */

    glutInit(&argc,argv);

    QApplication a(argc, argv);
    if (!QGLFormat::hasOpenGL()) {
	QMessageBox::information(0, "OpenGL boxes",
				 "This system does not support OpenGL/framebuffer objects.");
        return -1;
    }
    QGLFormat glf = QGLFormat::defaultFormat();
    glf.setSampleBuffers(true);
    glf.setSamples(4);
    QGLFormat::setDefaultFormat(glf);
    glf = QGLFormat::defaultFormat();
    glf.setSampleBuffers(true);
    glf.setSamples(16);
    QGLFormat::setDefaultFormat(glf);
    glf = QGLFormat::defaultFormat();
    glf.setSampleBuffers(true);
    glf.setSamples(64);
    QGLFormat::setDefaultFormat(glf);

    GLWidget widget(0);
    widget.resize(640, 480);
    widget.show();

    gl_box_solver.start();
    return a.exec();
}
