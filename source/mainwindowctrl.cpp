#include "mainwindow.h"

#include <QCameraInfo>
#include <QCryptographicHash>
#include <QDateTime>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFuture>
#include <QList>
#include <QMessageBox>
#include <QMetaObject>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QString>
#include <QtConcurrent>
#include <QTextStream>
#include <QTimer>
#include <QVariant>

#include <windows.h>
#include <winuser.h>

#include "commonhelper/commonhelper.h"
#include "logindialog/QLoginDialog.h"

QImage MatToQImage(const cv::Mat& mat);
cv::Mat QImage2cvMat(const QImage &image);

void MainWindow::initControl()
{
    m_pVideoCapture.reset(new VideoCapture);

    scanDevBtnClicked();
    importDatabase();
    readSettings();

    m_pFaceDetect.reset(new FaceDetect(m_pSdkAppIdEdit->text(),   m_pSdkKeyEdit->text(), m_activeKey));
    m_pFaceFeature.reset(new FaceFeature(m_pSdkAppIdEdit->text(), m_pSdkKeyEdit->text(), m_activeKey));
    m_pFaceFeature->setFeaturesHash(&m_FeaturesHash);

    readTextFile(QApplication::applicationDirPath() + "/voice.txt", m_speechTextList);
    readTextFile(QApplication::applicationDirPath() + "/hint.txt", m_tooltipTextList);

    if (m_speechTextList.count() == 0)
        m_speechTextList.append("好");
    if (m_tooltipTextList.count() == 0)
        m_tooltipTextList.append("相信美好的事情即将发生");

    topBtnClicked();

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [&](){
        m_pTimeLbl->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss ddd"));
    });
    timer->start(1000);

    m_tooltipTextTimer.start(5000);
}

void MainWindow::initSlots()
{
    connect(&m_faceInfoLastTimer, &QTimer::timeout, this, [&](){
        m_faceLastName.clear();
        m_pFaceInfoLbl->setText("");
    });

    connect(&m_tooltipTextTimer, &QTimer::timeout, this, [&](){
        m_pTooltipLbl->setText(m_tooltipTextList.at(QRandomGenerator::global()->generate() % m_tooltipTextList.count()));
    });

    connect(m_pFaceDetect.get(), &FaceDetect::statusChanged, this, [&](bool isAvailable) {
        if (isAvailable)
        {
            QString startTime, endTime, platform, sdkType, appId, sdkKey, sdkVersion, fileVersion;
            if (m_pFaceDetect->getActiveFileInfo(startTime, endTime, platform, sdkType, appId, sdkKey, sdkVersion, fileVersion))
            {
                QString sdkstartTime = QDateTime::fromSecsSinceEpoch(startTime.toLongLong()).toString("yyyy.MM.dd");
                QString sdkEndTime = QDateTime::fromSecsSinceEpoch(endTime.toLongLong()).toString("yyyy.MM.dd");

                m_pArcSdkTime->setText("(SDK有效期：" + sdkstartTime + " - " + sdkEndTime + ")");
            }
        }
        else
        {
            m_pSystemTrayIcon->showMessage("人脸识别SDK", "激活失败", QSystemTrayIcon::Information, 0);
        }
    });

    connect(m_pFaceFeature.get(), &FaceFeature::statusChanged, this, [&](bool isAvailable){
        Q_UNUSED(isAvailable)
    });

    connect(m_pVideoCapture.get(), &VideoCapture::statusChanged,  this, [&](bool isOpened){
        if (isOpened && m_pVideoCapture->getResolutions() != m_pCamResListBox->currentData().toSize())
        {
            m_pVideoCapture->setResolutions(m_pCamResListBox->currentData().toSize());
        }
        else
        {
            m_pVideoLbl->setMovie(m_pVideoMovie);
            m_pFaceInfoLbl->setText("");
            m_pFaceRectLbl->resize(0,0);
        }
    });

    connect(m_pVideoCapture.get(), static_cast<void(VideoCapture::*)(cv::Mat)>(&VideoCapture::updateImage), this, &MainWindow::videoUpdateImage);

    connect(m_pFaceDetect.get(), &FaceDetect::exportRect, this, &MainWindow::faceRectUpdate);

    connect(m_pFaceDetect.get(), &FaceDetect::exportFaceInfo, this, &MainWindow::faceDetectUpdate);

    connect(m_pFaceFeature.get(), &FaceFeature::exportMatchInfo, this, [&](QString info){
        if (info != m_faceLastName)
        {
            QSqlQuery selectQuery(QString("select * from p%1 where date = date('now', 'localtime')").arg(nameToMdk5(info)));

            if (selectQuery.exec() && selectQuery.next())
            {
                QSqlQuery query;
                QSqlQuery().exec(QString("update p%1 set offtime = time('now', 'localtime') where date = date('now', 'localtime')").arg(nameToMdk5(info)));
            }
            else
            {
                QSqlQuery query;
                QSqlQuery().exec(QString("insert into p%1(ontime) values(time('now', 'localtime'))").arg(nameToMdk5(info)));
            }

            m_faceLastName = info;
            m_pFaceInfoLbl->setText(std::move(info));
            CommonHelper::textToSpeech(info + "  " + m_speechTextList.at(QRandomGenerator::global()->generate() % m_speechTextList.count()), 1, 0, 1);
        }

        m_faceInfoLastTimer.start(2000);
    });

    connect(m_pVideoCapture.get(), static_cast<void(VideoCapture::*)(QImage)>(&VideoCapture::updateImage), this, [&](QImage img){
        m_originImageSize.setWidth(img.width());
        m_originImageSize.setHeight(img.height());
        m_pVideoLbl->setPixmap(QPixmap::fromImage(img.scaled(m_pVideoLbl->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    });
}

bool MainWindow::initDataBase()
{
    m_dataBase = QSqlDatabase::addDatabase("QSQLITE");
    m_dataBase.setDatabaseName("personnel.dat");

    if (!m_dataBase.open())
        return false;

    QString createPersonnelForm("create table if not exists personnel"
                                "([name] varchar(12) primary key,"
                                "[regtime] timestamp not null default (datetime('now','localtime')),"
                                "[feature] blob);");

    QSqlQuery query;
    if (!query.exec(createPersonnelForm))
        return false;

    return true;
}

void MainWindow::initPicDir()
{
    m_picPath = QCoreApplication::applicationDirPath() + "/pic";
    QDir dir(m_picPath);

    if (!dir.exists())
        dir.mkdir(m_picPath);
}

bool MainWindow::importDatabase()
{
    QSqlQuery query("select name, regtime, feature from personnel");

    if (!query.exec())
        return false;

    while (query.next())
    {
        ASF_FaceFeature faceFeature = {0, 0};

        QString name = query.value(0).toString();
        QString time = query.value(1).toString();

        faceFeature.featureSize = 1032;
        faceFeature.feature = (MByte *)malloc(faceFeature.featureSize * sizeof(MByte));

        memcpy(faceFeature.feature, query.value(2).toByteArray().data(), faceFeature.featureSize);
        m_FeaturesHash.insert(name, faceFeature);

        addAvatarList(name, time, QImage(m_picPath + "/" + nameToMdk5(name)));
    }

    m_pTotalLbl->setText("总计" + QString::number(m_FeaturesHash.count()) + "人");
    m_pTotalCntLbl->setText(QString::number(m_FeaturesHash.count()));

    return true;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_pTitleWidget)
    {
        QMouseEvent *me = dynamic_cast<QMouseEvent*>(e);
        if ((me != nullptr))
        {
            switch (e->type())
            {
            case QEvent::MouseButtonPress:
                if (me->button() == Qt::LeftButton && !isMaximized())
                {
                    m_mousePoint = me->pos();
                    isMousePressed = true;
                }
                break;

            case QEvent::MouseButtonRelease:
                if (me->button() == Qt::LeftButton)
                    isMousePressed = false;
                break;

            case QEvent::MouseMove:
                if (isMousePressed)
                    move( this->cursor().pos() - m_mousePoint);
                break;

            default:
                break;
            }

            return true;
        }
    }
    else if (obj == m_pVideoWidget && e->type() == QEvent::Resize)
    {
        m_pVideoLbl->resize(m_pVideoWidget->size());
    }

    return QDialog::eventFilter(obj, e);
}

void MainWindow::topBtnClicked()
{
    if (m_pTopBtn->isChecked())
        ::SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    else
        ::SetWindowPos(HWND(this->winId()), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

void MainWindow::minBtnClicked()
{
    showMinimized();
}

void MainWindow::maxBtnClicked()
{
    QPushButton* btn = static_cast<QPushButton*>(sender());

    if (isMaximized())
    {
          btn->setToolTip("最大化");
        IconHelper::setIcon(btn, QChar(0xf05b), 12);
        showNormal();
    }
    else
    {
        btn->setToolTip("向下还原");
        IconHelper::setIcon(btn, QChar(0xf066), 12);
        showMaximized();
    }
}

void MainWindow::closeBtnClicked()
{
    if (m_pLoginBtn->isChecked())
    {
        writeSettings();
        close();
    }
    else
    {
        m_pSystemTrayIcon->showMessage("系统提示", "请登陆后退出", QSystemTrayIcon::Information, 0);
    }
}

void MainWindow::loginBtnChecked(bool checked)
{
    bool ok = false;

    if (checked)
    {
        QLoginDialog ld(this);

        if (ld.exec() == QDialog::Accepted)
        {
            if (ld.getPassword() == "admin" && ld.getUser() == "admin")
            {
                ok = true;
            }
            else
            {
                QMessageBox::information(this, "登录", "用户名或密码错误");
                QPushButton *btn = static_cast<QPushButton*>(sender());
                btn->setChecked(false);
            }
        }
        else
        {
            QPushButton *btn = static_cast<QPushButton*>(sender());
            btn->setChecked(false);
        }
    }

    if (!ok) {
        m_pCamBtn->setChecked(true);
        navigationCamBtnClicked();
    }

    m_pCamBtn->setEnabled(ok);
    m_pRegisterBtn->setEnabled(ok);
    m_pStatisticsBtn->setEnabled(ok);
    m_pSettingBtn->setEnabled(ok);
}

void MainWindow::navigationCamBtnClicked()
{
    connect(m_pFaceDetect.get(), &FaceDetect::exportFaceInfo, this, &MainWindow::faceDetectUpdate);
    m_pStackedWidget->setCurrentIndex(0);
}

void MainWindow::navigationRegisterBtnClicked()
{
    disconnect(m_pFaceDetect.get(), &FaceDetect::exportFaceInfo, this, &MainWindow::faceDetectUpdate);
    m_pStackedWidget->setCurrentIndex(1);
}

void MainWindow::navigationStatisticsBtnClicked()
{
    disconnect(m_pFaceDetect.get(), &FaceDetect::exportFaceInfo, this, &MainWindow::faceDetectUpdate);
    m_pStackedWidget->setCurrentIndex(2);
}

void MainWindow::navigationSettingBtnClicked()
{
    disconnect(m_pFaceDetect.get(), &FaceDetect::exportFaceInfo, this, &MainWindow::faceDetectUpdate);
    m_pStackedWidget->setCurrentIndex(3);
}

void MainWindow::registerAvatarBtnClicked()
{
    QFileDialog fd(this);
    fd.setWindowTitle("选择注册的图片");
    fd.setAcceptMode(QFileDialog::AcceptOpen);
    fd.setFileMode(QFileDialog::ExistingFiles);
    fd.setNameFilter("Image (*.jpeg *.jpg *.png)");

    if (fd.exec() == QFileDialog::Accepted)
    {
        m_registerFilesPath = fd.selectedFiles();

        QPixmap img(m_registerFilesPath.at(0));
        if (!img.isNull())
            m_pRegisteAvatarBtn->setIcon(std::move(img));

        if (m_registerFilesPath.count() == 1)
            m_pRegisterNameEdit->setText(QFileInfo(m_registerFilesPath.at(0)).baseName());
        else
            m_pRegisterNameEdit->setText(QString("批量注册：%1").arg(m_registerFilesPath.count()));
    }
}

void MainWindow::registerCancelActionTriggered()
{
    m_pRegisteAvatarBtn->setIcon(QIcon(":/image/avatarwoman.png"));
    m_pRegisterNameEdit->clear();
    m_registerFilesPath.clear();
}

void MainWindow::registerBtnClicked()
{
    m_pRegisterLogEdit->appendPlainText("[ " + QDateTime::currentDateTime().toString() + " ] - [ 注册 ]");

    if (m_registerFilesPath.count() == 0)
    {
        m_pSystemTrayIcon->showMessage("人员注册", "请选择待注册人员", QSystemTrayIcon::Information, 0);
        m_pRegisterLogEdit->appendPlainText("未选中待注册图片");
        return;
    }

    uint32_t curIndex = m_registerFilesPath.count();
    uint32_t sucCnt = 0;
    uint32_t failCnt = 0;
    ASF_SingleFaceInfo faceInfo = {{0, 0, 0, 0}, 0};
    ASF_FaceFeature faceFeature = {0, 0};

    m_dataBase.transaction();

    QSqlQuery insertQuery;
    insertQuery.prepare("insert into personnel(name, feature) values(?,?)");

    m_pWaitDialog->show();

    for (const auto &item : m_registerFilesPath)
    {
        cv::Mat cvImage = cv::imread(item.toLocal8Bit().data());
        QImage  qimage  = MatToQImage(cvImage);

        QString baseName = QFileInfo(item).baseName();

        m_pRegisteAvatarBtn->setIcon(QPixmap::fromImage(qimage));
        m_pRegisterNameEdit->setText(QString::number(curIndex--, 10) + ":" + baseName);

        if (m_FeaturesHash.contains(baseName))
        {
            ++failCnt;
            m_pRegisterLogEdit->appendPlainText("失败 >>> " + item + " >>> 姓名已存在");
            continue;
        }

        QFuture<bool> future1 = QtConcurrent::run(m_pFaceFeature.data(), &FaceFeature::getFaceInfo, std::ref(cvImage), std::ref(faceInfo));
        while (!future1.isFinished()) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        if (!future1.result())
        {
            ++failCnt;
            m_pRegisterLogEdit->appendPlainText("失败 >>> " + item + " >>> 头像质量低请更换后尝试");
            continue;
        }

        QFuture<bool> future2 = QtConcurrent::run(m_pFaceFeature.data(), &FaceFeature::getFeatures, std::ref(cvImage), std::ref(faceInfo), std::ref(faceFeature));
        while (!future2.isFinished()) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        if (!future2.result())
        {
            ++failCnt;
            m_pRegisterLogEdit->appendPlainText("失败 >>> " + item + " >>> 头像质量低请更换后尝试");
            free(faceFeature.feature);
            continue;
        }

        auto iter = m_FeaturesHash.begin();
        for (; iter != m_FeaturesHash.end(); ++iter)
        {
            float confidenceLevel = 0.0f;
            QFuture<bool> future3 = QtConcurrent::run(m_pFaceFeature.data(), &FaceFeature::facePairMatching, std::ref(confidenceLevel), std::ref(faceFeature), std::ref(iter.value()));
            while (!future3.isFinished()) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
            if (future3.result() && confidenceLevel > 0.8)
            {
                ++failCnt;
                m_pRegisterLogEdit->appendPlainText("失败 >>> " + item + " >>> 头像已被 【 " + iter.key() + " 】使用，请重新确认");
                free(faceFeature.feature);
                break;
            }
        }

        if (iter == m_FeaturesHash.end())
        {
            insertQuery.bindValue(0, baseName);
            insertQuery.bindValue(1, QByteArray((const char*)(faceFeature.feature), faceFeature.featureSize * sizeof(MByte)));

            QSqlQuery createQuery(QString("create table if not exists %1([date] timestamp not null default (date('now','localtime')),"
                                          "[ontime] timestamp,"
                                          "[offtime] timestamp);").arg("p" + nameToMdk5(baseName)));
            if (createQuery.exec() && insertQuery.exec())
            {
                ++sucCnt;
                m_FeaturesHash.insert(baseName, faceFeature);
                m_pRegisterLogEdit->appendPlainText("成功 >>> " + item);
                addAvatarList(baseName, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), std::move(qimage));

                QFile imgFile(item);
                if (imgFile.open(QIODevice::ReadOnly))
                {
                    imgFile.copy(m_picPath + "/" + nameToMdk5(baseName));
                    imgFile.close();
                }
            }
            else
            {
                m_pRegisterLogEdit->appendPlainText("数据库插入失败： " + item + " (" + insertQuery.lastError().text() + ")");;
            }
        }
    }
    m_dataBase.commit();

    m_pRegisterLogEdit->appendPlainText(QString("结束 >>> ") + "总计:" + QString::number(m_registerFilesPath.count()) + " 成功：" + QString::number(sucCnt) + " 失败：" + QString::number(failCnt));
    m_pTotalCntLbl->setText(QString::number(m_FeaturesHash.count()));
    m_pTotalLbl->setText("总计" + QString::number(m_FeaturesHash.count()) + "人");
    m_pSystemTrayIcon->showMessage("人员注册", QString("成功: %1 , 失败: %2").arg(m_registerFilesPath.count()).arg(failCnt), QSystemTrayIcon::Information, 0);
    m_pRegisterNameEdit->setText("");
    m_pRegisteAvatarBtn->setIcon(QIcon(":/image/avatarwoman.png"));
    m_pRegisterNameEdit->clear();
    m_registerFilesPath.clear();

    m_pWaitDialog->hide();
}

void MainWindow::videoUpdateImage(cv::Mat image)
{
    if (m_pFaceDetect->m_curFaceInfoCnt < 1)
    {
        ++m_pFaceDetect->m_curFaceInfoCnt;

        QMetaObject::invokeMethod(m_pFaceDetect.get(), "importImage", Qt::QueuedConnection, Q_ARG(cv::Mat, std::move(image)));
    }
}

void MainWindow::faceDetectUpdate(cv::Mat image, ASF_SingleFaceInfo faceInfo)
{
    if (m_pFaceFeature->m_curFaceFeatureCnt < 1)
    {
        ++m_pFaceFeature->m_curFaceFeatureCnt;

        QMetaObject::invokeMethod(m_pFaceFeature.get(), "importFaceInfo", Qt::QueuedConnection, Q_ARG(cv::Mat, std::move(image)), Q_ARG(ASF_SingleFaceInfo, std::move(faceInfo)));
    }
}

void MainWindow::faceRectUpdate(QRect rect)
{
    const auto &currSize = m_pVideoWidget->size();

    auto scale_x = 1.0 * currSize.width() / m_originImageSize.width();
    auto scale_y = 1.0 * currSize.height() / m_originImageSize.height();

    m_pFaceRectLbl->setGeometry(QRect(rect.x() * scale_x, rect.y() * scale_y, rect.width() * scale_x, rect.height() * scale_y));
}

void MainWindow::readTextFile(const QString &path, QStringList &list)
{
    QFile file(path);

    if (file.open(QIODevice::ExistingOnly | QIODevice::ReadOnly))
    {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");

        while (!stream.atEnd())
            list.append(stream.readLine());

        file.close();
    }
}

void MainWindow::addAvatarList(const QString &name, const QString &time, const QImage &img)
{
    QListWidgetItem *item = new QListWidgetItem(m_pStaffListWidget);
    item->setText(name);
    item->setToolTip(time);
    item->setIcon(QPixmap::fromImage(img));
    item->setTextAlignment(Qt::AlignCenter);
    m_pStaffListWidget->addItem(item);
}

void MainWindow::updateStaffStatus(const QString &name)
{
    QSqlQuery query(QString("select ontime, offtime from p%1 where date = date('now', 'localtime')").arg(nameToMdk5(name)));

    QString onTime;
    QString offTime;

    if (query.exec() && query.next())
    {
        onTime  = query.value(0).toString();
        offTime = query.value(1).toString();
    }

    m_pStartStatusLbl->setText(onTime.isEmpty() ? "未打卡" : onTime);
    m_pEndStatusLbl->setText(offTime.isEmpty() ? "未打卡" : offTime);
}

inline QString MainWindow::nameToMdk5(const QString &name)
{
    return QString(QCryptographicHash::hash(name.toLocal8Bit(), QCryptographicHash::Sha1).toHex());
}

QString MainWindow::getExportAttendanceFilePath()
{
    QString path;

    QFileDialog fd(this);
    fd.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    fd.setFileMode(QFileDialog::Directory);
    fd.setWindowTitle("选择保存路径");
    if (fd.exec() == QDialog::Accepted)
        path = fd.selectedFiles().at(0);

    return path;
}

bool MainWindow::isNetStream(const QString name)
{
    if (name.contains("rtmp", Qt::CaseInsensitive) || name.contains("rtsp", Qt::CaseInsensitive) || name.contains("http", Qt::CaseInsensitive) || CommonHelper::isIP(name))
        return true;

    return false;
}

bool MainWindow::exportAttendanceFile(const QString &name, const QString savePath, bool isAbnormal, bool isSingle)
{
    if (m_pEndTimeEdit->time() < m_pStartTimeEdit->time())
    {
        m_pSystemTrayIcon->showMessage("考勤信息导出失败", "请确保上班时间晚于下班时间", QSystemTrayIcon::Information, 0);
        return false;
    }

    QFile file(savePath + '/' + name + (isAbnormal ? "_异常考勤_" : "_全部考勤_") + QDateTime::currentDateTime().toString("yyyyMMddhhmmss") + ".csv");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        m_pSystemTrayIcon->showMessage("考勤信息导出失败", "没有足够的权限创建文件，请更换路径后重试", QSystemTrayIcon::Information, 0);
        return false;
    }

    QTextStream out(&file);
    out << QString::fromUtf8("日期,") << QString::fromUtf8("上班打卡时间,") << QString::fromUtf8("下班打卡时间,") << QString::fromUtf8("今日时长\n");

    int dureTime = m_pStartTimeEdit->time().secsTo(m_pEndTimeEdit->time());

    uint32_t recordsCnt = 0;
    QString select = QString("select * from p%1 where date >= '%2' and date <= '%3'").arg(nameToMdk5(name)).arg(m_pStartDateEdit->date().toString("yyyy-MM-dd")).arg(m_pEndDateEdit->date().toString("yyyy-MM-dd"));
    QSqlQuery query;
    QDate date_last;

    if (query.exec(select))
    {
        if (query.next())
        {
            QDate date    = query.value(0).toDate();
            QTime onTime  = query.value(1).toTime();
            QTime offTime = query.value(2).toTime();

            if (isAbnormal)
            {
                if (!offTime.isValid() || (offTime.isValid() && dureTime > onTime.secsTo(offTime)))
                {
                    out << date.toString("yyyy-MM-dd")  << ',';
                    out << onTime.toString("hh:mm:ss")  << ',';
                    out << offTime.toString("hh:mm:ss") << ',';

                    if (offTime.isValid())
                        out << QTime::fromMSecsSinceStartOfDay(onTime.msecsTo(offTime)).toString("hh:mm:ss");
                    else
                        out << "0";

                    out << '\n';
                    date_last = date.addDays(1);
                    ++recordsCnt;
                }
            }
            else
            {
                out << date.toString("yyyy-MM-dd")  << ',';
                out << onTime.toString("hh:mm:ss")  << ',';
                out << offTime.toString("hh:mm:ss") << ',';

                if (offTime.isValid())
                    out << QTime::fromMSecsSinceStartOfDay(onTime.msecsTo(offTime)).toString("hh:mm:ss");
                else
                    out << "0";

                out << '\n';
                ++recordsCnt;
            }
            date_last = date.addDays(1);
        }

        while (query.next())
        {
            QDate date = query.value(0).toDate();

            while (date_last != date && date_last <= m_pEndDateEdit->date())
            {
                out << date_last.toString("yyyy-MM-dd") << ',' << ',' << ',' << '0' << '\n';
                date_last = date_last.addDays(1);
                ++recordsCnt;
            }

            if (date_last == date)
            {
                QDate date    = query.value(0).toDate();
                QTime onTime  = query.value(1).toTime();
                QTime offTime = query.value(2).toTime();

                if (isAbnormal)
                {
                    if (!offTime.isValid() || (offTime.isValid() && dureTime > onTime.secsTo(offTime)))
                    {
                        out << date.toString("yyyy-MM-dd")  << ',';
                        out << onTime.toString("hh:mm:ss")  << ',';
                        out << offTime.toString("hh:mm:ss") << ',';

                        if (offTime.isValid())
                            out << QTime::fromMSecsSinceStartOfDay(onTime.msecsTo(offTime)).toString("hh:mm:ss");
                        else
                            out << "0";

                        out << '\n';
                        date_last = date.addDays(1);
                        ++recordsCnt;
                    }
                }
                else
                {
                    out << date.toString("yyyy-MM-dd")  << ',';
                    out << onTime.toString("hh:mm:ss")  << ',';
                    out << offTime.toString("hh:mm:ss") << ',';

                    if (offTime.isValid())
                        out << QTime::fromMSecsSinceStartOfDay(onTime.msecsTo(offTime)).toString("hh:mm:ss");
                    else
                        out << "0";

                    out << '\n';
                    date_last = date.addDays(1);
                    ++recordsCnt;
                }
            }
        }
    }

    if (isSingle)
        m_pSystemTrayIcon->showMessage("导出成功", "总计 " + QString::number(recordsCnt) + " 条", QSystemTrayIcon::Information, 0);

    out.flush();
    file.close();

    return true;
}

void MainWindow::videoSwitchBoxChecked(int state)
{
    if (state)
    {
        if (isNetStream(m_pCamListBox->currentText()))
            QMetaObject::invokeMethod(m_pVideoCapture.get(), "openCamara", Qt::QueuedConnection, Q_ARG(QString, m_pCamListBox->currentText()));
        else
            QMetaObject::invokeMethod(m_pVideoCapture.get(), "openCamara", Qt::QueuedConnection, Q_ARG(int, m_pCamListBox->currentIndex()));

       m_pCamListBox->setEnabled(false);
       m_pCamResListBox->setEnabled(false);
       m_pCamScanBtn->setEnabled(false);
    }
    else
    {
        QMetaObject::invokeMethod(m_pVideoCapture.get(), "closeCamara", Qt::QueuedConnection);

        m_pCamListBox->setEnabled(true);
        m_pCamResListBox->setEnabled(true);
        m_pCamScanBtn->setEnabled(true);
    }
}

void MainWindow::scanDevBtnClicked()
{
    m_pCamListBox->clear();
    m_pCamResListBox->clear();

    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo : cameras)
        m_pCamListBox->addItem(cameraInfo.description());

    m_pCamListBox->lineEdit()->setPlaceholderText(QStringLiteral("可解析的视频流"));

    //m_pCamResListBox->addItem("160 x 120", QSize(160, 120));
    //m_pCamResListBox->addItem("320 x 180", QSize(320, 180));
    m_pCamResListBox->addItem("320 x 240", QSize(320, 240));
    m_pCamResListBox->addItem("424 x 240", QSize(424, 240));
    m_pCamResListBox->addItem("640 x 360", QSize(640, 360));
    m_pCamResListBox->addItem("640 x 480", QSize(640, 480));
    m_pCamResListBox->addItem("848 x 480", QSize(848, 480));
    m_pCamResListBox->addItem("960 x 540", QSize(960, 540));
    //m_pCamResListBox->addItem("1280 x 720", QSize(1280, 720));

    m_pCamResListBox->setCurrentIndex(3);
}

void MainWindow::liveThresholdvalueChanged(double d)
{
    m_pFaceFeature->setLiveThreshold(d);
}

void MainWindow::matchThresholdvalueChanged(double d)
{
    m_pFaceFeature->setMatchThreshold(d);
}

void MainWindow::autoStartBoxChecked(int state)
{
    CommonHelper::autoRunWithSystem(state, qApp->applicationName(), qApp->applicationFilePath());
}

void MainWindow::avatarListItemClicked(QListWidgetItem *item)
{
    QSqlQuery query(QString("select name, regtime from personnel where name = '%1'").arg(item->text()));
    if (query.exec() && query.next())
    {
        QString name = query.value(0).toString();
        m_pStatisticsSearchEdit->setText(name);
        m_pStatisticsTimeLbl->setText(query.value(1).toString());
        m_pStatisticsAvatarLbl->setPixmap(QPixmap::fromImage(QImage(m_picPath + "/" + nameToMdk5(name)).scaled(m_pStatisticsAvatarLbl->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));

        updateStaffStatus(name);
    }
}

void MainWindow::statisticsFindBtnClicked()
{
    if (m_pStatisticsSearchEdit->text().isEmpty())
    {
        m_pSystemTrayIcon->showMessage("查找失败", "请填写被查找姓名", QSystemTrayIcon::Information, 0);
        statisticsCancelActionTriggered();
        return;
    }

    QSqlQuery query(QString("select name, regtime from personnel where name like '%%1%'").arg(m_pStatisticsSearchEdit->text()));
    if (query.exec() && query.next())
    {
        QString name = query.value(0).toString();
        m_pStatisticsSearchEdit->setText(name);
        m_pStatisticsTimeLbl->setText(query.value(1).toString());
        m_pStatisticsAvatarLbl->setPixmap(QPixmap::fromImage(QImage(m_picPath + "/" + nameToMdk5(name)).scaled(m_pStatisticsAvatarLbl->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));

        updateStaffStatus(name);
    }
    else
    {
        m_pSystemTrayIcon->showMessage("查找失败", "被查找姓名不存在", QSystemTrayIcon::Information, 0);
        statisticsCancelActionTriggered();
    }
}

void MainWindow::statisticsDeleteBtnClicked()
{
    if (m_pStatisticsSearchEdit->text().isEmpty())
    {
        m_pSystemTrayIcon->showMessage("删除失败", "请填写被删除姓名", QSystemTrayIcon::Information, 0);
        return;
    }

    QString name = m_pStatisticsSearchEdit->text().trimmed();
    QSqlQuery query(QString("delete from personnel where name = '%1'").arg(name));
    if (query.exec())
    {
        QString hashName = nameToMdk5(name);

        QListWidgetItem *item = m_pStaffListWidget->findItems(name, Qt::MatchFixedString).at(0);
        if (item != nullptr)
        {
            m_pStaffListWidget->removeItemWidget(item);
            delete item;
            m_pStaffListWidget->update();
        }

        ASF_FaceFeature feature = m_FeaturesHash.value(name);
        delete feature.feature;
        m_FeaturesHash.remove(name);

        QFile::remove(m_picPath + "/" + hashName);
        QSqlQuery dropTableQuery(QString("drop table if exists p%1").arg(hashName));
        dropTableQuery.exec();

        m_pStatisticsSearchEdit->setText("");
        m_pStatisticsTimeLbl->setText("注册时间");
        m_pStatisticsAvatarLbl->setPixmap(QPixmap(":/image/avatarman.png"));
        m_pTotalCntLbl->setText(QString::number(m_FeaturesHash.count()));
        m_pTotalLbl->setText("总计" + QString::number(m_FeaturesHash.count()) + "人");
    }
    else
    {
        m_pSystemTrayIcon->showMessage("删除失败", name + "不存在", QSystemTrayIcon::Information, 0);
    }
}

void MainWindow::statisticsCancelActionTriggered()
{
    m_pStatisticsSearchEdit->setText("");
    m_pStatisticsTimeLbl->setText("注册时间");
    m_pStatisticsAvatarLbl->setPixmap(QPixmap(":/image/avatarman.png"));
    m_pStartStatusLbl->setText("打卡情况");
    m_pEndStatusLbl->setText("打卡情况");
}

void MainWindow::currStaffExport()
{
    QString name = m_pStatisticsSearchEdit->text();

    if (name.isEmpty())
    {
        m_pSystemTrayIcon->showMessage("当前人员考勤信息导出失败", "请填写要导出的人员姓名", QSystemTrayIcon::Information, 0);
        return;
    }

    if (!m_FeaturesHash.contains(name))
    {
        m_pSystemTrayIcon->showMessage("当前人员考勤信息导出失败", "导出人员姓名不存在", QSystemTrayIcon::Information, 0);
        return;
    }

    exportAttendanceFile(name, getExportAttendanceFilePath(), false);
}

void MainWindow::currStaffAbnormalExport()
{
    QString name = m_pStatisticsSearchEdit->text();

    if (name.isEmpty())
    {
        m_pSystemTrayIcon->showMessage("当前人员考勤信息导出失败", "请填写要导出的人员姓名", QSystemTrayIcon::Information, 0);
        return;
    }

    if (!m_FeaturesHash.contains(name))
    {
        m_pSystemTrayIcon->showMessage("当前人员考勤信息导出失败", "导出人员姓名不存在", QSystemTrayIcon::Information, 0);
        return;
    }

    exportAttendanceFile(name, getExportAttendanceFilePath(), true);
}

void MainWindow::allStaffExport()
{
    QString filePath = getExportAttendanceFilePath();
    int cnt = 0;

    for (auto iter = m_FeaturesHash.begin(); iter != m_FeaturesHash.end(); ++iter)
    {
        ++cnt;
        if (!exportAttendanceFile(iter.key(), filePath, false, false))
            return;
    }

    m_pSystemTrayIcon->showMessage("全部人员详细考勤信息导出完成", QString("总计: %1 人").arg(cnt), QSystemTrayIcon::Information, 0);
}

void MainWindow::allStaffAbnormalExport()
{
    QString filePath = getExportAttendanceFilePath();
    int cnt = 0;

    for (auto iter = m_FeaturesHash.begin(); iter != m_FeaturesHash.end(); ++iter)
    {
        ++cnt;
        if (!exportAttendanceFile(iter.key(), filePath, true, false))
            return;
    }

    m_pSystemTrayIcon->showMessage("全部人员异常考勤信息导出完成", QString("总计: %1 人").arg(cnt), QSystemTrayIcon::Information, 0);
}

void MainWindow::sdkAppIdEditEditingFinished()
{
    m_pSystemTrayIcon->showMessage("SDK 管理", "AppID，将在应用重启后生效", QSystemTrayIcon::Information, 0);
}

void MainWindow::sdkKeyEditEditingFinished()
{
    m_pSystemTrayIcon->showMessage("SDK 管理", "Key，将在应用重启后生效", QSystemTrayIcon::Information, 0);
}

void MainWindow::writeSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(), QCoreApplication::applicationName());

    settings.beginGroup("UI");
    settings.setValue("topBtn",      m_pTopBtn->isChecked());
    settings.setValue("startTime",   m_pStartTimeEdit->time());
    settings.setValue("endTime",     m_pEndTimeEdit->time());
    settings.setValue("startDate",   m_pStartDateEdit->date());
    settings.setValue("endDate",     m_pEndDateEdit->date());
    settings.setValue("camSwitch",   m_pCamSwitchBox->isChecked());
    settings.setValue("camList",     m_pCamListBox->currentText());
    settings.setValue("camResList",  m_pCamResListBox->currentText());
    settings.setValue("liveThreshold",  m_pLiveThresholdBox->value());
    settings.setValue("matchThreshold", m_pMatchThresholdBox->value());
    settings.setValue("sdkAppId",   m_pSdkAppIdEdit->text());
    settings.setValue("sdkKey",     m_pSdkKeyEdit->text());
    settings.setValue("autoStart",  m_pAutoStartBox->isChecked());
    settings.endGroup();
}

void MainWindow::readSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(), QCoreApplication::applicationName());

    settings.beginGroup("UI");

    if (settings.contains("autoStart"))
    {
        m_pTopBtn->setChecked(settings.value("topBtn").toBool());
        m_pStartTimeEdit->setTime(settings.value("startTime").toTime());
        m_pEndTimeEdit->setTime(settings.value("endTime").toTime());
        m_pStartDateEdit->setDate(settings.value("startDate").toDate());
        m_pEndDateEdit->setDate(settings.value("endDate").toDate());

        QString camName = settings.value("camList").toString();
        if (isNetStream(camName))
            m_pCamListBox->setCurrentText(camName);
        else
            m_pCamListBox->setCurrentIndex(m_pCamListBox->findText(settings.value("camList").toString()));

        m_pCamResListBox->setCurrentIndex(m_pCamResListBox->findText(settings.value("camResList").toString()));
        m_pLiveThresholdBox->setValue(settings.value("liveThreshold").toDouble());
        m_pMatchThresholdBox->setValue(settings.value("matchThreshold").toDouble());
        m_pSdkAppIdEdit->setText(settings.value("sdkAppId").toString());
        m_pSdkKeyEdit->setText(settings.value("sdkKey").toString());
        m_pAutoStartBox->setChecked(settings.value("autoStart").toBool());
        m_pCamSwitchBox->setChecked(settings.value("camSwitch").toBool());
    }

    settings.endGroup();
}

QImage MatToQImage(const cv::Mat& mat)
{
    switch (mat.type())
    {
        case CV_8UC4: // 8-bit, 4 channel
            return QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_ARGB32);

        case CV_8UC3: // 8-bit, 3 channel
            return QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_RGB888).rgbSwapped();

        case CV_8UC1: // 8-bit, 1 channel
            return QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Grayscale8);

        default:
            break;
    }

    return QImage();
}

cv::Mat QImage2cvMat(const QImage &image)
{
    cv::Mat mat;

    switch(image.format())
    {
    case QImage::Format_ARGB32:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
        return cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
        break;

    case QImage::Format_RGB888:
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);
        break;

    case QImage::Format_Indexed8:
        mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
        break;

    default:
        break;
    }

    return mat;
}



