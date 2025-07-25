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

namespace
{
const char uiCSS[] = R"(
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

void MainWin::openFile()
{
    if(hintLabel_)
        hintLabel_->setText("");
    const QString filter = tr("Images") + " (*.jpg *.png *.tiff)";
    QString defaultDir;
    const auto file = QFileDialog::getOpenFileName(this, tr("Open image file"),
                                                   defaultDir, filter);
    if(file.isEmpty())
    {
        if(hintLabel_)
            hintLabel_->setText(tapAnywhereText_);
        return;
    }
    if(hintLabel_)
        hintLabel_->setText(tr("Loading image..."));
    update();
    QTimer::singleShot(10, [this, file]{ canvas_->openFile(file); });
}

void MainWin::closeFile()
{
    canvas_->closeImage();
    setWindowTitle(appName_);
    if(hintLabel_)
    {
        hintLabel_->setText(tapAnywhereText_);
        hintLabel_->show();
    }
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
    , tapAnywhereText_(tr("Tap anywhere to open an image"))
    , canvas_(new Canvas)
{
    setWindowTitle(appName_);
    setWindowIcon(QIcon(":icon.png"));
    setCentralWidget(canvas_);

    connect(canvas_, &Canvas::newFileRequested, this, &MainWin::openFile);

#ifdef Q_OS_ANDROID
    const auto vbox = new QVBoxLayout(canvas_);
    vbox->addStretch(1);
    hintLabel_ = new QLabel(tapAnywhereText_);
    hintLabel_->setStyleSheet(uiCSS);
    vbox->addWidget(hintLabel_);
    vbox->addStretch(1);

    connect(canvas_, &Canvas::newImageLoaded, [this]
            {
                hintLabel_->hide();
            });
#else
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
}
