#ifndef VIDEOCAPTURE_H
#define VIDEOCAPTURE_H

#include <QImage>
#include <QObject>
#include <QThread>
#include <QTimer>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "arcface/arcfaceengine.h"

Q_DECLARE_METATYPE(QImage);
Q_DECLARE_METATYPE(cv::Mat);

class VideoCapture : public QObject
{
    Q_OBJECT
public:
    enum InputType {
        Camera  = 0u,
        Other
    };

    explicit VideoCapture(QObject *parent = nullptr);
    ~VideoCapture();

    bool isOpened() const;
    InputType inputType() const;
    QSize getResolutions() const;

public slots:
    void openCamara(const QString &url);
    void openCamara(int index);
    void closeCamara();
    void setResolutions(const QSize &size);

signals:
    void updateImage(QImage);
    void updateImage(cv::Mat);
    void statusChanged(bool isOpen);

private slots:
    void tbegin();
    void tend();
    void captureVideo();

private:
    InputType m_inputType = Camera;
    QScopedPointer<cv::VideoCapture> m_capture;  
    QTimer  *m_timer = nullptr;
    QThread m_thread;
};

#endif // VIDEOCAPTURE_H
