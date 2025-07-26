#pragma once

#include <cmath>
#include <memory>
#include <QTimer>
#include <QImage>
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>
#include <Eigen/Dense>

class ToolsWidget;
class QRotationSensor;
class Canvas : public QOpenGLWidget, public QOpenGLExtraFunctions
{
    Q_OBJECT

    enum class DragMode
    {
        None,
        Camera,
    } dragMode_=DragMode::None;
    double prevMouseX_, prevMouseY_;
    static constexpr double inline DEGREE = M_PI / 180;
    double horizViewAngle_ = 60 * DEGREE;
    double pitch_ = 0, yaw_ = 0;
    double deltaPitch_ = 0, deltaYaw_ = 0, deltaRoll_ = 0;

    GLuint vao_=0;
    GLuint vbo_=0;
    QOpenGLShaderProgram program_;
    std::unique_ptr<QOpenGLTexture> texture_;
    QImage image_;
    int viewportWidth_ = 0, viewportHeight_ = 0;
    int maxTexSize_ = 0;
    QRotationSensor* sensor_ = nullptr;
    QTimer frameTimer_;

public:
    Canvas(QWidget* parent=nullptr);
    ~Canvas();
    void setImage(const QImage& image);
    bool openFile(const QString& path);
    int maxTexSize() const { return maxTexSize_; }
    void closeImage();
    bool fileOpened() const { return texture_ || !image_.isNull(); }

signals:
    void newImageLoaded(const QString& fileName);
    void newFileRequested();

protected:
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void initializeGL() override;
    void paintGL() override;
private:
    void getViewportSize(); // This function helps avoid messing with HiDPI scaling
    void setupBuffers();
    void setupShaders();
    void setDragMode(DragMode mode, int x=0, int y=0) { dragMode_=mode; prevMouseX_=x; prevMouseY_=y; }
    Eigen::Matrix3d cameraRotation() const;
    Eigen::Vector3d calcViewDir(double screenX, double screenY) const;
};
