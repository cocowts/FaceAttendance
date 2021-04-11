#include "mainwindow.h"

#include <QAction>
#include <QButtonGroup>
#include <QDateTime>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QListWidgetItem>
#include <QMovie>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QDialog(parent, Qt::FramelessWindowHint)
{
    initDataBase();
    initPicDir();
    initUi();
    initControl();
    initSlots();
}

MainWindow::~MainWindow()
{
    for (auto iter = m_FeaturesHash.begin(); iter != m_FeaturesHash.end(); ++iter )
        free(iter.value().feature);
}

void MainWindow::initUi()
{
    QHBoxLayout *pHLayout = new QHBoxLayout;
    pHLayout->setMargin(0);
    pHLayout->setSpacing(0);
    pHLayout->addWidget(initNavigationWidget());
    pHLayout->addWidget(initStackedWidget());

    QVBoxLayout *pVLayout = new QVBoxLayout;
    pVLayout->setMargin(0);
    pVLayout->setSpacing(0);
    pVLayout->addWidget(initTitleWidget());
    pVLayout->addLayout(pHLayout);

    initSystemTrayIcon();

    initWaitDialog();

    setSizeGripEnabled(true);
    setLayout(pVLayout);
    resize(646, 480);
    setWindowTitle("简单桌面");
}

QWidget *MainWindow::initTitleWidget()
{
    m_pTitleWidget = new QWidget;

    m_pTitleWidget->setObjectName("titleWidget");
    m_pTitleWidget->installEventFilter(this);

    m_pLoginBtn = new QPushButton;
    m_pTopBtn   = new QPushButton;
    QLabel *iconLbl       = new QLabel;
    QLabel *titleLbl      = new QLabel;
    QLabel *vdividingLbl  = new QLabel;
    QPushButton *minBtn   = new QPushButton;
    QPushButton *maxBtn   = new QPushButton;
    QPushButton *closeBtn = new QPushButton;

    iconLbl->setObjectName("iconLbl");
    IconHelper::setIcon(iconLbl, QChar(0xf015), 12);

    titleLbl->setObjectName("titleLbl");
    titleLbl->setText("简单考勤");

    vdividingLbl->setObjectName("vdividingLbl");
    vdividingLbl->setAlignment(Qt::AlignCenter);
    vdividingLbl->setPixmap(QPixmap(":/image/vdividingline.png"));

    m_pLoginBtn->setObjectName("loginBtn");
    m_pLoginBtn->setCheckable(true);
    m_pLoginBtn->setToolTip("登录");
    IconHelper::setIcon(m_pLoginBtn, QChar(0xf058), 10);
    connect(m_pLoginBtn, &QPushButton::clicked, this, &MainWindow::loginBtnChecked);

    m_pTopBtn->setObjectName("topBtn");
    m_pTopBtn->setCheckable(true);
    m_pTopBtn->setToolTip("置顶");
    IconHelper::setIcon(m_pTopBtn, QChar(0xf08d), 10);
    connect(m_pTopBtn, &QPushButton::clicked, this, &MainWindow::topBtnClicked);

    minBtn->setObjectName("minBtn");
    minBtn->setToolTip("最小化");
    IconHelper::setIcon(minBtn, QChar(0xf068), 10);
    connect(minBtn, &QPushButton::clicked, this, &MainWindow::minBtnClicked);

    maxBtn->setObjectName("maxBtn");
    maxBtn->setToolTip("最大化");
    IconHelper::setIcon(maxBtn, QChar(0xf05b), 10);
    connect(maxBtn, &QPushButton::clicked, this, &MainWindow::maxBtnClicked);

    closeBtn->setObjectName("closeBtn");
    closeBtn->setToolTip("关闭");
    IconHelper::setIcon(closeBtn, QChar(0xf00d), 10);
    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::closeBtnClicked);

    QHBoxLayout *pLayout = new QHBoxLayout;
    pLayout->setSpacing(0);
    pLayout->setContentsMargins(10, 0, 0, 0);
    pLayout->addWidget(iconLbl);
    pLayout->addSpacing(10);
    pLayout->addWidget(titleLbl);
    pLayout->addStretch();
    pLayout->addWidget(m_pLoginBtn);
    pLayout->addWidget(vdividingLbl);
    pLayout->addWidget(m_pTopBtn);
    pLayout->addWidget(minBtn);
    pLayout->addWidget(maxBtn);
    pLayout->addWidget(closeBtn);

    m_pTitleWidget->setLayout(pLayout);

    return m_pTitleWidget;
}

QWidget *MainWindow::initNavigationWidget()
{
    QWidget *pNavigationWidget  = new QWidget;

    pNavigationWidget->setObjectName("navigationWidget");

    m_pCamBtn        = new QPushButton(pNavigationWidget);
    m_pRegisterBtn   = new QPushButton(pNavigationWidget);
    m_pStatisticsBtn = new QPushButton(pNavigationWidget);
    m_pSettingBtn    = new QPushButton(pNavigationWidget);

    m_pCamBtn->setAutoExclusive(true);
    m_pCamBtn->setCheckable(true);
    m_pCamBtn->setObjectName("camBtn");
    m_pCamBtn->setToolTip("相机");
    IconHelper::setIcon(m_pCamBtn, QChar(0xf030), 18);
    connect(m_pCamBtn, &QPushButton::clicked, this, &MainWindow::navigationCamBtnClicked);

    m_pRegisterBtn->setAutoExclusive(true);
    m_pRegisterBtn->setCheckable(true);
    m_pRegisterBtn->setObjectName("registerBtn");
    m_pRegisterBtn->setToolTip("注册");
    IconHelper::setIcon(m_pRegisterBtn, QChar(0xf044), 18);
    connect(m_pRegisterBtn, &QPushButton::clicked, this, &MainWindow::navigationRegisterBtnClicked);

    m_pStatisticsBtn->setAutoExclusive(true);
    m_pStatisticsBtn->setCheckable(true);
    m_pStatisticsBtn->setObjectName("statisticsBtn");
    m_pStatisticsBtn->setToolTip("统计");
    IconHelper::setIcon(m_pStatisticsBtn, QChar(0xf0b1), 18);
    connect(m_pStatisticsBtn, &QPushButton::clicked, this, &MainWindow::navigationStatisticsBtnClicked);

    m_pSettingBtn->setAutoExclusive(true);
    m_pSettingBtn->setCheckable(true);
    m_pSettingBtn->setObjectName("settingBtn");
    m_pSettingBtn->setToolTip("设置");
    IconHelper::setIcon(m_pSettingBtn, QChar(0xf085), 18);
    connect(m_pSettingBtn, &QPushButton::clicked, this, &MainWindow::navigationSettingBtnClicked);

    m_pCamBtn->setChecked(true);

    m_pCamBtn->setEnabled(false);
    m_pRegisterBtn->setEnabled(false);
    m_pStatisticsBtn->setEnabled(false);
    m_pSettingBtn->setEnabled(false);

    QVBoxLayout *pLayout = new QVBoxLayout;
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->setSpacing(0);
    pLayout->addWidget(m_pCamBtn);
    pLayout->addWidget(m_pRegisterBtn);
    pLayout->addWidget(m_pStatisticsBtn);
    pLayout->addStretch();
    pLayout->addWidget(m_pSettingBtn);

    pNavigationWidget->setLayout(pLayout);

    return pNavigationWidget;
}

QWidget *MainWindow::initStackedWidget()
{
    m_pStackedWidget = new QStackedWidget(this);

    m_pStackedWidget->addWidget(initVideoWidget());
    m_pStackedWidget->addWidget(initRegisterWidget());
    m_pStackedWidget->addWidget(initStatisticsWidget());
    m_pStackedWidget->addWidget(initSettingWidget());

    return m_pStackedWidget;
}

QWidget *MainWindow::initVideoWidget()
{
    m_pVideoWidget = new QWidget(this);
    m_pVideoLbl    = new QLabel(m_pVideoWidget);
    m_pFaceInfoLbl = new QLabel;
    m_pTimeLbl     = new QLabel;
    m_pTooltipLbl  = new QLabel;
    m_pTotalCntLbl = new QLabel;
    m_pVideoMovie  = new QMovie(":/image/video.gif");
    m_pFaceRectLbl = new QLabel(m_pVideoLbl);

    m_pVideoWidget->setObjectName("videoWidget");
    m_pVideoWidget->installEventFilter(this);

    m_pVideoLbl->setObjectName("videoLbl");
    m_pVideoLbl->setAlignment(Qt::AlignCenter);
    m_pVideoLbl->setMovie(m_pVideoMovie);
    m_pVideoMovie->start();

    m_pFaceInfoLbl->setObjectName("faceInfoLbl");
    m_pFaceInfoLbl->setAlignment(Qt::AlignLeft);
    m_pFaceInfoLbl->setText("");

    m_pTimeLbl->setObjectName("timeLbl");
    m_pTimeLbl->setAlignment(Qt::AlignRight);
    m_pTimeLbl->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss ddd"));

    m_pTooltipLbl->setObjectName("tooltipLbl");
    m_pTooltipLbl->setAlignment(Qt::AlignRight);
    m_pTooltipLbl->setText("祝你，早安 午安 晚安");

    m_pTotalCntLbl->setObjectName("totalCntLbl");
    m_pTotalCntLbl->setAlignment(Qt::AlignRight);
    m_pTotalCntLbl->setText("00");

    m_pFaceRectLbl->setObjectName("faceRectLbl");
    m_pFaceRectLbl->setGeometry(0, 0, 0 ,0);

    QVBoxLayout *pLayout = new QVBoxLayout;
    pLayout->addWidget(m_pFaceInfoLbl);
    pLayout->addStretch();
    pLayout->addWidget(m_pTotalCntLbl);
    pLayout->addWidget(m_pTimeLbl);
    pLayout->addWidget(m_pTooltipLbl);

    m_pVideoLbl->setLayout(pLayout);

    return m_pVideoWidget;
}

QWidget *MainWindow::initRegisterWidget()
{
    QWidget *pRegisterWidget = new QWidget(this);

    pRegisterWidget->setObjectName("registerWidget");

    QHBoxLayout *pHLayout = new QHBoxLayout;
    pHLayout->addStretch(1);
    pHLayout->addWidget(initRegisterBox(), 0);
    pHLayout->addStretch(1);

    QVBoxLayout *pVLayout = new QVBoxLayout;
    pVLayout->addLayout(pHLayout, 0);
    pVLayout->addWidget(initRegisterLogBox(), 1);

    QHBoxLayout *pMainLayout = new QHBoxLayout;
    pMainLayout->addLayout(pVLayout);

    pRegisterWidget->setLayout(pMainLayout);

    return pRegisterWidget;
}

QWidget *MainWindow::initRegisterBox()
{
    QWidget *ret = new QWidget(this);
    QPushButton *pRegisterBtn = new QPushButton;
    m_pRegisteAvatarBtn       = new QToolButton;
    m_pRegisterNameEdit       = new QLineEdit;
    QAction *pCancelAction = new QAction(m_pRegisterNameEdit);

    m_pRegisteAvatarBtn->setObjectName("registerAvatarBtn");
    m_pRegisteAvatarBtn->setToolTip("选择文件");
    m_pRegisteAvatarBtn->setIcon(QIcon(":/image/avatarwoman.png"));
    m_pRegisteAvatarBtn->setIconSize(QSize(200, 120));
    connect(m_pRegisteAvatarBtn, &QToolButton::clicked, this, &MainWindow::registerAvatarBtnClicked);

    m_pRegisterNameEdit->setObjectName("registerNameEdit");
    m_pRegisterNameEdit->setAlignment(Qt::AlignCenter);
    m_pRegisterNameEdit->setPlaceholderText("注册的名字");
    m_pRegisterNameEdit->setToolTip("系统填写");
    m_pRegisterNameEdit->setReadOnly(true);
    m_pRegisterNameEdit->setContextMenuPolicy(Qt::NoContextMenu);

    pCancelAction->setIcon(QIcon(":/image/cancel.png"));
    pCancelAction->setToolTip("取消注册");
    m_pRegisterNameEdit->addAction(pCancelAction, QLineEdit::TrailingPosition);
    connect(pCancelAction, &QAction::triggered, this, &MainWindow::registerCancelActionTriggered);

    pRegisterBtn->setObjectName("registerFaceBtn");
    pRegisterBtn->setText("注  册");
    connect(pRegisterBtn, &QToolButton::clicked, this, &MainWindow::registerBtnClicked);

    QVBoxLayout *pLayout = new QVBoxLayout;
    pLayout->addWidget(m_pRegisteAvatarBtn);
    pLayout->addWidget(m_pRegisterNameEdit);
    pLayout->addWidget(pRegisterBtn);

    ret->setLayout(pLayout);

    return ret;
}

QWidget *MainWindow::initRegisterLogBox()
{
    m_pRegisterLogEdit = new QPlainTextEdit;

    m_pRegisterLogEdit->setObjectName("operationLogEdit");
    m_pRegisterLogEdit->setPlaceholderText("所有的操作都将记录在这里");
    m_pRegisterLogEdit->setContextMenuPolicy(Qt::NoContextMenu);
    m_pRegisterLogEdit->setReadOnly(true);

    return m_pRegisterLogEdit;
}

QWidget *MainWindow::initStatisticsWidget()
{
    QWidget *pStatisticsWidget = new QWidget;

    QHBoxLayout *pHLayout = new QHBoxLayout;
    QVBoxLayout *pVLayout = new QVBoxLayout;

    pStatisticsWidget->setObjectName("statisticsWidget");

    pVLayout->addWidget(initStatisticsCurrStaffInfoWidget(), 0);
    pVLayout->addWidget(initStatisticsOtherWidget(), 0);
    pVLayout->addWidget(initStatisticsExportWidget(), 0);
    pVLayout->addStretch(1);
    pVLayout->setSpacing(0);
    pVLayout->setMargin(0);

    pHLayout->addLayout(pVLayout, 0);
    pHLayout->addWidget(initStatisticsStaffListWidget(), 1);
    pHLayout->setSpacing(0);
    pHLayout->setMargin(0);

    pStatisticsWidget->setLayout(pHLayout);

    return pStatisticsWidget;
}

QWidget *MainWindow::initStatisticsCurrStaffInfoWidget()
{
    QWidget *pStatisticsCurrStaffInfoWidget = new QWidget;
    m_pStatisticsAvatarLbl   = new QLabel;
    m_pStatisticsSearchEdit  = new QLineEdit;
    m_pStatisticsTimeLbl     = new QLabel;
    QPushButton *m_findBtn   = new QPushButton;
    QPushButton *m_deleteBtn = new QPushButton;
    QAction *pCancelAction   = new QAction(m_pStatisticsSearchEdit);

    m_pStatisticsAvatarLbl->setObjectName("statisticsAvatarLbl");
    m_pStatisticsAvatarLbl->setPixmap(QPixmap(":/image/avatarwoman.png"));
    m_pStatisticsAvatarLbl->setFixedSize(140, 140);
    m_pStatisticsAvatarLbl->setAlignment(Qt::AlignCenter);

    m_pStatisticsSearchEdit->setObjectName("statisticsSearchEdit");
    m_pStatisticsSearchEdit->setFixedWidth(100);
    m_pStatisticsSearchEdit->setMaxLength(12);
    m_pStatisticsSearchEdit->setPlaceholderText("人员姓名");
    m_pStatisticsSearchEdit->setToolTip("人员姓名");
    m_pStatisticsSearchEdit->setAlignment(Qt::AlignCenter);
    connect(m_pStatisticsSearchEdit, static_cast<void(QLineEdit::*)(void)>(&QLineEdit::returnPressed), this, &MainWindow::statisticsFindBtnClicked);

    pCancelAction->setIcon(QIcon(":/image/cancel.png"));
    pCancelAction->setToolTip("取消注册");
    m_pStatisticsSearchEdit->addAction(pCancelAction, QLineEdit::TrailingPosition);
    connect(pCancelAction, &QAction::triggered,this, &MainWindow::statisticsCancelActionTriggered);

    m_pStatisticsTimeLbl->setWordWrap(true);
    m_pStatisticsTimeLbl->setObjectName("statisticsTimeLbl");
    m_pStatisticsTimeLbl->setAlignment(Qt::AlignCenter);
    m_pStatisticsTimeLbl->setToolTip("注册时间");
    m_pStatisticsTimeLbl->setText("注册时间");
    m_pStatisticsTimeLbl->setMaximumHeight(40);
    m_pStatisticsTimeLbl->setFixedWidth(100);

    m_findBtn->setObjectName("statisticsFindBtn");
    m_findBtn->setText("查找");
    connect(m_findBtn, &QPushButton::clicked, this, &MainWindow::statisticsFindBtnClicked);

    m_deleteBtn->setObjectName("statisticsDeleteBtn");
    m_deleteBtn->setText("删除");
    connect(m_deleteBtn, &QPushButton::clicked, this, &MainWindow::statisticsDeleteBtnClicked);

    QVBoxLayout *pVLayout = new QVBoxLayout;
    pVLayout->setSpacing(5);
    pVLayout->addWidget(m_pStatisticsSearchEdit, 0);
    pVLayout->addWidget(m_pStatisticsTimeLbl, 0);
    pVLayout->addWidget(m_findBtn, 0);
    pVLayout->addWidget(m_deleteBtn, 0);

    QHBoxLayout *pHLayout = new QHBoxLayout;
    pHLayout->setSpacing(0);
    pHLayout->setMargin(10);
    pHLayout->addWidget(m_pStatisticsAvatarLbl, 0);
    pHLayout->addSpacing(10);
    pHLayout->addLayout(pVLayout, 0);
    pHLayout->addStretch(1);

    pStatisticsCurrStaffInfoWidget->setObjectName("statisticsCurrStaffInfoWidget");
    pStatisticsCurrStaffInfoWidget->setLayout(pHLayout);

    return pStatisticsCurrStaffInfoWidget;
}

QWidget *MainWindow::initStatisticsExportWidget()
{
    QWidget *pStatisticsExportWidget = new QWidget;
    m_pStartDateEdit = new QDateEdit(QDate::currentDate().addDays(-7));
    m_pEndDateEdit   = new QDateEdit(QDate::currentDate());
    QPushButton *pCurStaffExportBtn = new QPushButton;
    QPushButton *pCurStaffAbnormalExportBtn = new QPushButton;
    QPushButton *pAllStaffExportBtn = new QPushButton;
    QPushButton *pAllStaffAbnormalExportBtn = new QPushButton;

    pStatisticsExportWidget->setObjectName("statisticsExportWidget");
    m_pStartDateEdit->setCalendarPopup(true);
    m_pEndDateEdit->setCalendarPopup(true);

    pCurStaffExportBtn->setObjectName("curStaffExportBtn");
    pCurStaffExportBtn->setText("当前人员详细导出");
    connect(pCurStaffExportBtn, &QPushButton::clicked, this, &MainWindow::currStaffExport);

    pCurStaffAbnormalExportBtn->setObjectName("curStaffAbnormalExportBtn");
    pCurStaffAbnormalExportBtn->setText("当前人员异常导出");
    connect(pCurStaffAbnormalExportBtn, &QPushButton::clicked, this, &MainWindow::currStaffAbnormalExport);

    pAllStaffExportBtn->setObjectName("allStaffExportBtn");
    pAllStaffExportBtn->setText("全部人员详细导出");
    connect(pAllStaffExportBtn, &QPushButton::clicked, this, &MainWindow::allStaffExport);

    pAllStaffAbnormalExportBtn->setObjectName("allStaffAbnormalExportBtn");
    pAllStaffAbnormalExportBtn->setText("全部人像异常导出");
    connect(pAllStaffAbnormalExportBtn, &QPushButton::clicked, this, &MainWindow::allStaffAbnormalExport);

    QGridLayout *pLayout = new QGridLayout;
    pLayout->setSpacing(10);
    pLayout->addWidget(m_pStartDateEdit, 0, 0);
    pLayout->addWidget(m_pEndDateEdit, 0, 1);
    pLayout->addWidget(pCurStaffExportBtn, 1, 0);
    pLayout->addWidget(pCurStaffAbnormalExportBtn, 1, 1);
    pLayout->addWidget(pAllStaffExportBtn, 2, 0);
    pLayout->addWidget(pAllStaffAbnormalExportBtn, 2, 1);

    pStatisticsExportWidget->setLayout(pLayout);

    return pStatisticsExportWidget;
}

QWidget *MainWindow::initStatisticsStaffListWidget()
{
    m_pStaffListWidget = new QListWidget;
    m_pStaffListWidget->setViewMode(QListView::IconMode);
    m_pStaffListWidget->setResizeMode(QListView::Adjust);
    m_pStaffListWidget->setIconSize(QSize(80, 80));
    m_pStaffListWidget->setSpacing(15);
    m_pStaffListWidget->setSortingEnabled(true);
    m_pStaffListWidget->setGridSize(QSize(100, 100));
    m_pStaffListWidget->setWrapping(true);
    m_pStaffListWidget->setMovement(QListView::Static);

    connect(m_pStaffListWidget, &QListWidget::itemClicked, this, &MainWindow::avatarListItemClicked);

    return m_pStaffListWidget;
}

QWidget *MainWindow::initStatisticsOtherWidget()
{
    m_pStartStatusLbl = new QLabel;
    m_pEndStatusLbl   = new QLabel;
    m_pStartTimeEdit  = new QTimeEdit(QTime(8, 30));
    m_pEndTimeEdit    = new QTimeEdit(QTime(17, 30));
    m_pTotalLbl       = new QLabel;

    QWidget *pStatisticsOtherWidget = new QWidget;
    QGridLayout *pLayout = new QGridLayout;

    pStatisticsOtherWidget->setObjectName("statisticsOtherWidget");

    m_pStartStatusLbl->setText("打卡情况");
    m_pEndStatusLbl->setText("打卡情况");

    pLayout->setSpacing(10);
    pLayout->setColumnStretch(1,1);
    pLayout->addWidget(new QLabel("上班时间:"), 0, 0, Qt::AlignLeft);
    pLayout->addWidget(m_pStartTimeEdit,       0, 1, Qt::AlignLeft);
    pLayout->addWidget(m_pStartStatusLbl,      0, 2, Qt::AlignRight);

    pLayout->addWidget(new QLabel("下班时间:"), 1, 0, Qt::AlignLeft);
    pLayout->addWidget(m_pEndTimeEdit,         1, 1, Qt::AlignLeft);
    pLayout->addWidget(m_pEndStatusLbl,        1, 2, Qt::AlignRight);

    pLayout->addWidget(m_pTotalLbl);
    pStatisticsOtherWidget->setLayout(pLayout);

    return pStatisticsOtherWidget;
}

QWidget *MainWindow::initSettingWidget()
{
    QWidget *ret = new QWidget(this);

    //
    QGroupBox *pCamBox = new QGroupBox("相机", ret);
    m_pCamListBox      = new QComboBox;
    m_pCamResListBox   = new QComboBox;
    m_pCamScanBtn      = new QPushButton;
    m_pCamSwitchBox    = new QCheckBox;
    QHBoxLayout *pHLayout1   = new QHBoxLayout;

    ret->setObjectName("settingWidget");

    m_pCamListBox->setObjectName("camListBox");
    m_pCamListBox->setEditable(true);
    m_pCamListBox->setToolTip(QStringLiteral("可用的摄像头列表"));
    m_pCamListBox->setMinimumWidth(150);
    m_pCamListBox->setMaximumWidth(390);

    m_pCamResListBox->setObjectName("camResBox");
    m_pCamResListBox->setEditable(true);
    m_pCamResListBox->setToolTip(QStringLiteral("可用的分辨率"));
    m_pCamResListBox->setFixedWidth(100);

    m_pCamScanBtn->setObjectName("camScanBtn");
    m_pCamScanBtn->setText("硬件扫描");
    m_pCamScanBtn->setFixedWidth(80);
    connect(m_pCamScanBtn, &QPushButton::clicked, this, &MainWindow::scanDevBtnClicked);

    m_pCamSwitchBox->setObjectName("camSwitchBox");
    connect(m_pCamSwitchBox, &QCheckBox::stateChanged, this, &MainWindow::videoSwitchBoxChecked);

    pHLayout1->setSpacing(10);
    pHLayout1->addWidget(m_pCamSwitchBox, 0);
    pHLayout1->addWidget(m_pCamListBox, 1);
    pHLayout1->addWidget(m_pCamResListBox, 0);
    pHLayout1->addWidget(m_pCamScanBtn, 0);
    pHLayout1->addStretch(1);
    pCamBox->setLayout(pHLayout1);

    //
    QGroupBox *pFaceBox = new QGroupBox("人脸识别", ret);
    m_pLiveThresholdBox = new QDoubleSpinBox;
    m_pMatchThresholdBox = new QDoubleSpinBox;
    QGridLayout *pgLayout2 = new QGridLayout;

    m_pLiveThresholdBox->setRange(0, 1);
    m_pMatchThresholdBox->setRange(0, 1);

    m_pLiveThresholdBox->setValue(0.4);
    m_pMatchThresholdBox->setValue(0.7);

    m_pLiveThresholdBox->setSingleStep(0.1);
    m_pMatchThresholdBox->setSingleStep(0.1);

    connect(m_pLiveThresholdBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::liveThresholdvalueChanged);
    connect(m_pMatchThresholdBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::matchThresholdvalueChanged);

    pgLayout2->addWidget(new QLabel("比较阈值（推荐0.7）"), 0, 0, 1, 1, Qt::AlignLeft);
    pgLayout2->addWidget(m_pMatchThresholdBox, 0, 1, 1, 1, Qt::AlignLeft);
    pgLayout2->addWidget(new QLabel("活体阈值（推荐0.4）"), 1, 0, 1, 1, Qt::AlignLeft);
    pgLayout2->addWidget(m_pLiveThresholdBox, 1, 1, 1, 1, Qt::AlignLeft);
    pgLayout2->setColumnStretch(1, 1);
    pgLayout2->setSpacing(10);
    pFaceBox->setLayout(pgLayout2);

    //
    QGroupBox *pOtherBox = new QGroupBox("其它", ret);
    QVBoxLayout *pVLayout1 = new QVBoxLayout;

    pOtherBox->setEnabled(false);

    pVLayout1->addWidget(new QCheckBox("人脸抓拍"));
    pVLayout1->addWidget(new QCheckBox("钉钉同步"));
    pOtherBox->setLayout(pVLayout1);

    //
    QGroupBox *pSdkBox    = new QGroupBox("SDK");
    QFormLayout *pFLayout = new QFormLayout;
    m_pSdkAppIdEdit = new QLineEdit;
    m_pSdkKeyEdit   = new QLineEdit;
    m_pArcSdkTime   = new QLabel;

    m_pSdkAppIdEdit->setObjectName("sdkAppIdEdit");
    connect(m_pSdkAppIdEdit, static_cast<void(QLineEdit::*)(void)>(&QLineEdit::editingFinished), this, &MainWindow::sdkAppIdEditEditingFinished);
    m_pSdkKeyEdit->setObjectName("sdkKeyEdit");
    connect(m_pSdkKeyEdit, static_cast<void(QLineEdit::*)(void)>(&QLineEdit::editingFinished), this, &MainWindow::sdkKeyEditEditingFinished);

    pFLayout->addRow("AppID", m_pSdkAppIdEdit);
    pFLayout->addRow("Key",   m_pSdkKeyEdit);
    pFLayout->addWidget(m_pArcSdkTime);
    pSdkBox->setLayout(pFLayout);

    //
    QHBoxLayout *pVLayout2 = new QHBoxLayout;
    m_pAutoStartBox   = new QCheckBox;
    QLabel *pHelpLbl       = new QLabel;

    m_pAutoStartBox->setText("开机自启动");
    connect(m_pAutoStartBox, &QCheckBox::stateChanged, this, &MainWindow::autoStartBoxChecked);

    pHelpLbl->setText(QString("<a style='color: blue;' href=\"%1\">帮 助").arg(helpUrl.toString()));
    pHelpLbl->setOpenExternalLinks(true);

    pVLayout2->setSpacing(0);
    pVLayout2->addWidget(m_pAutoStartBox, 0);
    pVLayout2->addStretch(1);
    pVLayout2->addWidget(pHelpLbl, 0);

    //
    QVBoxLayout *pVLayout = new QVBoxLayout;

    pVLayout->addWidget(pCamBox, 0);
    pVLayout->addWidget(pFaceBox, 0);
    pVLayout->addWidget(pOtherBox, 0);
    pVLayout->addWidget(pSdkBox, 0);
    pVLayout->addStretch(1);
    pVLayout->addLayout(pVLayout2, 0);

    ret->setLayout(pVLayout);

    return ret;
}

void MainWindow::initSystemTrayIcon()
{
    m_pSystemTrayIcon = new QSystemTrayIcon(this);
    QMenu *menu = new QMenu("简单考勤");

    m_pSystemTrayIcon->setToolTip("简单考勤");

    menu->addAction("帮助", this, [&](){
        QDesktopServices::openUrl(helpUrl);
    });

    menu->addAction("关于", this, [&](){
        QDesktopServices::openUrl(QUrl("file:///C:/Users/TianSong/Desktop/sql.txt"));
    });

    menu->addAction("退出", this, &MainWindow::closeBtnClicked);

    connect(m_pSystemTrayIcon, &QSystemTrayIcon::activated, this, [&](QSystemTrayIcon::ActivationReason reason){
        if (reason == QSystemTrayIcon::Trigger)
        {
            showNormal();
            raise();
            activateWindow();
        }
    });

    m_pSystemTrayIcon->setContextMenu(menu);

    m_pSystemTrayIcon->setIcon(QIcon(":/image/logo.png"));
    m_pSystemTrayIcon->setToolTip("简单考勤");
    m_pSystemTrayIcon->show();
    m_pSystemTrayIcon->showMessage("服务启动", "祝你活力满满一整天", QSystemTrayIcon::Information, 0);
}

void MainWindow::initWaitDialog()
{
    m_pWaitDialog = new WaitDialog(this);

    m_pWaitDialog->setMovie(new QMovie(":/image/loading.gif"));
    m_pWaitDialog->setFixedSize(150, 150);
    m_pWaitDialog->setScaledContents(true);
}




