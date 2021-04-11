QT       += core gui concurrent texttospeech multimedia sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    arcface/arcfaceengine.cpp \
    facedetect.cpp \
    facefeature.cpp \
    iconhelper/iconhelper.cpp \
    logindialog/QLoginDialog.cpp \
    main.cpp \
    mainwindowctrl.cpp \
    mainwindowui.cpp \
    videocapture.cpp \
    waitdialog.cpp

HEADERS += \
    arcface/arcfaceengine.h \
    commonhelper/commonhelper.h \
    facedetect.h \
    facefeature.h \
    iconhelper/iconhelper.h \
    logindialog/QLoginDialog.h \
    mainwindow.h \
    videocapture.h \
    waitdialog.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icon.qrc \
    image.qrc \
    style.qrc

DISTFILES +=

LIBS += $$PWD\opencv\lib\libopencv_*.a
INCLUDEPATH += $$PWD\opencv\install\include

LIBS += -L$$PWD/arcface/lib/X64/ -llibarcsoft_face_engine
INCLUDEPATH += $$PWD/arcface/inc

RC_FILE = app.rc



