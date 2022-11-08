#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QCamera>
#include <QCameraImageCapture>
#include <QMediaRecorder>
#include <QScopedPointer>
#include <QAudioDeviceInfo>
#include <QAudioInput>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
class QCameraViewfinder;
QT_END_NAMESPACE

class DataSource;
class MyCameraCapture;
class Mp4Maker;
class AudioCaptor;

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

public slots:
    void onCapture();
    void onAudioAvailable(QByteArray);

private slots:
    void onAudioInfoChanged(int);
    void onCameraInfoChanged(int);
    void onCameraSettingChanged(int);

    void on_start_record_clicked();
    void on_stop_record_clicked();

private:
    void setCamera(const QCameraInfo &cameraInfo);
    void setAudio(const QAudioDeviceInfo &audioInfo);

private:
    Ui::Widget *ui;
    MyCameraCapture *viewfinder;
    QScopedPointer<QCamera> m_camera;
#if 0
    QScopedPointer<QMediaRecorder> m_mediaRecorder;
    QScopedPointer<QAudioInput> m_audioInput;
#else
    QSharedPointer<AudioCaptor> m_audioInput;
#endif
    DataSource *dataSource;
    QScopedPointer<Mp4Maker> m_mp4Maker;
    bool send2Mp4 = false;
};
#endif // WIDGET_H
