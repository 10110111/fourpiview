#pragma once

#include <QMainWindow>

class QLabel;
class Canvas;
class QToolButton;
class MainWin : public QMainWindow
{
public:
    MainWin(const QString& appName, const QString& filePath, QWidget* parent = nullptr);
protected:

    void closeEvent(QCloseEvent* event) override;
private:
    void showAboutDialog();
    void openFile();
    void closeFile();

    const QString appName_;
    const QString tapAnywhereText_;
    Canvas* canvas_ = nullptr;
    QLabel* hintLabel_ = nullptr;
};
