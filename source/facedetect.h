#ifndef FACEOPERATION_H
#define FACEOPERATION_H

#include <QAtomicInteger>
#include <QObject>
#include <QRect>
#include <QScopedPointer>
#include <QString>
#include <QThread>

#include <arcface/arcfaceengine.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class FaceDetect : public QObject
{
    Q_OBJECT
public:
    explicit FaceDetect(const QString &appID, const QString &sdkKey, const QString &activeKey);
    ~FaceDetect();

    bool getActiveFileInfo(QString &startTime, QString &endTime, QString &platform, QString &sdkType,
         QString &appId, QString &sdkKey, QString &sdkVersion, QString &fileVersion);

    bool getFaceInfo(cv::Mat &image, ASF_SingleFaceInfo &faceInfo);

public slots:
    void importImage(cv::Mat image);

public:
     QAtomicInteger<int> m_curFaceInfoCnt = 0;

signals:
    void exportRect(QRect rect);
    void exportFaceInfo(cv::Mat image, ASF_SingleFaceInfo faceInfo);
    void statusChanged(bool isAvailable);

private slots:
    void tbegin();
    void tend();

private:
    QScopedPointer<ArcFaceEngine> m_pVideoFaceEngine;

    QString m_appID;
    QString m_sdkKey;
    QString m_activeKey;

    QThread m_thread;
};

#endif // FACEOPERATION_H
