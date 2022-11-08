#ifndef MP4MAKER_H
#define MP4MAKER_H

#include <QObject>
#include <QString>

struct AVCodecContext;
struct AVFormatContext;
struct SwsContext;
struct SwrContext;
struct AVFrame;
struct AVStream;

class Mp4Maker : public QObject
{
    Q_OBJECT
public:
    explicit Mp4Maker(QString hwdevice, QObject *parent = nullptr);
    ~Mp4Maker();

    bool init(const QSize &size, const QString &save_path);

    static QStringList get_vdec_support_hwdevices();

public slots:
    void addAudio(QByteArray audio);
    void addImage(const QImage &img);

private:
    bool m_unit_started = false;
    QString m_hwdevice;

    AVFormatContext *pFormatCtx = NULL;

    AVCodecContext *pVideoCodecCtx = NULL;
    AVStream *pVideoStream = NULL;
    SwsContext *pSwsCtx = NULL;
    AVFrame *vframe = NULL;

    AVCodecContext *pAudioCodecCtx = NULL;
    AVStream *pAudioStream = NULL;
    SwrContext *swrCtx = NULL;
    AVFrame *aframe = NULL;

    int vpts = 0;
    int apts = 0;
};

#endif // MP4MAKER_H
