#-------------------------------------------------
#
# Project created by QtCreator 2020-05-05T19:58:33
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ffmpeg_demuxing_decode
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
    ffmpeg_demuxing_decode.cpp \
    logfromqt.cpp

HEADERS += \
    ffmpeg_demuxing_decode.h \
    logfromqt.h

FORMS +=

INCLUDEPATH += $$PWD/../Library/include \
                $$PWD/../Library/include/opencv \
                $$PWD/../Library/include/opencv2
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../Library/lib/ -lopencv_world346
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../Library/lib/ -lopencv_world346d
else:unix: LIBS += -L$$PWD/../Library/lib/ -lopencv_world346
INCLUDEPATH += $$PWD/../Library
DEPENDPATH += $$PWD/../Library

unix|win32: LIBS += -L$$PWD/../Library/lib/ -lpthreadVC2
unix|win32: LIBS += -L$$PWD/../Library/lib/ -ldetectNet
unix|win32: LIBS += -L$$PWD/../Library/lib/ -lavcodec -lavformat -lavutil -lavdevice -lavfilter -lpostproc -lswresample -lswscale
