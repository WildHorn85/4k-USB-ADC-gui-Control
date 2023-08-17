#include "worker.h"

qint64 last_sec = 0;

worker::worker(QObject *parent) : QObject(parent), running(false)
{
    qDebug() << "running: " << running;
}


worker::~worker() {}

void worker::do_Work()
{
    //qDebug() << "inside do_Work";
    emit SignalToObj_mainThreadGUI();

    if (!running) {
        qDebug() << "running: " << running;
        fflush(NULL);
        usleep(100);
        return;
    }

    /* Main loadout */
    qint64 curr_sec = QDateTime::currentMSecsSinceEpoch();
    if (curr_sec >= last_sec + 1000) {
        emit timeout();
        last_sec = curr_sec;
    } else {
        usleep(100);
    }

    QMetaObject::invokeMethod(this, "do_Work", Qt::QueuedConnection);
}

void worker::StopWork()
{
    //qDebug() << "inside StopWork";
    running = false;
    do_Work();
}

void worker::StartWork()
{
    //qDebug() << "inside StartWork";
    running = true;
    do_Work();
}
