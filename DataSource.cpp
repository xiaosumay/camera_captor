#include "DataSource.h"

#include <QtCharts/QXYSeries>
#include <QtCharts/QAreaSeries>
#include <QtCore/QDebug>
#include <QtCore/QtMath>
#include <QDateTime>
#include <QDir>
#include <QApplication>
#include <QtConcurrent>
#include <QThreadPool>

QT_CHARTS_USE_NAMESPACE

Q_DECLARE_METATYPE(QAbstractSeries *)
Q_DECLARE_METATYPE(QAbstractAxis *)

static QThreadPool g_pool;

DataSource::DataSource(QObject *parent) : QObject(parent), m_channels(1), m_id(0), m_running(true)
{
    qRegisterMetaType<QAbstractSeries *>();
    qRegisterMetaType<QAbstractAxis *>();

    g_pool.setMaxThreadCount(1);
}

DataSource::~DataSource()
{
    waitForFinished();
}

void DataSource::writeData2(QByteArray data)
{
    writeData(data.constData(), data.length());
}

qint64 DataSource::readData(char *data, qint64 maxSize)
{
    Q_UNUSED(data)
    Q_UNUSED(maxSize)
    return -1;
}

qint64 DataSource::writeData(const char *data, qint64 maxSize)
{
#if 0
    static qint64 lasttime = QDateTime::currentMSecsSinceEpoch();
    auto curtime = QDateTime::currentMSecsSinceEpoch();

    qDebug() << "speed: " << (curtime - lasttime);
    lasttime = curtime;
#endif

    emit capture();

    if (m_series) {
        qint64 range = 2000;
        QVector<QPointF> oldPoints = m_series->pointsVector();
        QVector<QPointF> points;
        int resolution = 16;
        int step = (sizeof(short) * m_channels);

        int maxSize1 = maxSize / step;

        if (oldPoints.count() < range) {
            points = m_series->pointsVector();
        } else {
            for (int i = maxSize1 / resolution; i < oldPoints.count(); i++)
                points.append(QPointF(i - maxSize1 / resolution, oldPoints.at(i).y()));
        }

        qint64 size = points.count();
        for (int k = 0; k < maxSize1 / resolution; k++) {
            points.append(QPointF(k + size, ((qint8)(data[resolution * k * step + 1])) / 32.0));
        }

        m_series->replace(points);
    }

    if (m_channels == 1) {
        emit audioAvailable(QByteArray(data, maxSize));

    } else {
        QByteArray buff(maxSize / m_channels, Qt::Uninitialized);
        for (int i = 0; i < (maxSize / (m_channels * sizeof(short))); ++i) {
            memcpy(buff.data() + sizeof(short) * i, data + sizeof(short) * i * m_channels, sizeof(short));
        }

        emit audioAvailable(buff);
    }

    if (m_audio) {
        m_audio->write(QByteArray(data, maxSize));
        m_audio->flush();
    }

    return maxSize;
}

static bool isEmpty(QList<std::tuple<int, QString, QImage>> &imageQueue, QMutex &imageQueueMutex)
{
    bool empty = true;
    {
        QMutexLocker l(&imageQueueMutex);
        empty = imageQueue.isEmpty();
    }

    return empty;
}

void DataSource::runnThread()
{
    std::tuple<int, QString, QImage> image;

    while (m_running) {
        {
            QMutexLocker l(&imageQueueMutex);
            if (!imageQueue.isEmpty()) {
                image = imageQueue.takeFirst();
            } else {
                QThread::msleep(40);
                continue;
            }
        }

        int ret = 0;
        do {
            ret = std::get<2>(image).save(QString("%1/image-%2.jpg")
                                              .arg(std::get<1>(image))
                                              .arg(std::get<0>(image), 8, 10, QChar('0')));

        } while (ret == 0);
    }
}

void DataSource::onAudio(const char *data, int len)
{
    QByteArray qData = QByteArray(data, len);
    QMetaObject::invokeMethod(this, "writeData2", Qt::QueuedConnection, Q_ARG(QByteArray, qData));
}

void DataSource::setSeries(QAbstractSeries *series)
{
    m_series = static_cast<QXYSeries *>(series);
}

void DataSource::setChannelCount(int count)
{
    m_channels = count;
    m_id = 0;
    m_name = QApplication::applicationDirPath() + "/" + QDateTime::currentDateTime().toString("yy-MM-dd-HH-mm-ss");
    m_running = true;

    QDir().mkpath(m_name);

    m_audio.reset(new QFile(QStringLiteral("%1/audio-%2.pcm").arg(m_name).arg(m_channels)));
    m_audio->open(QIODevice::WriteOnly | QIODevice::Truncate);

    QtConcurrent::run(&g_pool, this, &DataSource::runnThread);
}

void DataSource::onImageAvailable(const QImage &img)
{
    {
        QMutexLocker l(&imageQueueMutex);
        imageQueue << std::make_tuple(m_id++, m_name, img);
    }
}

void DataSource::waitForFinished()
{
    while (!isEmpty(imageQueue, imageQueueMutex)) { QThread::msleep(40); }

    m_running = false;
    g_pool.waitForDone();
}
