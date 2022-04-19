#include "DataSource.h"

#include <QtCharts/QXYSeries>
#include <QtCharts/QAreaSeries>
#include <QtCore/QDebug>
#include <QtCore/QtMath>
#include <QDateTime>
#include <QDir>
#include <QApplication>
#include <QtConcurrent>

QT_CHARTS_USE_NAMESPACE

Q_DECLARE_METATYPE(QAbstractSeries *)
Q_DECLARE_METATYPE(QAbstractAxis *)

static QThreadPool g_pool;

DataSource::DataSource(QObject *parent)
    : QIODevice(parent)
    , m_channels(1)
    , m_id(0)
{
    qRegisterMetaType<QAbstractSeries*>();
    qRegisterMetaType<QAbstractAxis *>();

    g_pool.setMaxThreadCount(4);
}


qint64 DataSource::readData(char * data, qint64 maxSize)
{
    Q_UNUSED(data)
    Q_UNUSED(maxSize)
    return -1;
}


qint64 DataSource::writeData(const char * data, qint64 maxSize)
{
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
        for (int i = 0; i < 640; ++i) {
            memcpy(buff.data() + sizeof(short) * i, data + sizeof(short) * i * m_channels, sizeof(short));
        }

        emit audioAvailable(buff);
    }

    if (m_audio) m_audio->write(data, maxSize);

    return maxSize;
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

    QDir().mkpath(m_name);

    m_audio.reset(new QFile(QStringLiteral("%1/audio-%2.pcm").arg(m_name).arg(m_channels)));
    m_audio->open(QIODevice::WriteOnly | QIODevice::Append);
}
