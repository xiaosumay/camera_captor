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
    QScopedPointer<QMediaRecorder> m_mediaRecorder;
    QScopedPointer<QAudioInput> m_audioInput;
    DataSource *dataSource;
    QScopedPointer<Mp4Maker> m_mp4Maker;
};
#endif // WIDGET_H
