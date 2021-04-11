#ifndef FACEFEATURE_H
#define FACEFEATURE_H

#include <QAtomicInteger>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QScopedPointer>
#include <QString>
#include <QThread>
#include <QVector>
#include <QWeakPointer>

#include <arcface/arcfaceengine.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <QHash>

Q_DECLARE_METATYPE(ASF_SingleFaceInfo);

class FaceFeature : public QObject
{
    Q_OBJECT
public:
    using FaceFeatureHash = QHash<QString, ASF_FaceFeature>;

    explicit FaceFeature(const QString &appID, const QString &sdkKey, const QString &activeKey, double matchThreshold = 0.8, double liveThreshold = 0.5);
    ~FaceFeature();

    bool getActiveFileInfo(QString &startTime, QString &endTime, QString &platform, QString &sdkType, QString &appId, QString &sdkKey, QString &sdkVersion, QString &fileVersion);
    void setFeaturesHash(FaceFeatureHash *other);
    bool getFeatures(cv::Mat &image, ASF_SingleFaceInfo &faceInfo, ASF_FaceFeature &faceFeature);
    bool getFaceInfo(cv::Mat &image, ASF_SingleFaceInfo &faceInfo);
    bool facePairMatching(float &confidenceLevel, ASF_FaceFeature &feature1, ASF_FaceFeature &feature2);

public slots:
    void importFaceInfo(cv::Mat image, ASF_SingleFaceInfo faceInfo);
    void setLiveThreshold(double value = 0.5);
    void setMatchThreshold(double value = 0.8);

public:
    QAtomicInteger<int> m_curFaceFeatureCnt = 0;

signals:
    void statusChanged(bool isAvailable);
    void exportFeature(int index, bool isAlive);
    void exportFeature(ASF_FaceFeature feature, bool isAlive);
    void exportMatchInfo(QString info);

private slots:
    void tbegin();
    void tend();

private:
    QScopedPointer<ArcFaceEngine> m_pImageFaceEngine;

    ASF_MultiFaceInfo m_multiFaceInfo = {0, 0, 0, 0};
    ASF_FaceFeature m_faceFeature = {0, 0};

    QString m_appID;
    QString m_sdkKey;
    QString m_activeKey;

    double m_matchThreshold;
    double m_liveThreshold;

    FaceFeatureHash *m_pFeaturesHash = nullptr;

    QThread m_thread;
};

#endif // FACEFEATURE_H

