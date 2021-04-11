#include "facefeature.h"

FaceFeature::FaceFeature(const QString &appID, const QString &sdkKey, const QString &activeKey, double matchThreshold, double liveThreshold)
    : m_appID(appID), m_sdkKey(sdkKey), m_activeKey(activeKey), m_matchThreshold(matchThreshold), m_liveThreshold(liveThreshold)
{
    qRegisterMetaType<ASF_SingleFaceInfo>("ASF_SingleFaceInfo");

    moveToThread(&m_thread);

    connect(&m_thread, &QThread::started,  this, &FaceFeature::tbegin);
    connect(&m_thread, &QThread::finished, this, &FaceFeature::tend);

    m_thread.start();
}

FaceFeature::~FaceFeature()
{
    m_thread.quit();
    m_thread.wait();
}

void FaceFeature::tbegin()
{
    m_multiFaceInfo.faceOrient = (MInt32*)malloc(sizeof(MInt32));
    m_multiFaceInfo.faceRect = (MRECT*)malloc(sizeof(MRECT));
    m_faceFeature.featureSize = 1032;
    m_faceFeature.feature = (MByte *)malloc(m_faceFeature.featureSize * sizeof(MByte));

    m_pImageFaceEngine.reset(new ArcFaceEngine);

    if (m_pImageFaceEngine->ActiveSDK(m_appID.toLatin1().data(), m_sdkKey.toLatin1().data(), m_activeKey.toLatin1().data()) == MOK
            && m_pImageFaceEngine->InitEngine(ASF_DETECT_MODE_IMAGE) == MOK)
    {
        m_pImageFaceEngine->SetLivenessThreshold(m_liveThreshold, 0.7);

        statusChanged(true);
    }
    else
    {
        statusChanged(false);
    }
}

void FaceFeature::tend()
{
    m_pImageFaceEngine->UnInitEngine();

    free(m_multiFaceInfo.faceOrient);
    free(m_multiFaceInfo.faceRect);
}

bool FaceFeature::getFaceInfo(cv::Mat &image, ASF_SingleFaceInfo &faceInfo)
{
    if (m_pImageFaceEngine->PreDetectFace(image, faceInfo, true) == MOK)
        return true;

    return false;
}

bool FaceFeature::getActiveFileInfo(QString &startTime, QString &endTime, QString &platform, QString &sdkType,
     QString &appId, QString &sdkKey, QString &sdkVersion, QString &fileVersion)
{
    ASF_ActiveFileInfo activeFileInfo = {0, 0, 0, 0, 0, 0, 0, 0};

    if (m_pImageFaceEngine->GetActiveFileInfo(activeFileInfo) == MOK)
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

void FaceFeature::importFaceInfo(cv::Mat image, ASF_SingleFaceInfo faceInfo)
{
    if (m_pFeaturesHash == nullptr)
        return;

    ASF_LivenessInfo liveNessInfo = {0, 0 };

    m_multiFaceInfo.faceNum       = 1;
    m_multiFaceInfo.faceOrient[0] = faceInfo.faceOrient;
    m_multiFaceInfo.faceRect[0]   = faceInfo.faceRect;

    if (m_pImageFaceEngine->FaceASFProcess(m_multiFaceInfo, image, liveNessInfo) == MOK
            && liveNessInfo.isLive[0] == 1
            && m_pImageFaceEngine->PreExtractFeature(image, m_faceFeature, faceInfo) == MOK)
    {
        MFloat confidenceLevel = 0;
        MFloat maxThreshold = m_matchThreshold;
        QString key;
        for (auto iter = m_pFeaturesHash->begin(); iter != m_pFeaturesHash->end(); ++iter )
        {
            if (MOK == m_pImageFaceEngine->FacePairMatching(confidenceLevel, m_faceFeature, *iter) && confidenceLevel > maxThreshold)
            {
                maxThreshold = confidenceLevel;
                key = iter.key();
            }
        }

        if (!key.isEmpty())
            exportMatchInfo(std::move(key));
    }

    -- m_curFaceFeatureCnt;
}

void FaceFeature::setLiveThreshold(double value)
{
    m_liveThreshold = value;

    m_pImageFaceEngine->SetLivenessThreshold(m_liveThreshold, 0.7);
}

void FaceFeature::setMatchThreshold(double value)
{
    m_matchThreshold = value;
}

void FaceFeature::setFeaturesHash(FaceFeatureHash *other)
{
    m_pFeaturesHash = other;
}

bool FaceFeature::getFeatures(cv::Mat &image, ASF_SingleFaceInfo &faceInfo, ASF_FaceFeature &faceFeature)
{
    faceFeature.featureSize = 1032;
    faceFeature.feature = (MByte *)malloc(faceFeature.featureSize * sizeof(MByte));

    if (m_pImageFaceEngine->PreExtractFeature(image, faceFeature, faceInfo) == MOK)
        return true;

    return false;
}

bool FaceFeature::facePairMatching(float &confidenceLevel, ASF_FaceFeature &feature1, ASF_FaceFeature &feature2)
{
    return m_pImageFaceEngine->FacePairMatching(confidenceLevel, feature1, feature2) == MOK;
}
