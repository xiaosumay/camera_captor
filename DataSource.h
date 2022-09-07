#ifndef DATASOURCE_H
#define DATASOURCE_H

#include <QtCore/QObject>
#include <QtCharts/QAbstractSeries>
#include <QtCharts/QXYSeries>
#include <QtCore/QIODevice>
#include <QVideoFrame>
#include <atomic>
#include <QFile>
#include <QMutex>

#include <tuple>

QT_BEGIN_NAMESPACE
class QQuickView;
QT_END_NAMESPACE

QT_CHARTS_USE_NAMESPACE

class DataSource : public QIODevice
{
    Q_OBJECT
public:
    explicit DataSource(QObject *parent = 0);
    ~DataSource();

    void setSeries(QAbstractSeries *series);

    void setChannelCount(int count);
    QString getPath() const { return m_name; }

public:
    void onImageAvailable(const QImage &img);
    void waitForFinished();

signals:
    void capture();
    void audioAvailable(QByteArray);

protected:
    qint64 readData(char *data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private:
    void runnThread();

private:
    QXYSeries *m_series;
    int m_channels;
    QString m_name;
    std::atomic_uint64_t m_id;
    QScopedPointer<QFile> m_audio;

    QList<std::tuple<int, QString, QImage>> imageQueue;
    QMutex imageQueueMutex;
    std::atomic_bool m_running;
};


#endif // DATASOURCE_H
