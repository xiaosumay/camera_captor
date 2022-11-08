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
    void setupRate(int rate) { m_rate = rate; }

public:
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const;
    bool present(const QVideoFrame &frame);

private:
    bool getClock();

private:
    QQueue<QImage> m_image;
    QMutex mutable m_locker;
    QLabel *m_label;
    int m_rate = 0;
};

#endif // MYCAMERACAPTURE_H
