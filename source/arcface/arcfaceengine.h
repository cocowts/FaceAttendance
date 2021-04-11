#ifndef ARC_FACE_ENGINE
#define ARC_FACE_ENGINE

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "merror.h"
#include "arcsoft_face_sdk.h"

class ArcFaceEngine
{
public:
	ArcFaceEngine();
	~ArcFaceEngine();

    //����
	MRESULT ActiveSDK(char* appID, char* sdkKey, char* activeKey);
    //��ȡ�����ļ���Ϣ�����Ի�ȡ����Ч�ڣ�
	MRESULT GetActiveFileInfo(ASF_ActiveFileInfo& activeFileInfo);
    //��ʼ������
	MRESULT InitEngine(ASF_DetectMode mode);	
    //�ͷ�����
	MRESULT UnInitEngine();	
    //�������
    MRESULT PreDetectFace(cv::Mat& image, ASF_SingleFaceInfo& faceRect, bool isRGB);
    //����������ȡ
    MRESULT PreExtractFeature(cv::Mat& image, ASF_FaceFeature& feature, ASF_SingleFaceInfo& faceRect);
    //�����ȶ�
	MRESULT FacePairMatching(MFloat &confidenceLevel, ASF_FaceFeature feature1, ASF_FaceFeature feature2, 
		ASF_CompareModel compareModel = ASF_LIFE_PHOTO);
    //���û�����ֵ
	MRESULT SetLivenessThreshold(MFloat	rgbLiveThreshold, MFloat irLiveThreshold);
    //RGBͼ���������Լ��
    MRESULT FaceASFProcess(ASF_MultiFaceInfo detectedFaces, cv::Mat &img, ASF_AgeInfo &ageInfo,
		ASF_GenderInfo &genderInfo, ASF_Face3DAngle &angleInfo, ASF_LivenessInfo& liveNessInfo);
    MRESULT FaceASFProcess(ASF_MultiFaceInfo detectedFaces, cv::Mat &img, ASF_LivenessInfo& liveNessInfo);

    //IR������
    MRESULT FaceASFProcess_IR(ASF_MultiFaceInfo detectedFaces, cv::Mat &img, ASF_LivenessInfo& irLiveNessInfo);
    //��ȡ�汾��Ϣ
	const ASF_VERSION GetVersion();
	
private:
	MHandle m_hEngine;
};

//��ɫ�ռ�ת��
int ColorSpaceConversion(cv::Mat& image, MInt32 format, ASVLOFFSCREEN& offscreen);

#endif // ARC_FACE_ENGINE
