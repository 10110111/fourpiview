#include <stdlib.h>
#include <iostream>
#include <QScreen>
#include <QApplication>
#include "MainWin.hpp"

namespace
{
QString uiCSS = R"(
QToolButton {
    border: 1px solid rgba(0, 0, 0, 0);
    border-radius: 24px;
    background: rgba(0,0,0,0);
    padding: 0px, 0px, 0px, 0px;
}
QToolButton:hover {
    background: rgba(0,0,0,64);
}
QToolButton:pressed {
    background: rgba(0,0,0,128);
}
QLabel {
    color: white;
    qproperty-alignment: AlignCenter;
}
)";
}

int main(int argc, char** argv)
{
    setenv("QT_IMAGEIO_MAXALLOC", "4096", false);
    QApplication app(argc, argv);
    const auto args = app.arguments();
    QString filePath;
    if(args.size() == 2)
        filePath = args[1];
    else if(args.size() != 1)
        std::cerr << "Bad number of arguments, ignoring all\n";

    app.setStyleSheet(uiCSS);
    MainWin mainWin(u8"4π View", filePath);

    const auto size = app.primaryScreen()->size().height();
    mainWin.resize(size, 0.7 * size);
    mainWin.show();

    return app.exec();
}
