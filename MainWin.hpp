#pragma once

#include <QMainWindow>

class Canvas;
class MainWin : public QMainWindow
{
public:
    MainWin(const QString& appName, const QString& filePath, QWidget* parent = nullptr);
private:
    void showAboutDialog();
    void openFile();

    const QString appName_;
    Canvas* canvas_;
};
