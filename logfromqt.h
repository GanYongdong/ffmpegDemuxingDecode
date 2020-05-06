#ifndef LOGFROMQT_H
#define LOGFROMQT_H

#include <QDebug>

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

void outputMessage(QtMsgType type, const QMessageLogContext &, const QString &msg);

class logFromQT
{
public:
    logFromQT();

    //void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
};

#endif // LOGFROMQT_H
