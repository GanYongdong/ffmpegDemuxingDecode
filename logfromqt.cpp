#include "logfromqt.h"
#include <QMutex>
#include <QFile>
#include <QDate>

logFromQT::logFromQT()
{
//#ifdef QT_NO_DEBUG
    //qDebug() << "release mode" << endl;
//#else
    //qSetMessagePattern( "[%{time yyyyMMdd h:mm:ss.zzz} %{if-debug}DEBUG%{endif}%{if-info}INFO%{endif}%{if-warning}WARNING%{endif}%{if-critical}CRITICAL%{endif}%{if-fatal}FATAL%{endif}] %{file}:%{line} - %{message}" );
//#endif

}

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    }
}


void outputMessage(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    static QMutex mutex;
    mutex.lock();

    QString text;
    switch(type)
    {
    case QtDebugMsg:
        text = QString("Debug:");
        break;

    case QtInfoMsg:
        text = QString("Info:");
        break;

    case QtWarningMsg:
        text = QString("Warning:");
        break;

    case QtCriticalMsg:
        text = QString("Critical:");
        break;

    case QtFatalMsg:
        text = QString("Fatal:");
    }

//#ifdef QT_NO_DEBUG
//    QString context_info = "";
//#else
//    QString context_info = QString("%1:%2").arg(QString(context.file)).arg(context.line);
//#endif
    QString current_date_time = QDateTime::currentDateTime().toString("yyyyMMdd hh:mm:ss.zzz");
    QString current_date = QString("%1").arg(current_date_time);
    QString message = QString("[%1 %2]%4").arg(current_date).arg(text).arg(msg);

    QFile file("log.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream text_stream(&file);
    text_stream << message;
    file.flush();
    file.close();

    mutex.unlock();
}

