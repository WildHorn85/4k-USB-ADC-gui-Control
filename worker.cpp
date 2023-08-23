#include "worker.h"

qint64 last_sec = 0;
extern uint options[5];
extern ulong adc_out_buff[4096];
extern ulong summ;

worker::worker(QObject *parent) : QObject(parent), running(false)
{
    //qDebug() << "running: " << running;
}


worker::~worker() {}

void worker::do_Work()
{
    emit SignalToObj_mainThreadGUI();
    if (!running) return;

    /* Main loadout */
    qint64 curr_sec = QDateTime::currentMSecsSinceEpoch();
    if (curr_sec >= last_sec + 1000) {
        last_sec = curr_sec;
        int fResult = adc_read_mem(adc_out_buff, &summ, 4096, options[1]);
        if (fResult) emit error_w(fResult);
        emit timeout();
    } else {
        #ifdef  __linux__
            usleep(1000);
        #else
            Sleep(1);
        #endif
    }
    QMetaObject::invokeMethod(this, "do_Work", Qt::QueuedConnection);
}

void worker::StopWork()
{
    running = false;
    do_Work();
}

void worker::StartWork()
{
    running = true;
    do_Work();
}
