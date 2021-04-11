#include "mainwindow.h"

#include <QApplication>
#include <QTextCodec>
#include <QSharedMemory>
#include <QMessageBox>

#include <commonhelper/commonhelper.h>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName(QString("TianSong"));
    QCoreApplication::setApplicationName(QString("FaceAttendance"));

    QApplication a(argc, argv);

    QSharedMemory sharedMemory(QCoreApplication::organizationName() + QCoreApplication::applicationName());
    if (!sharedMemory.create(512, QSharedMemory::ReadWrite))
        exit(0);
    CommonHelper::setStyleSheet(QStringLiteral(":/style/default.qss"));

    MainWindow w;

    w.show();

    return a.exec();
}
