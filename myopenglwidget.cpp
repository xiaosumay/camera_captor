#include "myopenglwidget.h"

#include <QWidget>
#include <QPainter>

extern QImage QVideoFrameToQImage(const QVideoFrame &videoFrame);

MyOpenGLWidget::MyOpenGLWidget(QWidget *parant) : QOpenGLWidget(parant) {}

void MyOpenGLWidget::showCameraFrameSlot(QVideoFrame image)
{
    m_CameraFrame = image;
    update();
}

void MyOpenGLWidget::paintGL()
{
    QPainter painter(this);
    if (m_CameraFrame.size().width() <= 0) {
        return;
    }

    QImage _image = QVideoFrameToQImage(m_CameraFrame).scaled(size(), Qt::KeepAspectRatio);
    painter.drawImage((width() - _image.width()) / 2, 0, _image);
}
