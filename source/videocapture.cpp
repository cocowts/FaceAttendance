#include "videocapture.h"

VideoCapture::VideoCapture(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<cv::Mat>("cv::Mat");

    moveToThread(&m_thread);

    connect(&m_thread, &QThread::started,  this, &VideoCapture::tbegin);
    connect(&m_thread, &QThread::finished, this, &VideoCapture::tend);

    m_thread.start();
}

VideoCapture::~VideoCapture()
{
    m_thread.quit();
    m_thread.wait();
}

void VideoCapture::tbegin()
{
    m_capture.reset(new cv::VideoCapture);

    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &VideoCapture::captureVideo);
}

void VideoCapture::tend()
{
    closeCamara();
}

void VideoCapture::openCamara(const QString &url)
{
    if (!m_capture->isOpened() && m_capture->open(url.toLatin1().data()))
    {
        m_inputType = InputType::Other;

        emit statusChanged(true);

        m_timer->start(30);
    }
    else
    {
       emit statusChanged(false);
    }
}

void VideoCapture::openCamara(int index)
{
    if (!m_capture->isOpened() && m_capture->open(index))
    {
        m_inputType = InputType::Camera;

        emit statusChanged(true);

        m_timer->start(30);
    }
    else
    {
        emit statusChanged(false);
    }
}

void VideoCapture::closeCamara()
{
    m_timer->stop();

    m_capture->release();

    statusChanged(false);
}

bool VideoCapture::isOpened() const
{
    return m_capture->isOpened();
}

VideoCapture::InputType VideoCapture::inputType() const
{
    return m_inputType;
}

QSize VideoCapture::getResolutions() const
{
    QSize ret;

    ret.setWidth(m_capture->get(cv::CAP_PROP_FRAME_WIDTH));
    ret.setHeight(m_capture->get(cv::CAP_PROP_FRAME_HEIGHT));

    return ret;
}

void VideoCapture::setResolutions(const QSize &size)
{
    if (m_capture->isOpened())
    {
        m_capture->set(cv::CAP_PROP_FRAME_WIDTH, size.width());
        m_capture->set(cv::CAP_PROP_FRAME_HEIGHT, size.height());
    }
}

void VideoCapture::captureVideo()
{
    cv::Mat frame;

    *m_capture >> frame;

    if (m_inputType == InputType::Camera)
        cv::flip(frame, frame, 1);

    QImage img = QImage(frame.data, frame.cols, frame.rows, QImage::Format_RGB888).rgbSwapped();

    if (!img.isNull())
    {
        updateImage(std::move(frame));

        updateImage(std::move(img));
    }
}
