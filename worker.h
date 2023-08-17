#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QDebug>
#include <QTime>
#include <unistd.h>              //sleep
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

public slots:
    void StopWork();
    void StartWork();

private slots:
    void do_Work();

private:
    volatile bool running;
};

#endif // WORKER_H
