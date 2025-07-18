#pragma once

#include <QMainWindow>

class QLabel;
class Canvas;
class QToolButton;
class MainWin : public QMainWindow
{
public:
    MainWin(const QString& appName, const QString& filePath, QWidget* parent = nullptr);
private:
    void showAboutDialog();
    void openFile();

    const QString appName_;
    const QString tapAnywhereText_;
    Canvas* canvas_ = nullptr;
    QLabel* hintLabel_ = nullptr;
    QToolButton* closeFileButton_ = nullptr;
};
