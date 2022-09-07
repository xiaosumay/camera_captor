#ifndef MYCAMERACAPTURE_H
#define MYCAMERACAPTURE_H

#include <QLabel>
#include <QAbstractVideoSurface>
#include <QMutex>
#include <QQueue>

class MyCameraCapture : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    MyCameraCapture(QLabel *widget, QObject *parent = nullptr);

    QImage getImage();

public:
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const;
    bool present(const QVideoFrame &frame);

private:
    QQueue<QImage> m_image;
    QMutex mutable m_locker;
    QLabel *m_label;
};

#endif // MYCAMERACAPTURE_H
