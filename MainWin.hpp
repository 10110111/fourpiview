#pragma once

#include <QMainWindow>

class QLabel;
class Canvas;
class Gallery;
class QToolButton;
class MainWin : public QMainWindow
{
public:
    MainWin(const QString& appName, const QString& filePath, QWidget* parent = nullptr);
protected:

    void closeEvent(QCloseEvent* event) override;
private:
    void showAboutDialog();
    void doOpenFile(const QString& path);
    void openFile();
    void closeFile();

    const QString appName_;
    Gallery* gallery_ = nullptr;
    Canvas* canvas_ = nullptr;
    QLabel* hintLabel_ = nullptr;
};
