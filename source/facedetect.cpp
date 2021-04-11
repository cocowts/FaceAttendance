#include "facedetect.h"

#include <QRect>

FaceDetect::FaceDetect(const QString &appID, const QString &sdkKey, const QString &activeKey)
    : m_appID(appID), m_sdkKey(sdkKey), m_activeKey(activeKey)
{
    moveToThread(&m_thread);

    connect(&m_thread, &QThread::started,  this, &FaceDetect::tbegin);
    connect(&m_thread, &QThread::finished, this, &FaceDetect::tend);

    m_thread.start();
}

FaceDetect::~FaceDetect()
{
   m_thread.quit();
   m_thread.wait();
}

void FaceDetect::tbegin()
{
    m_pVideoFaceEngine.reset(new ArcFaceEngine);

    if (m_pVideoFaceEngine->ActiveSDK(m_appID.toLatin1().data(), m_sdkKey.toLatin1().data(), m_activeKey.toLatin1().data()) == MOK
            && m_pVideoFaceEngine->InitEngine(ASF_DETECT_MODE_VIDEO) == MOK)
    {
        statusChanged(true);
    }
    else
    {
        statusChanged(false);
    }
}

void FaceDetect::tend()
{
    m_pVideoFaceEngine->UnInitEngine();

    statusChanged(false);
}

bool FaceDetect::getActiveFileInfo(QString &startTime, QString &endTime, QString &platform, QString &sdkType,
     QString &appId, QString &sdkKey, QString &sdkVersion, QString &fileVersion)
{
    ASF_ActiveFileInfo activeFileInfo = {  };

    if (m_pVideoFaceEngine->GetActiveFileInfo(activeFileInfo) == MOK)
    {
        startTime   = activeFileInfo.startTime;
        endTime     = activeFileInfo.endTime;
        platform    = activeFileInfo.platform;
        sdkType     = activeFileInfo.sdkKey;
        appId       = activeFileInfo.appId;
        sdkKey      = activeFileInfo.sdkKey;
        sdkVersion  = activeFileInfo.sdkVersion;
        fileVersion = activeFileInfo.fileVersion;

        return true;
    }

    return false;
}

bool FaceDetect::getFaceInfo(cv::Mat &image, ASF_SingleFaceInfo &faceInfo)
{
    if (m_pVideoFaceEngine->PreDetectFace(image, faceInfo, true) == MOK)
        return true;

    return false;
}

void FaceDetect::importImage(cv::Mat image)
{
    ASF_SingleFaceInfo faceInfo = {{0, 0, 0, 0}, 0};

    QRect rect;

    if (m_pVideoFaceEngine->PreDetectFace(image, faceInfo, true) == MOK)
    {
        rect.setLeft(faceInfo.faceRect.left);
        rect.setTop(faceInfo.faceRect.top);
        rect.setRight(faceInfo.faceRect.right);
        rect.setBottom(faceInfo.faceRect.bottom);

        exportFaceInfo(std::move(image), std::move(faceInfo));
    }

    emit exportRect(std::move(rect));

    --m_curFaceInfoCnt;
}
