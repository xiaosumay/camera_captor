#include "mycameracapture.h"
#include <QPainter>
#include <QDebug>

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include "private/qvideoframe_p.h"

#include <QtConcurrent>

QImage QVideoFrameToQImage(const QVideoFrame &videoFrame)
{
    if (videoFrame.handleType() == QAbstractVideoBuffer::NoHandle) {
        QImage image = qt_imageFromVideoFrame(videoFrame);
        if (image.isNull()) {
            return QImage();
        }

        return image;
    }

    if (videoFrame.handleType() == QAbstractVideoBuffer::GLTextureHandle) {
        QImage image(videoFrame.width(), videoFrame.height(), QImage::Format_ARGB32);

        GLuint textureId = static_cast<GLuint>(videoFrame.handle().toInt());
        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        QOpenGLFunctions *f = ctx->functions();
        GLuint fbo;

        f->glGenFramebuffers(1, &fbo);

        GLint prevFbo;
        f->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);

        f->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
        f->glReadPixels(0, 0, videoFrame.width(), videoFrame.height(), GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
        f->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));

        return image.rgbSwapped();
    }

    return QImage();
}

MyCameraCapture::MyCameraCapture(QWidget *parent) : QAbstractVideoSurface(parent), m_image() {}

QVideoFrame MyCameraCapture::getImage()
{
    static QVideoFrame image;

    QMutexLocker l(&m_locker);

    if (m_image.isEmpty()) { return image; }

    image = m_image.dequeue();

    return image;
}

QList<QVideoFrame::PixelFormat> MyCameraCapture::supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const
{
    Q_UNUSED(type);

    return QList<QVideoFrame::PixelFormat>()
           << QVideoFrame::Format_ARGB32 << QVideoFrame::Format_ARGB32_Premultiplied << QVideoFrame::Format_RGB32
           << QVideoFrame::Format_RGB24 << QVideoFrame::Format_RGB565 << QVideoFrame::Format_RGB555
           << QVideoFrame::Format_ARGB8565_Premultiplied << QVideoFrame::Format_BGRA32
           << QVideoFrame::Format_BGRA32_Premultiplied << QVideoFrame::Format_BGR32 << QVideoFrame::Format_BGR24
           << QVideoFrame::Format_BGR565 << QVideoFrame::Format_BGR555 << QVideoFrame::Format_BGRA5658_Premultiplied
           << QVideoFrame::Format_AYUV444 << QVideoFrame::Format_AYUV444_Premultiplied << QVideoFrame::Format_YUV444
           << QVideoFrame::Format_YUV420P << QVideoFrame::Format_YV12 << QVideoFrame::Format_UYVY
           << QVideoFrame::Format_YUYV << QVideoFrame::Format_NV12 << QVideoFrame::Format_NV21
           << QVideoFrame::Format_IMC1 << QVideoFrame::Format_IMC2 << QVideoFrame::Format_IMC3
           << QVideoFrame::Format_IMC4 << QVideoFrame::Format_Y8 << QVideoFrame::Format_Y16 << QVideoFrame::Format_Jpeg
           << QVideoFrame::Format_CameraRaw << QVideoFrame::Format_AdobeDng;
}

bool MyCameraCapture::getClock()
{
    static int i = 6;

    if (m_rate < 10)
        return true;

    if (i-- == 0) {
        if (m_rate >= 30)
            i = 6;
        else if (m_rate >= 25)
            i = 5;
        else if (m_rate >= 10) {
            i = 2;
        }

        return true;
    }

    return false;
}

bool MyCameraCapture::present(const QVideoFrame &frame)
{
    if (frame.isValid()) {
        {
            QMutexLocker l(&m_locker);

            m_image << QVideoFrame(frame);

            if (m_image.size() > 5) {
                m_image.pop_front();
            }
        }

        if (getClock()) {
            emit showCameraFrame(frame);
        }
    }

    return true;
}
