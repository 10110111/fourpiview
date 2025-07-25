#include "MainWin.hpp"
#include <QLabel>
#include <QTimer>
#include <QAction>
#include <QMenuBar>
#include <QCloseEvent>
#include <QGridLayout>
#include <QToolButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include "Canvas.hpp"
#include "Gallery.hpp"

void MainWin::openFile()
{
    hintLabel_->hide();
    const QString filter = tr("Images") + " (*.jpg *.png *.tiff)";
    QString defaultDir;
    const auto file = QFileDialog::getOpenFileName(this, tr("Open image file"),
                                                   defaultDir, filter);
    if(file.isEmpty())
    {
        closeFile();
        return;
    }
    doOpenFile(file);
}

void MainWin::doOpenFile(const QString& path)
{
    hintLabel_->setText(tr("Loading image..."));
    hintLabel_->show();
    gallery_->hide();
    canvas_->show();
    QTimer::singleShot(10, [this, path]{ canvas_->openFile(path); });
}

void MainWin::closeFile()
{
    canvas_->closeImage();
    setWindowTitle(appName_);
    canvas_->hide();
    gallery_->show();
}

void MainWin::closeEvent(QCloseEvent* event)
{
#ifdef Q_OS_ANDROID
    if(canvas_->fileOpened())
    {
        closeFile();
        event->setAccepted(false);
    }
#else
    Q_UNUSED(event);
#endif
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
    , gallery_(new Gallery)
    , canvas_(new Canvas)
{
    setWindowTitle(appName_);
    setWindowIcon(QIcon(":icon.png"));

    const auto holder = new QWidget;
    const auto layout = new QHBoxLayout;
    holder->setLayout(layout);
    setCentralWidget(holder);

    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(gallery_);
    layout->addWidget(canvas_);

    connect(canvas_, &Canvas::newFileRequested, this, &MainWin::openFile);

    const auto vbox = new QVBoxLayout(canvas_);
    vbox->addStretch(1);
    hintLabel_ = new QLabel("");
    hintLabel_->hide();
    vbox->addWidget(hintLabel_);
    vbox->addStretch(1);

    connect(canvas_, &Canvas::newImageLoaded, [this]
            {
                hintLabel_->hide();
                gallery_->hide();
            });

    connect(gallery_, &Gallery::openFileRequest, this, &MainWin::doOpenFile);

#ifndef Q_OS_ANDROID
    const auto menuBar = this->menuBar();
    const auto aboutAction = new QAction(tr("&About"), this);
    connect(aboutAction, &QAction::triggered, this, &MainWin::showAboutDialog);

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

    const auto closeAction = new QAction(tr("&Close"), this);
    closeAction->setShortcut(QKeySequence::fromString("Ctrl+W"));
    connect(closeAction, &QAction::triggered, this, &MainWin::closeFile);
    fileMenu->addAction(closeAction);

    const auto quitAction = new QAction(tr("&Quit"), this);
    quitAction->setShortcut(QKeySequence::fromString("Ctrl+Q"));
    connect(quitAction, &QAction::triggered, []{qApp->quit();});
    fileMenu->addAction(quitAction);
#endif

    if(!filePath.isEmpty())
        QTimer::singleShot(0, [this, filePath]{canvas_->openFile(filePath);});
    else
        canvas_->hide();
}
