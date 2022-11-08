QT       += core gui multimedia multimediawidgets charts concurrent multimedia-private

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

FFMPEG_ROOT = $$PWD/ffmpeg-n4.4-latest-win64-gpl-shared-4.4

#DEFINES += MAKE_AUDIO=1
DEFINES += MAKE_VIDEO=1

INCLUDEPATH += include/
LIBS += -L$$PWD/libs -lportaudio

INCLUDEPATH += $$FFMPEG_ROOT/include
LIBS += -L$$FFMPEG_ROOT/lib -lavutil -lavformat -lavcodec -lavdevice -lavfilter -lswresample -lswscale -lpostproc

SOURCES += \
    DataSource.cpp \
    main.cpp \
    mp4maker.cpp \
    mycameracapture.cpp \
    recorder.cpp \
    widget.cpp

HEADERS += \
    AudioQueue.h \
    DataSource.h \
    common.h \
    mp4maker.h \
    mycameracapture.h \
    recorder.h \
    widget.h

FORMS += \
    widget.ui

TRANSLATIONS += \
    camera_captor_zh_CN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
