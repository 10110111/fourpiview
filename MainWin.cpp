#include "MainWin.hpp"
#include <QTimer>
#include <QAction>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include "Canvas.hpp"

void MainWin::openFile()
{
    const QString filter = tr("Images") + " (*.jpg *.png *.tiff)";
    QString defaultDir;
    const auto file = QFileDialog::getOpenFileName(this, tr("Open image file"),
                                                   defaultDir, filter);
    if(file.isEmpty()) return;
    canvas_->openFile(file);
}

void MainWin::showAboutDialog()
{
    const auto defaultFontSize = QFontMetrics(font()).ascent();
    const int bigFontSize = defaultFontSize * 1.3;
    const auto aboutText = tr(
        "<p style=\"font-size: %1px\"><b>4&pi; View</b> v%2</p>"
        "360&deg; panorama viewer<br>"
        "Built with Qt %3<br>"
        "Running with Qt %4<br>"
        "GL_MAX_TEXTURE_SIZE: %5<br>"
        );
    QMessageBox::information(this, tr("About"),
                             aboutText.arg(bigFontSize)
                                      .arg(PROJECT_VERSION)
                                      .arg(QT_VERSION_STR)
                                      .arg(qVersion())
                                      .arg(canvas_->maxTexSize()));
}

MainWin::MainWin(const QString& appName, const QString& filePath, QWidget* parent)
    : QMainWindow(parent)
    , appName_(appName)
    , canvas_(new Canvas)
{
    setWindowTitle(appName_);
    setCentralWidget(canvas_);

    connect(canvas_, &Canvas::newFileRequested, this, &MainWin::openFile);

    const auto menuBar = this->menuBar();
    const auto aboutAction = new QAction(tr("&About"), this);
    connect(aboutAction, &QAction::triggered, this, &MainWin::showAboutDialog);
#ifdef Q_OS_ANDROID
    const auto fileMenu = menuBar->addMenu(tr("File"));
    const auto openAction = new QAction(tr("Open File"), this);
    connect(openAction, &QAction::triggered, this, &MainWin::openFile);
    fileMenu->addAction(openAction);
    fileMenu->addAction(aboutAction);
#else
    connect(canvas_, &Canvas::newImageLoaded,
            [this](const QString& fileName)
            { setWindowTitle(fileName + " - " + appName_); });

    const auto fileMenu = menuBar->addMenu(tr("&File"));
    const auto helpMenu = menuBar->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);

    const auto openAction = new QAction(tr("&Open"), this);
    openAction->setShortcut(QKeySequence::fromString("Ctrl+O"));
    connect(openAction, &QAction::triggered, this, &MainWin::openFile);
    fileMenu->addAction(openAction);

    const auto quitAction = new QAction(tr("&Quit"), this);
    quitAction->setShortcut(QKeySequence::fromString("Ctrl+Q"));
    connect(quitAction, &QAction::triggered, []{qApp->quit();});
    fileMenu->addAction(quitAction);
#endif

    if(!filePath.isEmpty())
        QTimer::singleShot(0, [&]{canvas_->openFile(filePath);});

}
