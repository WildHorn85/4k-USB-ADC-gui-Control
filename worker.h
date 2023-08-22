#ifndef WORKER_H
#define WORKER_H

#ifdef __linux__
    #include <unistd.h>              //sleep
#else
    #include <windows.h>
#endif

#include <QObject>
#include <QDebug>
#include <QTime>

#include <cstring>
#include "adc_control.h"

class worker : public QObject {
    Q_OBJECT
public:
    explicit worker(QObject *parent = 0);
    ~worker();

signals:
    void SignalToObj_mainThreadGUI();
    void timeout();
    void error_w(int);

public slots:
    void StopWork();
    void StartWork();

private slots:
    void do_Work();

private:
    volatile bool running;
};

#endif // WORKER_H
