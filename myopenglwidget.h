#ifndef MYOPENGLWIDGET_H
#define MYOPENGLWIDGET_H
#include <QOpenGLWidget>
#include <QVideoFrame>

class MyOpenGLWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    MyOpenGLWidget(QWidget *parant = Q_NULLPTR);

public slots:
    void showCameraFrameSlot(QVideoFrame image);

    // QOpenGLWidget interface
protected:
    void paintGL() override;

private:
    QVideoFrame m_CameraFrame;
};

#endif // MYOPENGLWIDGET_H
