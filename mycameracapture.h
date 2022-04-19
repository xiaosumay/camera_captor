#ifndef MYCAMERACAPTURE_H
#define MYCAMERACAPTURE_H

#include <QLabel>
#include <QAbstractVideoSurface>
#include <QMutex>

class MyCameraCapture : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    MyCameraCapture(QLabel *widget, QObject *parent = nullptr);

    QImage getImage() const;

public:
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const;
    bool present(const QVideoFrame &frame);

private:
    QImage m_image;
    QMutex mutable m_locker;
    QLabel *m_label;
};

#endif // MYCAMERACAPTURE_H
