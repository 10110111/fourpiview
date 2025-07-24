#include "MainWin.hpp"
#include <QLabel>
#include <QTimer>
#include <QAction>
#include <QMenuBar>
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
    hintLabel_->setText("");
    const QString filter = tr("Images") + " (*.jpg *.png *.tiff)";
    QString defaultDir;
    const auto file = QFileDialog::getOpenFileName(this, tr("Open image file"),
                                                   defaultDir, filter);
    if(file.isEmpty()) return;
    hintLabel_->setText(tr("Loading image..."));
    update();
    QTimer::singleShot(10, [this, file]{ canvas_->openFile(file); });
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
    closeFileButton_ = new QToolButton;
    closeFileButton_->setStyleSheet(uiCSS);
    closeFileButton_->setIcon(QIcon(":cross.svg"));
    closeFileButton_->setFixedSize(48,48);
    closeFileButton_->setIconSize(QSize(16,16));
    closeFileButton_->hide();
    const auto vbox = new QVBoxLayout(canvas_);
    const auto hbox = new QHBoxLayout;
    vbox->addLayout(hbox);
    vbox->addStretch(1);
    hintLabel_ = new QLabel(tapAnywhereText_);
    hintLabel_->setStyleSheet(uiCSS);
    vbox->addWidget(hintLabel_);
    vbox->addStretch(1);
    hbox->addWidget(closeFileButton_);
    hbox->addStretch(1);

    connect(closeFileButton_, &QAbstractButton::clicked, this,
            [this]
            {
                canvas_->closeImage();
                setWindowTitle(appName_);
                hintLabel_->setText(tapAnywhereText_);
                hintLabel_->show();
                closeFileButton_->hide();
            });
    connect(canvas_, &Canvas::newImageLoaded, [this]
            {
                hintLabel_->hide();
                closeFileButton_->show();
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

    const auto quitAction = new QAction(tr("&Quit"), this);
    quitAction->setShortcut(QKeySequence::fromString("Ctrl+Q"));
    connect(quitAction, &QAction::triggered, []{qApp->quit();});
    fileMenu->addAction(quitAction);
#endif

    if(!filePath.isEmpty())
        QTimer::singleShot(0, [this, filePath]{canvas_->openFile(filePath);});
}
