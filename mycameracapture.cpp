#include "mycameracapture.h"
#include <QPainter>
#include <QDebug>

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include "private/qvideoframe_p.h"

QImage QVideoFrameToQImage(const QVideoFrame &videoFrame)
{
    if (videoFrame.handleType() == QAbstractVideoBuffer::NoHandle) {
        QImage image = qt_imageFromVideoFrame(videoFrame);
        if (image.isNull()) { return QImage(); }

        if (image.format() != QImage::Format_ARGB32) { image = image.convertToFormat(QImage::Format_ARGB32); }

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

MyCameraCapture::MyCameraCapture(QLabel *widget, QObject *parent)
    : QAbstractVideoSurface(parent)
    , m_image()
    , m_label(widget)
{
}

QImage MyCameraCapture::getImage()
{
    static QImage image;

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

bool MyCameraCapture::present(const QVideoFrame &frame)
{
    if (frame.isValid()) {
        QVideoFrame cloneFrame(frame);
        cloneFrame.map(QAbstractVideoBuffer::ReadOnly);
        {
            QMutexLocker l(&m_locker);

            m_image << QVideoFrameToQImage(cloneFrame);
#if 0
            QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());

            if (imageFormat != QImage::Format_Invalid) {
                m_image << QImage(cloneFrame.bits(), cloneFrame.width(), cloneFrame.height(), imageFormat);
            } else {
                int nbytes = frame.mappedBytes();
                m_image << QImage::fromData(frame.bits(), nbytes);
            }
#endif
            if (m_image.size() > 5) { m_image.pop_front(); }
        }

        cloneFrame.unmap();

        m_label->setPixmap(QPixmap::fromImage(m_image.first()).scaled(m_label->size(), Qt::KeepAspectRatio));

        return true;
    }

    return false;
}
