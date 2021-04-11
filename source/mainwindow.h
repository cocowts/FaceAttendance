#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QHash>
#include <QLabel>
#include <QLCDNumber>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMovie>
#include <QPlainTextEdit>
#include <QPoint>
#include <QPushButton>
#include <QSqlDatabase>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <QToolButton>
#include <QWidget>

#include <iconhelper/iconhelper.h>
#include "videocapture.h"
#include "facedetect.h"
#include "facefeature.h"
#include "waitdialog.h"

class MainWindow : public QDialog
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *e) override;

private:
    void initUi();
    void initControl();
    void initSlots();
    bool initDataBase();

    bool importDatabase();
    void initPicDir();

    //
    QWidget *initTitleWidget();
    QWidget *initNavigationWidget();
    QWidget *initStackedWidget();
    QWidget *initVideoWidget();

    //
    QWidget *initRegisterWidget();
    QWidget *initRegisterBox();
    QWidget *initRegisterLogBox();

    //
    QWidget *initStatisticsWidget();
    QWidget *initStatisticsCurrStaffInfoWidget();
    QWidget *initStatisticsExportWidget();
    QWidget *initStatisticsStaffListWidget();
    QWidget *initStatisticsOtherWidget();

    //
    QWidget *initSettingWidget();

    //
    void initSystemTrayIcon();

    //
    void initWaitDialog();

    //
    void readTextFile(const QString &path, QStringList &list);
    void addAvatarList(const QString &name, const QString &time, const QImage &img);

    void updateStaffStatus(const QString &name);
    QString nameToMdk5(const QString &name);

    bool exportAttendanceFile(const QString &name, const QString savePath, bool isAbnormal = false, bool isSingle = true);
    QString getExportAttendanceFilePath();

    bool isNetStream(const QString name);

private slots:
    void topBtnClicked();
    void minBtnClicked();
    void maxBtnClicked();
    void closeBtnClicked();
    void loginBtnChecked(bool checked);

    void navigationCamBtnClicked();
    void navigationRegisterBtnClicked();
    void navigationStatisticsBtnClicked();
    void navigationSettingBtnClicked();

    void registerAvatarBtnClicked();
    void registerCancelActionTriggered();
    void registerBtnClicked();

    void videoUpdateImage(cv::Mat image);
    void faceDetectUpdate(cv::Mat image, ASF_SingleFaceInfo faceInfo);
    void faceRectUpdate(QRect rect);

    void videoSwitchBoxChecked(int state);
    void scanDevBtnClicked();
    void liveThresholdvalueChanged(double d);
    void matchThresholdvalueChanged(double d);
    void autoStartBoxChecked(int state);

    void avatarListItemClicked(QListWidgetItem *item);
    void statisticsFindBtnClicked();
    void statisticsDeleteBtnClicked();
    void statisticsCancelActionTriggered();

    void currStaffExport();
    void currStaffAbnormalExport();
    void allStaffExport();
    void allStaffAbnormalExport();

    void sdkAppIdEditEditingFinished();
    void sdkKeyEditEditingFinished();

    void writeSettings();
    void readSettings();

private:
   QPoint m_mousePoint;
   bool isMousePressed = false;

   QWidget *m_pTitleWidget  = nullptr;
   QPushButton *m_pLoginBtn = nullptr;
   QPushButton *m_pTopBtn   = nullptr;

   QStackedWidget *m_pStackedWidget = nullptr;
   QPushButton *m_pCamBtn           = nullptr;
   QPushButton *m_pRegisterBtn      = nullptr;
   QPushButton *m_pStatisticsBtn    = nullptr;
   QPushButton *m_pSettingBtn       = nullptr;

   QWidget *m_pVideoWidget = nullptr;
   QLabel *m_pVideoLbl     = nullptr;
   QLabel *m_pTimeLbl      = nullptr;
   QLabel *m_pTooltipLbl   = nullptr;
   QLabel *m_pTotalCntLbl  = nullptr;
   QMovie *m_pVideoMovie   = nullptr;
   QLabel *m_pFaceRectLbl  = nullptr;
   QLabel *m_pFaceInfoLbl  = nullptr;

   QToolButton *m_pRegisteAvatarBtn   = nullptr;
   QLineEdit *m_pRegisterNameEdit     = nullptr;
   QPlainTextEdit *m_pRegisterLogEdit = nullptr;

   QLabel *m_pStatisticsAvatarLbl     = nullptr;
   QLineEdit *m_pStatisticsSearchEdit = nullptr;
   QLabel *m_pStatisticsTimeLbl       = nullptr;

   QLabel *m_pStartStatusLbl   = nullptr;
   QLabel *m_pEndStatusLbl     = nullptr;
   QTimeEdit *m_pStartTimeEdit = nullptr;
   QTimeEdit *m_pEndTimeEdit   = nullptr;
   QDateEdit *m_pStartDateEdit = nullptr;
   QDateEdit *m_pEndDateEdit   = nullptr;
   QLabel *m_pTotalLbl         = nullptr;
   QListWidget *m_pStaffListWidget = nullptr;

   QCheckBox *m_pCamSwitchBox  = nullptr;
   QComboBox *m_pCamListBox    = nullptr;
   QComboBox *m_pCamResListBox = nullptr;
   QPushButton *m_pCamScanBtn  = nullptr;
   QDoubleSpinBox *m_pLiveThresholdBox  = nullptr;
   QDoubleSpinBox *m_pMatchThresholdBox = nullptr;
   QLineEdit *m_pSdkAppIdEdit  = nullptr;
   QLineEdit *m_pSdkKeyEdit    = nullptr;
   QLabel *m_pArcSdkTime       = nullptr;

   QCheckBox *m_pAutoStartBox  = nullptr;

   QSystemTrayIcon *m_pSystemTrayIcon = nullptr;

   WaitDialog *m_pWaitDialog = nullptr;

   QScopedPointer<VideoCapture> m_pVideoCapture;
   QScopedPointer<FaceDetect>   m_pFaceDetect;
   QScopedPointer<FaceFeature>  m_pFaceFeature;

   QString m_appID;  //{"AV3HLtNhNM98X8tGBB4GtG14PD34gouNHBQVY9E4bjEa"};
   QString m_sdkKey; //{"6FSukJpCaon1ajHdSvmKXoGUpT2sLhAcXhH31uWNhMC9"};
   QString m_activeKey{""};
   QHash<QString, ASF_FaceFeature> m_FeaturesHash;

   QUrl helpUrl{"https://segmentfault.com/a/1190000039804899"};

   QString m_faceLastName;
   QString m_picPath;

   QTimer m_tooltipTextTimer;
   QTimer m_faceInfoLastTimer;
   QSize  m_originImageSize;

   QStringList m_registerFilesPath;
   QStringList m_tooltipTextList;
   QStringList m_speechTextList;

   QSqlDatabase m_dataBase;
};
#endif // MAINWINDOW_H
