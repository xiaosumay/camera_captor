#include "widget.h"
#include "ui_widget.h"

#include <QCameraInfo>
#include <QAudioDeviceInfo>
#include <QCameraViewfinder>
#include <QtConcurrent>
#include <QThreadPool>
#include <QAudioInput>

#include <QtCharts/QtCharts>

#include "DataSource.h"
#include "mycameracapture.h"

#include "mp4maker.h"

#include "recorder.h"

#if defined(MAKE_AUDIO)
#include "AudioQueue.h"
static BufferUnits bufferUnits;
#endif

Q_LOGGING_CATEGORY(WidgetLog, "aiui.Widget")

Q_DECLARE_METATYPE(QCameraInfo)

static QThreadPool g_mp4_pool;

Widget::Widget(QWidget *parent)
    : QWidget(parent), ui(new Ui::Widget), m_audioInput(Q_NULLPTR), m_mp4Maker(Q_NULLPTR)
{
    ui->setupUi(this);

    g_mp4_pool.setMaxThreadCount(1);

#if defined(MAKE_AUDIO)
    reset_buffer(&bufferUnits);
#endif

    const QList<QCameraInfo> availableCameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo : availableCameras) {
        ui->camraList->addItem(cameraInfo.description(), QVariant::fromValue(cameraInfo));
    }

    const QList<QAudioDeviceInfo> availableAudios = QAudioDeviceInfo::availableDevices(
        QAudio::AudioInput);
    for (const auto &deviceName : AudioCaptor::getDevices()) {
        ui->audioList->addItem(QString::fromStdString(deviceName));
    }

    connect(ui->camraList, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &Widget::onCameraInfoChanged);
    connect(ui->audioList, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &Widget::onAudioInfoChanged);
    connect(ui->cameraInfo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &Widget::onCameraSettingChanged);

    viewfinder = new MyCameraCapture(ui->cameraView, this);

    setCamera(QCameraInfo::defaultCamera());

    QLineSeries *series = new QLineSeries();
    series->setName(QStringLiteral("波形图"));

    dataSource = new DataSource(this);
    connect(dataSource, &DataSource::capture, this, &Widget::onCapture);
    connect(dataSource, &DataSource::audioAvailable, this, &Widget::onAudioAvailable,
            Qt::QueuedConnection);

    //dataSource->open(QIODevice::WriteOnly);
    dataSource->setSeries(series);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("");

    chart->createDefaultAxes();
    chart->axisX()->setRange(0, 2000);
    chart->axisY()->setRange(-1, 1);

    ui->audioView->setRenderHint(QPainter::Antialiasing);
    ui->audioView->setChart(chart);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::onCapture()
{
#if !defined(MAKE_VIDEO)
    QImage img = viewfinder->getImage();
    dataSource->onImageAvailable(img);
#endif
}

void Widget::onAudioAvailable(QByteArray audio)
{
    if (!send2Mp4)
        return;

    QImage img = viewfinder->getImage();

#if defined(MAKE_AUDIO)
    QtConcurrent::run(&g_mp4_pool, [this, img, audio]() {
        m_mp4Maker->addImage(img);

        static QByteArray block(2048, '\0');
        int actual = 0;

        write_loop_data(&bufferUnits, audio.data(), audio.size());
        while (read_loop_data(&bufferUnits, block.data(), block.size(), &actual) != -1) {
            m_mp4Maker->addAudio(block);
        }
    });
#else
    QtConcurrent::run(&g_mp4_pool, m_mp4Maker.get(), &Mp4Maker::addImage, img);
#endif
}

void Widget::onAudioInfoChanged(int idx)
{
#if 0
    const QAudioDeviceInfo &info = ui->audioList->itemData(idx).value<QAudioDeviceInfo>();

    qDebug() << "audio info: " << info.deviceName();

    ui->channelList->clear();
    foreach (auto item, info.supportedChannelCounts()) {
        ui->channelList->addItem(QString::number(item));
    }
#else
    ui->channelList->clear();
    for (auto item : QList<int>{1, 2, 4, 6, 8, 10, 16, 32}) {
        ui->channelList->addItem(QString::number(item));
    }
#endif
}

void Widget::onCameraInfoChanged(int idx)
{
    const QVariant info = ui->camraList->itemData(idx);
    qDebug() << "audio info: " << info.value<QCameraInfo>().description();

    setCamera(info.value<QCameraInfo>());
}

void Widget::onCameraSettingChanged(int idx)
{
    const QVariant info = ui->cameraInfo->itemData(idx);
    //设置摄像头参数
    if (m_camera)
        m_camera->setViewfinderSettings(info.value<QCameraViewfinderSettings>());
}

void Widget::setCamera(const QCameraInfo &cameraInfo)
{
    m_camera.reset(new QCamera(cameraInfo));
    m_camera->setViewfinder(viewfinder);
    m_camera->start();

    QList<QCameraViewfinderSettings> ViewSets = m_camera->supportedViewfinderSettings();

    ui->cameraInfo->clear();
    foreach (const QCameraViewfinderSettings &ViewSet, ViewSets) {
        if (ViewSet.pixelFormat() != QVideoFrame::Format_Jpeg)
            continue;

        ui->cameraInfo->addItem(QStringLiteral("rate: %1, resolution: %2x%3")
                                    .arg(ViewSet.maximumFrameRate())
                                    .arg(ViewSet.resolution().width())
                                    .arg(ViewSet.resolution().height()),
                                QVariant::fromValue(ViewSet));
    }

    viewfinder->setupRate(ViewSets[0].maximumFrameRate());

    m_camera->setViewfinderSettings(ViewSets[0]);
}

void Widget::setAudio(const QAudioDeviceInfo &audioInfo)
{
    int channels = ui->channelList->currentText().toInt();
#if 0
    QAudioFormat formatAudio;
    formatAudio.setSampleRate(16000);
    formatAudio.setChannelCount(channels);
    formatAudio.setSampleSize(16);
    formatAudio.setCodec("audio/pcm");
    formatAudio.setByteOrder(QAudioFormat::LittleEndian);
    formatAudio.setSampleType(QAudioFormat::SignedInt);

    qDebug() << formatAudio;

    auto audioInput = new QAudioInput(audioInfo, formatAudio);
    audioInput->setNotifyInterval(64);

    dataSource->setChannelCount(formatAudio.channelCount());

    m_audioInput.reset(audioInput);
#endif
    try {
        dataSource->setChannelCount(channels);

        AudioCaptor *ac = new AudioCaptor(ui->audioList->currentText().toStdString(), dataSource);

        m_audioInput.reset(ac);
    } catch (std::invalid_argument &ec) {
        m_audioInput.reset(NULL);
        qDebug() << "AudioCaptor: " << QString::fromLatin1(ec.what());
    }
}

void Widget::on_start_record_clicked()
{
    ui->audioList->setEnabled(false);
    ui->camraList->setEnabled(false);
    ui->channelList->setEnabled(false);
    ui->start_record->setEnabled(false);

    auto audioinfo = ui->audioList->currentData();
    setAudio(audioinfo.value<QAudioDeviceInfo>());

    auto videoInfo = ui->cameraInfo->currentData();
    QCameraViewfinderSettings vs = videoInfo.value<QCameraViewfinderSettings>();

#if defined(MAKE_VIDEO)
    m_mp4Maker.reset(new Mp4Maker("qsv"));
    send2Mp4 = true;

    m_mp4Maker->init(vs.resolution(), dataSource->getPath() + "/video.mp4");
#endif

    if (m_audioInput)
        m_audioInput->start();
}

void Widget::on_stop_record_clicked()
{
    if (m_audioInput) {
        ui->audioList->setEnabled(true);
        ui->channelList->setEnabled(true);
        ui->camraList->setEnabled(true);

        ui->start_record->setEnabled(true);

        m_audioInput.reset(Q_NULLPTR);

        send2Mp4 = false;
        g_mp4_pool.waitForDone();
        m_mp4Maker.reset(Q_NULLPTR);

        dataSource->waitForFinished();
    }
}
