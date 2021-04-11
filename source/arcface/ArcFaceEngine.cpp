#include <arcface/arcfaceengine.h>

using namespace std;

#define NSCALE 32
#define FACENUM 5

ArcFaceEngine::ArcFaceEngine()
{ }

ArcFaceEngine::~ArcFaceEngine()
{ }

MRESULT ArcFaceEngine::ActiveSDK(char* appID,char* sdkKey, char* activeKey)
{
    (void)activeKey;
#ifdef PRO
    MRESULT res = ASFOnlineActivation(appID, sdkKey, activeKey);
#else
    MRESULT res = ASFOnlineActivation(appID, sdkKey);
#endif
    if (MOK != res && MERR_ASF_ALREADY_ACTIVATED != res)
        return res;
    return MOK;
}

MRESULT ArcFaceEngine::GetActiveFileInfo(ASF_ActiveFileInfo& activeFileInfo)
{
    MRESULT res = ASFGetActiveFileInfo(&activeFileInfo);
    return res;
}

MRESULT ArcFaceEngine::InitEngine(ASF_DetectMode detectMode)
{
    m_hEngine = NULL;
    MInt32 mask = 0;
    MInt32 scaleVal = 0;

    if (ASF_DETECT_MODE_IMAGE == detectMode)
    {
        scaleVal = 32;
        mask = ASF_FACE_DETECT | ASF_FACERECOGNITION | ASF_AGE | ASF_GENDER | ASF_FACE3DANGLE | ASF_LIVENESS | ASF_IR_LIVENESS;
    }
    else
    {
        scaleVal = 16;
        mask = ASF_FACE_DETECT | ASF_FACERECOGNITION | ASF_LIVENESS | ASF_IR_LIVENESS;
    }

    MRESULT res = ASFInitEngine(detectMode, ASF_OP_0_ONLY, scaleVal, FACENUM, mask, &m_hEngine);
    return res;
}


MRESULT ArcFaceEngine::FacePairMatching(MFloat &confidenceLevel, ASF_FaceFeature feature1,
    ASF_FaceFeature feature2, ASF_CompareModel compareModel)
{
    int res = ASFFaceFeatureCompare(m_hEngine, &feature1, &feature2, &confidenceLevel, compareModel);
    return res;
}

MRESULT ArcFaceEngine::SetLivenessThreshold(MFloat	rgbLiveThreshold, MFloat irLiveThreshold)
{
    ASF_LivenessThreshold threshold = { 0, 0 };

    threshold.thresholdmodel_BGR = rgbLiveThreshold;
    threshold.thresholdmodel_IR = irLiveThreshold;

    int res = ASFSetLivenessParam(m_hEngine, &threshold);
    return res;
}

MRESULT ArcFaceEngine::FaceASFProcess(ASF_MultiFaceInfo detectedFaces, cv::Mat &img, ASF_AgeInfo &ageInfo,
    ASF_GenderInfo &genderInfo, ASF_Face3DAngle &angleInfo, ASF_LivenessInfo& liveNessInfo)
{
    MInt32 lastMask = ASF_AGE | ASF_GENDER | ASF_FACE3DANGLE | ASF_LIVENESS;
    cv::Mat cutImg = img(cv::Rect(0, 0, img.cols - (img.cols % 4), img.rows));
    ASVLOFFSCREEN offscreen = { 0, 0, 0, {0}, {0} };

    ColorSpaceConversion(cutImg, ASVL_PAF_RGB24_B8G8R8, offscreen);

    int res = ASFProcessEx(m_hEngine, &offscreen, &detectedFaces, lastMask);
    res = ASFGetAge(m_hEngine, &ageInfo);
    res = ASFGetGender(m_hEngine, &genderInfo);
    res = ASFGetFace3DAngle(m_hEngine, &angleInfo);
    res = ASFGetLivenessScore(m_hEngine, &liveNessInfo);
    cutImg.release();

    return res;
}

MRESULT ArcFaceEngine::FaceASFProcess(ASF_MultiFaceInfo detectedFaces, cv::Mat &img, ASF_LivenessInfo& liveNessInfo)
{
    MInt32 lastMask = ASF_LIVENESS;
    cv::Mat cutImg = img(cv::Rect(0, 0, img.cols - (img.cols % 4), img.rows));
    ASVLOFFSCREEN offscreen = { 0, 0, 0, {0}, {0} };

    ColorSpaceConversion(cutImg, ASVL_PAF_RGB24_B8G8R8, offscreen);

    int res = ASFProcessEx(m_hEngine, &offscreen, &detectedFaces, lastMask);
    res = ASFGetLivenessScore(m_hEngine, &liveNessInfo);
    cutImg.release();

    return res;
}

MRESULT ArcFaceEngine::FaceASFProcess_IR(ASF_MultiFaceInfo detectedFaces, cv::Mat &img, ASF_LivenessInfo& irLiveNessInfo)
{
    MInt32 lastMask = ASF_IR_LIVENESS;
    cv::Mat grayMat = img(cv::Rect(0, 0, img.cols - (img.cols % 4), img.rows));
    cv::cvtColor(grayMat, grayMat, cv::COLOR_BGR2GRAY);

    ASVLOFFSCREEN offscreen = { 0, 0, 0, {0}, {0} };
    ColorSpaceConversion(grayMat, ASVL_PAF_GRAY, offscreen);

    int res = ASFProcessEx_IR(m_hEngine, &offscreen, &detectedFaces, lastMask);
    res = ASFGetLivenessScore_IR(m_hEngine, &irLiveNessInfo);
    grayMat.release();
    return res;
}

const ASF_VERSION ArcFaceEngine::GetVersion()
{
    const ASF_VERSION pVersionInfo = ASFGetVersion();
    return pVersionInfo;
}

MRESULT ArcFaceEngine::PreDetectFace(cv::Mat &image, ASF_SingleFaceInfo& faceRect, bool isRGB)
{
    MRESULT res = MOK;
    ASF_MultiFaceInfo detectedFaces = { 0, 0, 0, 0 };//人脸检测
    cv::Mat cutImg = image(cv::Rect(0, 0, image.cols - (image.cols % 4), image.rows));
    if (isRGB)
    {
        ASVLOFFSCREEN offscreen = { 0, 0, 0, {0}, {0} };
        ColorSpaceConversion(cutImg, ASVL_PAF_RGB24_B8G8R8, offscreen);

        res = ASFDetectFacesEx(m_hEngine, &offscreen, &detectedFaces);
    }
    else  //IR图像
    {
        cv::Mat cutImg = image(cv::Rect(0, 0, image.cols - (image.cols % 4), image.rows));
        cv::cvtColor(cutImg, cutImg, cv::COLOR_BGR2GRAY);
        ASVLOFFSCREEN offscreen = { 0, 0, 0, {0}, {0} };
        ColorSpaceConversion(cutImg, ASVL_PAF_GRAY, offscreen);

        res = ASFDetectFacesEx(m_hEngine, &offscreen, &detectedFaces);
    }

    if (res != MOK || detectedFaces.faceNum < 1)
    {
        cutImg.release();
        return -1;
    }

    int max = 0;
    int maxArea = 0;

    for (int i = 0; i < detectedFaces.faceNum; i++)
    {
        if (detectedFaces.faceRect[i].left < 0)
            detectedFaces.faceRect[i].left = 10;
        if (detectedFaces.faceRect[i].top < 0)
            detectedFaces.faceRect[i].top = 10;
        if (detectedFaces.faceRect[i].right < 0 || detectedFaces.faceRect[i].right > cutImg.cols)
            detectedFaces.faceRect[i].right = cutImg.cols - 10;
        if (detectedFaces.faceRect[i].bottom < 0 || detectedFaces.faceRect[i].bottom > cutImg.rows)
            detectedFaces.faceRect[i].bottom = cutImg.rows - 10;

        if ((detectedFaces.faceRect[i].right - detectedFaces.faceRect[i].left)*
            (detectedFaces.faceRect[i].bottom - detectedFaces.faceRect[i].top) > maxArea)
        {
            max = i;
            maxArea = (detectedFaces.faceRect[i].right - detectedFaces.faceRect[i].left)*
                (detectedFaces.faceRect[i].bottom - detectedFaces.faceRect[i].top);
        }
    }

    faceRect.faceRect.left = detectedFaces.faceRect[max].left;
    faceRect.faceRect.top = detectedFaces.faceRect[max].top;
    faceRect.faceRect.right = detectedFaces.faceRect[max].right;
    faceRect.faceRect.bottom = detectedFaces.faceRect[max].bottom;
    faceRect.faceOrient = detectedFaces.faceOrient[max];
    cutImg.release();

    return res;
}

// 预先提取人脸特征
MRESULT ArcFaceEngine::PreExtractFeature(cv::Mat &image, ASF_FaceFeature& feature, ASF_SingleFaceInfo& faceRect)
{
    if (image.data == NULL)
        return -1;

    cv::Mat cutImg = image(cv::Rect(0, 0, image.cols - (image.cols % 4), image.rows));

    MRESULT res = MOK;
    ASF_FaceFeature detectFaceFeature = { 0, 0 };//特征值
    ASVLOFFSCREEN offscreen = { 0, 0, 0, {0}, {0} };
    ColorSpaceConversion(cutImg, ASVL_PAF_RGB24_B8G8R8, offscreen);
    res = ASFFaceFeatureExtractEx(m_hEngine, &offscreen, &faceRect, &detectFaceFeature);

    if (MOK != res)
    {
        cutImg.release();
        return res;
    }

    if (!feature.feature)
        return -1;

    memset(feature.feature, 0, detectFaceFeature.featureSize);
    memcpy(feature.feature, detectFaceFeature.feature, detectFaceFeature.featureSize);
    cutImg.release();

    return res;
}

MRESULT ArcFaceEngine::UnInitEngine()
{
    //销毁引擎
    return ASFUninitEngine(m_hEngine);
}

//颜色空间转换
int ColorSpaceConversion(cv::Mat& image, MInt32 format, ASVLOFFSCREEN& offscreen)
{
    switch (format)
    {
    case ASVL_PAF_RGB24_B8G8R8:
        offscreen.u32PixelArrayFormat = (unsigned int)format;
        offscreen.i32Width  = image.cols;
        offscreen.i32Height = image.rows;
        offscreen.pi32Pitch[0] = image.step;
        offscreen.ppu8Plane[0] = (MUInt8*)image.data;
        break;
    case ASVL_PAF_GRAY:
        offscreen.u32PixelArrayFormat = (unsigned int)format;
        offscreen.i32Width  = image.cols;
        offscreen.i32Height = image.rows;
        offscreen.pi32Pitch[0] = image.step;
        offscreen.ppu8Plane[0] = (MUInt8*)image.data;
        break;
    default:
        return 0;
    }
    return 1;
}
