#ifndef MYCAMERACAPTURE_H
#define MYCAMERACAPTURE_H

#include <QOpenGLWidget>
#include <QAbstractVideoSurface>
#include <QMutex>
#include <QQueue>

QImage QVideoFrameToQImage(const QVideoFrame &videoFrame);

class MyCameraCapture : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    MyCameraCapture(QWidget *parent = nullptr);

    QVideoFrame getImage();
    void setupRate(int rate) { m_rate = rate; }

signals:
    void showCameraFrame(QVideoFrame);

public:
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const;
    bool present(const QVideoFrame &frame);

private:
    bool getClock();

private:
    QQueue<QVideoFrame> m_image;
    QMutex mutable m_locker;
    int m_rate = 0;
};

#endif // MYCAMERACAPTURE_H
