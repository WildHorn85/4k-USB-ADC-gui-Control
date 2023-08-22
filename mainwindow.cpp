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
uint tick_values[17] = {1, 2, 4, 5, 10, 20, 30, 40, 50, 100, 150, 200, 250, 300, 400, 500, 1000};

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

    myWorker = new worker;
    WorkerThread = new QThread;
    myWorker->moveToThread(WorkerThread);

    connect(this, SIGNAL(call_redraw(bool)), this, SLOT(redraw_chart(bool)));
    connect(this, SIGNAL(startWorkSignal()), myWorker, SLOT(StartWork()));
    connect(this, SIGNAL(stopWorkSignal()), myWorker, SLOT(StopWork()));
    connect(myWorker, SIGNAL(timeout()), this, SLOT(thread_timeout()));
    connect(this, SIGNAL(error(int)), this, SLOT(on_error(int)));
    connect(myWorker, SIGNAL(error_w(int)), this, SLOT(on_error(int)));

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
    if (!flag_connected) {    //IF NO CONNECTION
        ui->statusbar->showMessage("Connecting...");
        int fResult = adc_begin(true, options, force_dt, force_rmmod);
        if (fResult != 0) {     //CONNECTION ERROR
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
            //ui->btn_clean->setEnabled(true);
            ui->btn_save->setEnabled(true);
            ui->timer_set->setEnabled(true);
            flag_connected = 1;
            if (int fResult = adc_thld_reset(options[1])) emit error(fResult);
            if (int fResult = adc_thld_set(options[1], options[2], options[3])) emit error(fResult);
            if (int fResult = adc_read_mem(adc_out_buff, &summ, 4096, options[1])) emit error(fResult);
            emit call_redraw(1);
        }
    } else {                  //IF CONNECTION PRESIST
        int fResult = adc_close();
        if (fResult != 0) {     //DISCONNECT ERROR
            emit error(fResult);
            ui->btn_connect->setText("Disconnect");
            flag_connected = 1;
        } else {                //DISCONNECT OK
            ui->statusbar->showMessage("Disconnected");
            ui->btn_connect->setText("Connect");
            ui->btn_start->setEnabled(false);
            //ui->btn_clean->setEnabled(false);
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
        } else {
            ui->statusbar->showMessage("Started");
            ui->btn_start->setText("Stop");
            flag_started = 1;

            QTime enTime = ui->timer_set->time();
            QTime tm;
            tm.setHMS(0,0,0);
            if (enTime > tm) {
                emit startWorkSignal();
                WorkerThread->start();
            } else {
                 ui->statusbar->showMessage("Timer is empty!");
                 ui->btn_start->setText("Start");
                 flag_started = 0;
            }
        }
    } else {
        int fResult = adc_stop(options[1]);
        if (fResult != 0) {
            emit error(fResult);
        } else {
            ui->statusbar->showMessage("Stopped");
            ui->btn_start->setText("Start");
            flag_started = 0;
            emit stopWorkSignal();
            WorkerThread->quit();
            WorkerThread->wait();
        }
    }
}

void MainWindow::on_btn_clean_clicked()
{
    if (flag_connected) {
        int fResult = adc_clear_mem(options[1]);
        if (fResult != 0) {
            emit error(fResult);
        } else {
            max_val = 1000;
            ui->statusbar->showMessage("ADC Memory Cleared");
            if (int fResult = adc_read_mem(adc_out_buff, &summ, 4096, options[1])) emit error(fResult);
            emit call_redraw(flag_connected);
        }
    } else {
        for (int i = 0; i < 4096; i++) adc_out_buff[i] = 0;
        summ = 0;
        emit call_redraw(flag_connected);
    }
}

void MainWindow::on_btn_load_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Spectrum"), "", tr("Spectra Files (*.txt *.dat)"));
    //qDebug() << "load file name:" << fileName;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit error(100);
        return;
    }

    QTextStream in(&file);

    int pos = 0;
    while (!in.atEnd()) {
        QString line = in.readLine();
        int value = line.split(" ").takeLast().toInt();  //Get last int value from string
        adc_out_buff[pos] = value;
        pos++;
    }
    if (ui->check_load->isChecked() && flag_connected) {
        if (int fResult = adc_write_mem(adc_out_buff, 4096, options[1])) emit error(fResult);
        if (int fResult = adc_read_mem(adc_out_buff, &summ, 4096, options[1])) emit error(fResult);
    }
    emit call_redraw(flag_connected);
}

void MainWindow::on_btn_save_clicked()
{
    QFileDialog dialog(this, "Save Spectrum", QString(), "Spectrum Files (*.txt)");
    dialog.setDefaultSuffix(".txt");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if (dialog.exec()) {
        const auto fileName = dialog.selectedFiles().at(0);
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
    if (enTime != tm) {
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
    if (enTime != tm) {
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
    if (flag_connected) {
        if (int fResult = adc_thld_reset(options[1])) emit error(fResult);
        if (int fResult = adc_thld_set(options[1], options[2], options[3])) emit error(fResult);
        if (int fResult = adc_read_mem(adc_out_buff, &summ, 4096, options[1])) emit error(fResult);
    }
    emit call_redraw(flag_connected);
    //qDebug() << options[0] << options[1] << options[2] << options[3] << options[4];
}

void MainWindow::redraw_chart(bool rflag)
{
    ulong adc_draw_buff[4096];
    bool flag_overload = 0;

    /*if (rflag) {
        int fResult = adc_read_mem(adc_out_buff, &summ, 4096, options[1]);  // функция чтения памяти АЦП
        if (fResult) {
            emit error(fResult);
            return;
        }
        if (summ > ulong(8500000000000)) {
            qDebug() << "GRAPH OVERLOAD";
            ui->statusbar->showMessage("GRAPH OVERLOAD");
            for (int i = 0; i < 4096; i++) adc_draw_buff[i] = 1000;
        } else {
            for (int i = 0; i < 4096; i++) adc_draw_buff[i] = adc_out_buff[i];
        }
    } else {
        for (int i = 0; i < 4096; i++) summ += adc_out_buff[i];
        if (summ > ulong(8500000000000)) {
            qDebug() << "GRAPH OVERLOAD";
            ui->statusbar->showMessage("GRAPH OVERLOAD");
            for (int i = 0; i < 4096; i++) adc_draw_buff[i] = 1000;
        } else {
            for (int i = 0; i < 4096; i++) adc_draw_buff[i] = adc_out_buff[i];
        }
    }*/

    if (!rflag) for (int i = 0; i < 4096; i++) summ += adc_out_buff[i];
    if (summ > ulong(8500000000000)) {
        //ui->statusbar->showMessage("GRAPH OVERLOAD");
        flag_overload = 1;
        for (int i = 0; i < 4096; i++) adc_draw_buff[i] = 1000;
    } else {
        for (int i = 0; i < 4096; i++) adc_draw_buff[i] = adc_out_buff[i];
    }

    auto *series0 = new QLineSeries();
    ulong range = x_max - x_min + 1;
    QVector<QPointF> points(range);
    for(std::vector<int>::size_type i = 0; i < range; ++i) {
        if (ui->check_log10->isChecked()) {
            if (adc_draw_buff[x_min+i] == 0) points[i] = QPointF(x_min+i, 1);    //remove zeros from log10
            else points[i] = QPointF(x_min+i, adc_draw_buff[x_min+i]);
        } else {
            points[i] = QPointF(x_min+i, adc_draw_buff[x_min+i]);
        }
    }
    series0->replace(points);

    //MAX Value from data
    if (ui->check_auto_y->isChecked()) {
        ulong t_max_val = 0;
        for (uint i = x_min; i < x_max; i++) if(t_max_val < adc_draw_buff[i]) t_max_val = adc_draw_buff[i];
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
    QPen pen(0x0B5394); //blue
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

    QFont title_font;
    title_font.setBold(1);
    title_font.setPixelSize(11);
    title_font.setPointSize(11);

    QFont label_font;
    label_font.setBold(0);
    label_font.setPixelSize(10);
    label_font.setPointSize(10);

    auto axisX = new QValueAxis;
    axisX->setTitleText("ADC Channels");
    axisX->setLabelFormat("%i");
    axisX->setTitleFont(title_font);
    axisX->setLabelsFont(label_font);
    axisX->setRange(x_min, x_max);
    /*axisX->setTickType(QValueAxis::TicksFixed);
    //axisX->setTickCount((series0->count()/((x_max-x_min)/16))+1); //Used with static X ticks (default)*/

    axisX->setTickType(QValueAxis::TicksDynamic);       //Set dynamic X ticks
    axisX->setTickAnchor(0);                            //first X tick position
    uint tick_near = (range - 1) / 14;
    if (tick_near == 0) tick_near = 1;
    uint size = sizeof(tick_values);
    for (uint i = 0; i < size; i++) {
        if (tick_near < tick_values[i]) {
            tick_near = tick_values[i-1];
            break;
        }
    }
    axisX->setTickInterval(tick_near);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    if (ui->check_log10->isChecked()) {
        auto axisY = new QLogValueAxis;
        axisY->setTitleText("Log10 Events");
        axisY->setLabelFormat("%g");
        axisY->setTitleFont(title_font);
        axisX->setLabelsFont(label_font);
        axisY->setRange(1, qreal(max_val));
        axisY->setBase(10.0);
        axisY->setMinorTickCount(-1);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
    } else {
        auto axisY = new QValueAxis;
        axisY->setTitleText("Int Events");
        axisY->setLabelFormat("%g");
        axisY->setTitleFont(title_font);
        axisX->setLabelsFont(label_font);
        axisY->setTickCount(11);
        axisY->setRange(0, qreal(max_val));
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
    }

    //Marker line
    if (ui->cb_mark->isChecked()) {
        auto mark_val = ui->sb_mark->value();
        auto *marker0 = new QLineSeries();
        *marker0 << QPointF(mark_val, 0) << QPointF(mark_val, 1);
        QPen pen_m(0xF50202);   //red
        pen_m.setWidth(1);
        marker0->setPen(pen_m);
        chart->addSeries(marker0);
        marker0->attachAxis(axisX);
    }


    chart->setMargins(QMargins(0,0,5,0));   //left, top, right, and bottom
    //chart->setBackgroundVisible(false);

    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setChart(chart);

    if (flag_overload) {
        QFont title_overload;
        title_overload.setBold(1);
        title_overload.setPixelSize(14);
        title_overload.setPointSize(14);

        QGraphicsTextItem *GO_text = ui->graphicsView->scene()->addText("WARNING!\nGRAPH\nOVERLOADED");
        GO_text->setDefaultTextColor(0xFFFFFF);
        GO_text->setFont(title_overload);
        GO_text->setPos(80,30);
    }

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
    if (enTime == tm) {
        ui->timer_set->setTime(enTime);
        ui->statusbar->showMessage("Time is out");
        ui->btn_start->setText("Start");
        flag_started = 0;
        if (int fResult = adc_stop(options[1])) emit error(fResult);
        else emit stopWorkSignal();
        WorkerThread->exit();
        WorkerThread->wait();
    } else {
        ui->timer_set->setTime(enTime);
    }
    emit call_redraw(flag_connected);
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

void MainWindow::on_sb_mark_editingFinished()
{
    emit call_redraw(0);
}


void MainWindow::on_check_log10_stateChanged()
{
    emit call_redraw(0);
}

void MainWindow::on_cb_mark_stateChanged()
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
        sprintf(err_msg,"ERROR %d: FT_IO_ERROR (The connection to the device has been lost. Check USB cable!.)",errnum);
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
    case 100:
        sprintf(err_msg,"ERROR %d: WH_FILE_OPEN_ERROR (Can not open file!)",errnum);
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
    if (flag_connected) {
        ui->btn_connect->setText("Connect");
        ui->btn_start->setEnabled(false);
        ui->btn_save->setEnabled(false);
        ui->timer_set->setEnabled(false);
        flag_connected = 0;
    }

    ErrorMsg error_msg(errnum);
    error_msg.setModal(true);
    error_msg.exec();
}

void MainWindow::on_btn_timer_reset_clicked()
{
    QTime tm;
    tm.setHMS(0,10,0);
    ui->timer_set->setTime(tm);
}

