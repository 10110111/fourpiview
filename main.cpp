#include <stdlib.h>
#include <iostream>
#include <QScreen>
#include <QApplication>
#include "MainWin.hpp"

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

    MainWin mainWin(u8"4π View", filePath);

    const auto size = app.primaryScreen()->size().height();
    mainWin.resize(size, 0.7 * size);
    mainWin.show();

    return app.exec();
}
