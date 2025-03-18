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
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QGuiApplication>         // for AA_EnableHighDpiScaling
#include <QSurfaceFormat>
#include "glwidget.hpp"
#include "box_solver.hpp"

box_solver gl_box_solver;

int main(int argc, char **argv)
{
    // High-DPI support (from Tetris3D) — MUST be before QApplication
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);

    QSurfaceFormat fmt;
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(3, 3);
    fmt.setSamples(4);                  // 4x MSAA antialiasing (from Tetris3D)
    fmt.setStencilBufferSize(-1);       // stencil buffer for future use
    fmt.setOption(QSurfaceFormat::DebugContext);
    QSurfaceFormat::setDefaultFormat(fmt);

#ifdef Q_OS_LINUX
    qputenv("QT_OPENGL", "desktop");
#endif

    GLWidget widget(0);
    widget.resize(640, 480);
    widget.show();

    gl_box_solver.start();
    return a.exec();
}
