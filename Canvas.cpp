#include "Canvas.hpp"
#include <QDebug>
#include <QFileInfo>
#include <QMimeData>
#include <QMessageBox>
#include <QMouseEvent>
#include <QImageReader>
#include <QRotationSensor>

namespace
{

QSurfaceFormat makeGLSurfaceFormat()
{
    QSurfaceFormat format;
    if(QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL)
    {
        format.setVersion(3,3);
        format.setProfile(QSurfaceFormat::CoreProfile);
    }
    else
    {
        format.setVersion(3,0);
    }
    return format;
}

QByteArray prependGLSLVersion(const char*const src)
{
    QByteArray out;
    if(QOpenGLContext::currentContext()->isOpenGLES())
        out = "#version 300 es\n"
              "precision highp float;\n";
    else
        out = "#version 330\n";
    out += src;
    return out;
}

QPoint position(QMouseEvent* event)
{
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    return event->pos();
#else
    return event->position().toPoint();
#endif
}

inline QMatrix3x3 toQMatrix(Eigen::Matrix3d const& m)
{
    const Eigen::Matrix3f mf = m.cast<float>().transpose();
    return QMatrix3x3(mf.data());
}

double normalizedAngle(double angle)
{
    while(angle > M_PI)
        angle -= M_PI*2;
    while(angle < -M_PI)
        angle += M_PI*2;
    return angle;
}

}

Canvas::Canvas(QWidget* parent)
    : QOpenGLWidget(parent)
    , sensor_(new QRotationSensor(this))
{
    setFormat(makeGLSurfaceFormat());
    setAcceptDrops(true);
    const int freq = 1000 / 60;
    frameTimer_.start(freq);
    connect(&frameTimer_, &QTimer::timeout, this, qOverload<>(&Canvas::update));
    sensor_->setDataRate(2 * freq);
    if(sensor_->connectToBackend())
    {
        qDebug() << "Sensor connected to backend, starting";
        sensor_->start();
    }
    else
    {
        qDebug() << "Failed to connect sensor to backend";
    }
}

void Canvas::setupBuffers()
{
    if(!vao_)
        glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    if(!vbo_)
        glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    const GLfloat vertices[]=
    {
        -1, -1,
         1, -1,
        -1,  1,
         1,  1,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);
    constexpr GLuint attribIndex=0;
    constexpr int coordsPerVertex=2;
    glVertexAttribPointer(attribIndex, coordsPerVertex, GL_FLOAT, false, 0, 0);
    glEnableVertexAttribArray(attribIndex);
    glBindVertexArray(0);
}

void Canvas::setupShaders()
{
    const auto vertSrc = prependGLSLVersion(R"(
in vec3 vertex;
out vec3 position;
void main()
{
    position=vertex;
    gl_Position=vec4(vertex,1);
}
)");
    if(!program_.addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc))
        QMessageBox::critical(nullptr, tr("Error compiling shader"),
                              tr("Failed to compile %1:\n%2").arg("vertex shader").arg(program_.log()));

    const auto fragSrc = prependGLSLVersion(R"(
uniform sampler2D tex;
in vec3 position;
out vec4 color;

uniform float horizViewAngle;
uniform float viewportAspectRatio;
uniform mat3 cameraRotation;

const float PI = 3.1415926535897932;

vec3 calcViewDir()
{
    vec2 pos=position.xy;
    float camDistToScreen = 1. / tan(horizViewAngle/2.);
    pos.y /= viewportAspectRatio;
    return cameraRotation * normalize(vec3(-camDistToScreen, pos));
}

void main()
{
    vec3 viewDir = calcViewDir();
    float elevation = asin(clamp(viewDir.z, -1., 1.));
    float azimuth = atan(viewDir.y, viewDir.x);
    vec2 texc = vec2(-azimuth / (2.*PI),
                    -elevation / PI + 0.5);
    // The usual automatic computation of derivatives of texture coordinates
    // breaks down at the discontinuity of atan, resulting in choosing the most
    // minified mip level instead of the correct one, which looks as a seam on
    // the screen. Thus, we need to compute them in a custom way, treating atan
    // as a (continuous) multivalued function. We differentiate
    // atan(viewDirX(x,y), viewDirY(x,y)) with respect to x and y and yield
    // gradLongitude vector.
    vec2 gradViewDirX = vec2(dFdx(viewDir.x), dFdy(viewDir.x));
    vec2 gradViewDirY = vec2(dFdx(viewDir.y), dFdy(viewDir.y));
    vec2 gradLongitude = vec2(viewDir.y*gradViewDirX.s-viewDir.x*gradViewDirY.s,
                              viewDir.y*gradViewDirX.t-viewDir.x*gradViewDirY.t)
                                                        /
                                             dot(viewDir, viewDir);
    float texTdx = dFdx(texc.t);
    float texTdy = dFdy(texc.t);
    vec2 texDx = vec2(gradLongitude.s/(2.*PI), texTdx);
    vec2 texDy = vec2(gradLongitude.t/(2.*PI), texTdy);
    color = textureGrad(tex, texc, texDx, texDy);
}
)");
    if(!program_.addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc))
        QMessageBox::critical(nullptr, tr("Error compiling shader"),
                              tr("Failed to compile %1:\n%2").arg("fragment shader").arg(program_.log()));
    if(!program_.link())
    {
        QMessageBox::critical(nullptr, tr("Error linking shader program"),
                              tr("Failed to link %1:\n%2").arg("shader program").arg(program_.log()));
    }
    else
    {
        qDebug() << "Shaders compiled and linked successfully";
    }
}

void Canvas::initializeGL()
{
    initializeOpenGLFunctions();
    getViewportSize();

    setupBuffers();
    setupShaders();

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize_);
    qDebug() << "GL_MAX_TEXTURE_SIZE:" << maxTexSize_;

    glFinish();
}

void Canvas::getViewportSize()
{
    GLint vp[4] = {};
    glGetIntegerv(GL_VIEWPORT, vp);
    viewportWidth_ = vp[2];
    viewportHeight_ = vp[3];
}

Canvas::~Canvas()
{
    makeCurrent();
}

Eigen::Matrix3d Canvas::cameraRotation() const
{
    using namespace Eigen;
    Matrix3d m;
    m = AngleAxisd(yaw_ + deltaYaw_, Vector3d::UnitZ()) *
        AngleAxisd(pitch_ + deltaPitch_, -Vector3d::UnitY()) *
        AngleAxisd(deltaRoll_, Vector3d::UnitX());
    return m;
}

Eigen::Vector3d Canvas::calcViewDir(const double screenX, const double screenY) const
{
    const auto x = screenX / viewportWidth_ * 2 - 1;
    const auto y = 1 - screenY / viewportWidth_ * 2; // width instead of height takes aspect ratio into account
    const float camDistToScreen = 1 / std::tan(horizViewAngle_ / 2);
    return cameraRotation() * Eigen::Vector3d(-camDistToScreen, x, y).normalized();
}

void Canvas::mouseMoveEvent(QMouseEvent*const event)
{
    const auto pos = QPointF(::position(event)) * devicePixelRatio();
    switch(dragMode_)
    {
    case DragMode::Camera:
    {
        const auto dir0 = calcViewDir(viewportWidth_ / 2., viewportHeight_ / 2.);
        const auto dir1 = calcViewDir(viewportWidth_ / 2. + 1, viewportHeight_ / 2. + 1);
        const auto anglePerPixel = std::acos(std::min(1., dir0.dot(dir1)));
        const auto deltaYaw = -(prevMouseX_-pos.x())*anglePerPixel;
        const auto deltaPitch = (prevMouseY_-pos.y())*anglePerPixel;
        pitch_ = std::clamp(pitch_ + deltaPitch, -M_PI/2, M_PI/2);
        yaw_ = std::remainder(yaw_ + deltaYaw, 2*M_PI);
        break;
    }
    default:
        break;
    }
    prevMouseX_=pos.x();
    prevMouseY_=pos.y();
    update();
}

void Canvas::mousePressEvent(QMouseEvent*const event)
{
    if(!texture_ && event->button() == Qt::LeftButton)
    {
        // Tapped an empty space
        emit newFileRequested();
    }

    const auto pos = QPointF(::position(event)) * devicePixelRatio();
    switch(event->button())
    {
    case Qt::LeftButton:
        setDragMode(DragMode::Camera, pos.x(), pos.y());
        break;
    default:
        break;
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent*)
{
    setDragMode(DragMode::None);
}

void Canvas::wheelEvent(QWheelEvent*const event)
{
    const auto stepSize = event->modifiers() & Qt::ShiftModifier ? 1*DEGREE : 5*DEGREE;
    const auto step = -stepSize * event->angleDelta().y()/120.;
    horizViewAngle_ += step;
    constexpr double minViewAngle = 5 * DEGREE;
    constexpr double maxViewAngle = 160 * DEGREE;
    if(horizViewAngle_ <= minViewAngle) horizViewAngle_ = minViewAngle;
    if(horizViewAngle_ > maxViewAngle) horizViewAngle_ = maxViewAngle;
    update();
}

void Canvas::dragEnterEvent(QDragEnterEvent*const event)
{
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void Canvas::dropEvent(QDropEvent*const event)
{
    const auto urls = event->mimeData()->urls();
    if(urls.count() != 1)
    {
        QMessageBox::critical(this, tr("Too many files"),
                              tr("%1 files have been dropped, while only 1 "
                                 "can be opened at a time.").arg(urls.count()));
        return;
    }
    const auto path = urls[0].toLocalFile();
    if(openFile(path))
        event->acceptProposedAction();
}

void Canvas::setImage(const QImage& image)
{
    image_ = image;
    update();
}

bool Canvas::openFile(const QString& path)
{
    QImageReader imgReader(path);
    const auto img = imgReader.read();
    if(img.isNull())
    {
        QMessageBox::critical(this, tr("Error opening image"),
                              tr("Failed to read the image file\n\"%1\":\n%2")
                                .arg(path)
                                .arg(imgReader.errorString()));
        return false;
    }
    setImage(img);
    emit newImageLoaded(QFileInfo(path).fileName());
    qDebug() << "Image loaded";
    return true;
}

void Canvas::closeImage()
{
    texture_.reset();
}

void Canvas::paintGL()
{
    using namespace Eigen;
    if(const auto rot = sensor_->reading(); rot && sensor_->hasZ())
    {
        Matrix3d sensorRotation;
        sensorRotation =
            // Compensate for the default 0,0,0 orientation,
            // which represents the phone lying on a table.
            AngleAxisd(M_PI/2, -Vector3d::UnitY()) *
            // Apply the sensed rotation
            AngleAxisd(rot->z() * DEGREE, Vector3d::UnitX()) *
            AngleAxisd(rot->x() * DEGREE, Vector3d::UnitY()) *
            AngleAxisd(rot->y() * DEGREE, Vector3d::UnitZ());
        const Vector3d rpy = sensorRotation.eulerAngles(2,1,0).reverse();
        deltaRoll_ = rpy[0];
        deltaPitch_ = -rpy[1];
        deltaYaw_ = rpy[2];
        if(std::abs(deltaPitch_) > M_PI/2)
        {
            // Invert the whole set of angles
            deltaRoll_  = normalizedAngle(deltaRoll_ - M_PI);
            deltaPitch_ = normalizedAngle(M_PI - deltaPitch_);
            deltaYaw_   = normalizedAngle(deltaYaw_ - M_PI);
        }
    }

    getViewportSize();
#ifdef Q_OS_ANDROID
    glClearColor(0.0,0.0,0.0,1);
#else
    glClearColor(0.3,0.3,0.3,1);
#endif
    glClear(GL_COLOR_BUFFER_BIT);

    if(!isVisible() || viewportWidth_==0 || viewportHeight_==0 || (!texture_ && image_.isNull()))
        return;

    if(!image_.isNull())
    {
        if(image_.width() > maxTexSize_ || image_.height() > maxTexSize_)
        {
            qWarning() << "Image resolution of" << image_.width() << "×" << image_.height()
                << "exceeds GL_MAX_TEXTURE_SIZE =" << maxTexSize_;
            image_ = image_.scaled(std::min(image_.width(), maxTexSize_),
                                   std::min(image_.height(), maxTexSize_),
                                  Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            qDebug() << "Image resized to" << image_.width() << "×" << image_.height();
        }
        // We have a new image, upload it to the GL
        texture_.reset(new QOpenGLTexture(image_));
        texture_->bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GLint anisotropy = 0;
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropy);
        if(anisotropy > 0)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
        image_ = {};
    }

    glBindVertexArray(vao_);

    program_.bind();
    texture_->bind(0);
    program_.setUniformValue("tex", 0);
    program_.setUniformValue("horizViewAngle", float(horizViewAngle_));
    program_.setUniformValue("viewportAspectRatio", float(viewportWidth_) / viewportHeight_);
    program_.setUniformValue("cameraRotation", toQMatrix(cameraRotation()));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
}
