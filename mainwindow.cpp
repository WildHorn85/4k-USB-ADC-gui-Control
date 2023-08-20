#include "mainwindow.h"
#include "errormsg.h"
#include "ui_mainwindow.h"
#include "dialog.h"

bool flag_connected = 0;
bool flag_started = 0;
extern uint options[5];
extern bool force_dt;
extern bool force_rmmod;
//uint val_ch = 99;
ulong adc_out_buff[4096] = {0, };
uint x_min = 0;
uint x_max = 4095;
ulong max_val = 1000;
ulong summ = 0;

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QDir::setCurrent(qApp->applicationDirPath());

    int fResult;
    if ((fResult = settings_ini(options)) != 0) {
        char err_msg[256];
        switch (fResult) {
        case 501:
            sprintf(err_msg,"ERROR %d: settings.ini is absent. New one created.",fResult);
            break;
        case 502:
            sprintf(err_msg,"ERROR %d: settings.ini is corrupted. Recreating with fixes.",fResult);
            break;
        default:
            sprintf(err_msg,"ERROR %d: unknown settings.ini error",fResult);
            break;
        }
        ui->statusbar->showMessage(err_msg);
    }
    //qDebug() << "main INIT options is:" << options[0] << options[1] << options[2] << options[3] << options[4];

    myWorker = new worker;
    WorkerThread = new QThread;
    myWorker->moveToThread(WorkerThread);

    connect(this, SIGNAL(call_redraw(bool)), this, SLOT(redraw_chart(bool)));
    connect(this, SIGNAL(startWorkSignal()), myWorker, SLOT(StartWork()));
    connect(this, SIGNAL(stopWorkSignal()), myWorker, SLOT(StopWork()));
    connect(myWorker, SIGNAL(timeout()), this, SLOT(thread_timeout()));
    connect(this, SIGNAL(error(int)), this, SLOT(on_error(int)));

    redraw_chart(0);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::on_btn_quit_clicked()
{
    QCoreApplication::quit();
}

void MainWindow::on_btn_connect_clicked()
{
    //qDebug() << "CONNECTED: " << flag_connected;
    if (!flag_connected) {    //IF NO CONNECTION
        ui->statusbar->showMessage("Connecting...");
        int fResult = adc_begin(true, options, force_dt, force_rmmod);
        if (fResult != 0) {     //CONNECTION ERROR
            //qDebug() << "ERROR" << fResult;
            emit error(fResult);
            ui->btn_connect->setText("Connect");
            flag_connected = 0;
        } else {                //CONENCTION OK
            char bar_msg[256];
            int chan_n = 1;
            if (options[1]) chan_n = 2;
            sprintf(bar_msg,"Connected (%d-ch deivce) on port %d (%d-8-N-1)", chan_n, options[0], options[4]);
            ui->statusbar->showMessage(bar_msg);
            ui->btn_connect->setText("Disconnect");
            ui->btn_start->setEnabled(true);
            ui->btn_clean->setEnabled(true);
            ui->btn_save->setEnabled(true);
            ui->timer_set->setEnabled(true);
            flag_connected = 1;
            adc_thld_reset(options[1]);
            adc_thld_set(options[1], options[2], options[3]);
            emit call_redraw(1);
        }
    } else {                  //IF CONNECTION PRESIST
        int fResult = adc_close();
        if (fResult != 0) {     //DISCONNECT ERROR
            //qDebug() << "ERROR" << fResult;
            emit error(fResult);
            ui->btn_connect->setText("Disconnect");
            flag_connected = 1;
        } else {                //DISCONNECT OK
            ui->statusbar->showMessage("Disconnected");
            ui->btn_connect->setText("Connect");
            ui->btn_start->setEnabled(false);
            ui->btn_clean->setEnabled(false);
            ui->btn_save->setEnabled(false);
            ui->timer_set->setEnabled(false);
            flag_connected = 0;
        }
    }
}

void MainWindow::on_btn_start_clicked()
{
    if (!flag_started) {
        int fResult = adc_start(options[1]);
        if (fResult != 0) {
            emit error(fResult);
            ui->btn_start->setText("Start");
            flag_started = 0;
        } else {
            ui->statusbar->showMessage("Started");
            ui->btn_start->setText("Stop");
            flag_started = 1;

            QTime enTime = ui->timer_set->time();
            QTime tm;
            tm.setHMS(0,0,0);
            if (enTime > tm) {
                //qDebug() << "startwork signal emmitted";
                emit startWorkSignal();
                WorkerThread->start();
            } else {
                 //qDebug() << "Timer is empty. Nothing to do.";
                 ui->statusbar->showMessage("Timer is empty");
                 ui->btn_start->setText("Start");
                 flag_started = 0;
            }
        }
    } else {
        int fResult = adc_stop(options[1]);
        if (fResult != 0) {
            //qDebug() << "ERROR" << fResult;
            emit error(fResult);
            ui->btn_start->setText("Stop");
            flag_started = 1;
        } else {
            ui->statusbar->showMessage("Stopped");
            ui->btn_start->setText("Start");
            flag_started = 0;
            //qDebug() << "stopwork signal emmitted";
            emit stopWorkSignal();
            WorkerThread->quit();
            WorkerThread->wait();
        }
    }
}

void MainWindow::on_btn_clean_clicked()
{
    //qDebug() << "CLEAN START";
    int fResult = adc_clear_mem(options[1]);
    //qDebug() << "CLEAN result = " << fResult;
    if (fResult != 0) {
        emit error(fResult);
    } else {
        max_val = 1000;
        ui->statusbar->showMessage("ADC Memory Cleared");
        emit call_redraw(1);
    }
    //qDebug() << "CLEAN FINISH";
}

void MainWindow::on_btn_load_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Spectrum"), "", tr("Spectra Files (*.txt *.dat)"));
    qDebug() << "load file name:" << fileName;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "FILE OPEN ERROR";
        return;
    }

    QTextStream in(&file);

    int pos = 0;
    while (!in.atEnd()) {
        QString line = in.readLine();
        int value = line.split(" ").takeLast().toInt();  //Get last int value from string
        //qDebug() << value;
        adc_out_buff[pos] = value;
        pos++;
    }
    if (ui->check_load->isChecked()) adc_write_mem(adc_out_buff, 4096, options[1]);
    summ = 0;
    emit call_redraw(1);
}

void MainWindow::on_btn_save_clicked()
{
    QFileDialog dialog(this, "Save Spectrum", QString(), "Spectrum Files (*.txt)");
    dialog.setDefaultSuffix(".txt");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if (dialog.exec()) {
        const auto fileName = dialog.selectedFiles().at(0);
        qDebug() << fileName;
        QFile file(fileName);
        if ( file.open( QIODevice::WriteOnly ) ) {
            QTextStream stream( &file );
            for(int i=0;i<4096;i++) {
                char line[30];
                sprintf(line,"%4d %10lu\n", i, adc_out_buff[i]);
                stream << line;
            }
        }
        file.close();
    }
}

void MainWindow::on_pushButto_minus_clicked()
{
    QTime tm;
    tm.setHMS(0,0,0);
    QTime enTime = ui->timer_set->time();
    qDebug() << enTime;
    if (enTime == tm) {
        qDebug() << "reached bottom border";
    } else {
        QTime setTime = enTime.addSecs(-60);
        if (setTime > enTime) ui->timer_set->setTime(tm);
        else ui->timer_set->setTime(setTime);
    }
}

void MainWindow::on_pushButton_plus_clicked()
{
    QTime tm;
    tm.setHMS(23,59,59);
    QTime enTime = ui->timer_set->time();
    qDebug() << enTime;
    if (enTime == tm) {
        qDebug() << "reached upper border";
    } else {
        QTime setTime = enTime.addSecs(60);
        if (setTime < enTime) ui->timer_set->setTime(tm);
        else ui->timer_set->setTime(setTime);
    }
}

void MainWindow::on_btn_options_clicked()
{
    Dialog options_w;
    options_w.setModal(true);
    options_w.exec();
    emit call_redraw(flag_connected);
    //qDebug() << options[0] << options[1] << options[2] << options[3] << options[4];
}

void MainWindow::redraw_chart(bool rflag)
{
    if (rflag) {
        int fResult = adc_read_mem(adc_out_buff, &summ, 4096, options[1]);  // функция чтения памяти АЦП
        if (fResult) {
            emit error(fResult);
            return;
        }
        if (summ > ulong(8500000000000)) {
            qDebug() << "GRAPH OVERLOAD";
            ui->statusbar->showMessage("GRAPH OVERLOAD");
            for (int i = 0; i < 4096; i++) adc_out_buff[i] = 1000;
        }
    } else {
        for (int i = 0; i < 4096; i++) summ += adc_out_buff[i];
    }

    auto *series0 = new QLineSeries();
    ulong range = x_max - x_min + 1;
    QVector<QPointF> points(range);
    for(std::vector<int>::size_type i = 0; i < range; ++i) {
        if (ui->check_log10->isChecked()) {
            if (adc_out_buff[x_min+i] == 0) points[i] = QPointF(x_min+i, 1);    //remove zeros from log10
            else points[i] = QPointF(x_min+i, adc_out_buff[x_min+i]);
        } else {
            points[i] = QPointF(x_min+i, adc_out_buff[x_min+i]);
        }
    }
    series0->replace(points);

    //MAX Value from data
    if (ui->check_auto_y->isChecked()) {
        ulong t_max_val = 0;
        for (uint i = x_min; i < x_max; i++) if(t_max_val < adc_out_buff[i]) t_max_val = adc_out_buff[i];
        if (t_max_val > 0) {
            ui->y_max->setValue(t_max_val);
            max_val = t_max_val;
        } else {
            max_val = 1000;
        }
    } else {
        max_val = ui->y_max->value();
    }

    auto series = new QAreaSeries(series0);
    QPen pen(0x0B5394);
    pen.setWidth(1);
    series->setPen(pen);

    QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
    gradient.setColorAt(0.0, 0x23649e);
    gradient.setColorAt(1.0, 0x23649e);
    gradient.setCoordinateMode(QGradient::LogicalMode);
    series->setBrush(gradient);

    QChart *chart = new QChart();
    chart->legend()->hide();
    chart->addSeries(series);

    auto axisX = new QValueAxis;
    axisX->setTitleText("ADC Channels");
    axisX->setLabelFormat("%i");
    axisX->setTickCount((series0->count()/((x_max-x_min)/16))+1);
    axisX->setRange(x_min, x_max);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    if (ui->check_log10->isChecked()) {
        auto axisY = new QLogValueAxis;
        axisY->setTitleText("Log10 Events");
        axisY->setLabelFormat("%g");
        axisY->setRange(1, qreal(max_val));
        axisY->setBase(10.0);
        axisY->setMinorTickCount(-1);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
    } else {
        auto axisY = new QValueAxis;
        axisY->setTitleText("Int Events");
        axisY->setLabelFormat("%g");
        axisY->setTickCount(11);
        axisY->setRange(0, qreal(max_val));
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
    }

    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setChart(chart);

    ui->label_enum->setText(QString::number(summ));

    int chan_n = 2;
    if (options[1] < 2) chan_n = 1;
    ui->label_chan_val->setText(QString::number(chan_n));
}

void MainWindow::thread_timeout()
{
    QTime tm;
    tm.setHMS(0,0,0);
    QTime enTime = ui->timer_set->time();
    enTime = enTime.addSecs(-1);
    //qDebug() << currsec;
    if (enTime == tm) {
        qDebug() << "Time is out!";
        ui->statusbar->showMessage("Time is out");
        ui->btn_start->setText("Start");
        flag_started = 0;
        adc_stop(options[1]);
        emit stopWorkSignal();
        ui->timer_set->setTime(enTime);
        WorkerThread->exit();
        WorkerThread->wait();
    } else {
        //enTime = enTime.addSecs(-1);
        ui->timer_set->setTime(enTime);
    }
    emit call_redraw(1);
}

void MainWindow::on_x_min_editingFinished()
{
    x_min = ui->x_min->value();
    ui->x_max->setMinimum(x_min+10);
    emit call_redraw(0);
}


void MainWindow::on_x_max_editingFinished()
{
    x_max = ui->x_max->value();
    ui->x_min->setMaximum(x_max-10);
    emit call_redraw(0);
}


void MainWindow::on_check_auto_y_stateChanged()
{
    if (ui->check_auto_y->isChecked()) {
        ui->y_max->setEnabled(false);
    } else {
        ui->y_max->setEnabled(true);
    }
    emit call_redraw(0);
}


void MainWindow::on_y_max_editingFinished()
{
    max_val = ui->y_max->value();
    emit call_redraw(0);
}


void MainWindow::on_check_log10_stateChanged()
{
    emit call_redraw(0);
}

void MainWindow::on_error(int errnum)
{
    qDebug() << "Error" << errnum << "emited";

    char err_msg[256];
    switch (errnum) {
    case 1:
        sprintf(err_msg,"ERROR %d: FT_INVALID_HANDLE",errnum);
        break;
    case 2:
        sprintf(err_msg,"ERROR %d: FT_DEVICE_NOT_FOUND (Connect ADC to USB-port first!)",errnum);
        break;
    case 3:
        sprintf(err_msg,"ERROR %d: FT_DEVICE_NOT_OPENED (Unload (rmmod) ftdi_sio and usbserial drivers first!)",errnum);
        break;
    case 4:
        sprintf(err_msg,"ERROR %d: FT_IO_ERROR",errnum);
        break;
    case 5:
        sprintf(err_msg,"ERROR %d: FT_INSUFFICIENT_RESOURCES",errnum);
        break;
    case 6:
        sprintf(err_msg,"ERROR %d: FT_INVALID_PARAMETER",errnum);
        break;
    case 7:
        sprintf(err_msg,"ERROR %d: FT_INVALID_BAUD_RATE",errnum);
        break;
    case 8:
        sprintf(err_msg,"ERROR %d: FT_DEVICE_NOT_OPENED_FOR_ERASE",errnum);
        break;
    case 9:
        sprintf(err_msg,"ERROR %d: FT_DEVICE_NOT_OPENED_FOR_WRITE",errnum);
        break;
    case 10:
        sprintf(err_msg,"ERROR %d: FT_FAILED_TO_WRITE_DEVICE",errnum);
        break;
    case 11:
        sprintf(err_msg,"ERROR %d: FT_EEPROM_READ_FAILED",errnum);
        break;
    case 12:
        sprintf(err_msg,"ERROR %d: FT_EEPROM_WRITE_FAILED",errnum);
        break;
    case 13:
        sprintf(err_msg,"ERROR %d: FT_EEPROM_ERASE_FAILED",errnum);
        break;
    case 14:
        sprintf(err_msg,"ERROR %d: FT_EEPROM_NOT_PRESENT",errnum);
        break;
    case 15:
        sprintf(err_msg,"ERROR %d: FT_EEPROM_NOT_PROGRAMMED",errnum);
        break;
    case 16:
        sprintf(err_msg,"ERROR %d: FT_INVALID_ARGS",errnum);
        break;
    case 17:
        sprintf(err_msg,"ERROR %d: FT_NOT_SUPPORTED",errnum);
        break;
    case 18:
        sprintf(err_msg,"ERROR %d: FT_OTHER_ERROR",errnum);
        break;
    case 19:
        sprintf(err_msg,"ERROR %d: FT_DEVICE_LIST_NOT_READY",errnum);
        break;
    case 20:
        sprintf(err_msg,"ERROR %d: DEVICE MEMORY ERROR",errnum);
        break;
    case 21:
        sprintf(err_msg,"ERROR %d: WH_DEVICE_READ_TIMEOUT (Can't read memory of device)",errnum);
        break;
    case 404:
        sprintf(err_msg,"ERROR %d: WH_ADC_NOT_FOUND_BY_NAME (ADC device not found. Unplug other devices from PC or select another port!)",errnum);
        break;
    default:
        sprintf(err_msg,"ERROR %d: unknown error",errnum);
        break;
    }
    ui->statusbar->showMessage(err_msg);

    if (flag_started) {
        emit stopWorkSignal();
        WorkerThread->quit();
        WorkerThread->wait();
        ui->btn_start->setText("Start");
        flag_started = 0;
    }

    ErrorMsg error_msg(errnum);
    error_msg.setModal(true);
    error_msg.exec();
}

