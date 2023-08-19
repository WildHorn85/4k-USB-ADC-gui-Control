#include "dialog.h"
#include "ui_dialog.h"
#include "adc_control.h"

uint options[5] = {0, };
bool force_dt = 0;
bool force_rmmod = 0;

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    ui->sb_port->setValue(options[0]);
    //ui->sb_dev->setValue(options[1]);
    //ui->sb_chan->setValue(options[2]);
    ui->sb_p_grub->setValue(options[2]);
    ui->sb_p_toch->setValue(options[3]);
    ui->combo_baudrate->setCurrentText(QString::number(options[4]));
    //ui->combo_baudrate->;

    switch(options[1]) {
        case 0:
            ui->sb_dev->setValue(1);
            ui->sb_chan->setValue(1);
            break;
        case 1:
            ui->sb_dev->setValue(2);
            ui->sb_chan->setValue(1);
            break;
        case 2:
            ui->sb_dev->setValue(2);
            ui->sb_chan->setValue(2);
            break;
        default:
            ui->sb_dev->setValue(1);
            ui->sb_chan->setValue(1);
            break;
    }

    qDebug() << "dialog INIT options is:" << options[0] << options[1] << options[2] << options[3] << options[4];
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_buttonBox_accepted()
{
    options[0] = ui->sb_port->value();
    options[2] = ui->sb_p_grub->value();
    options[3] = ui->sb_p_toch->value();
    options[4] = ui->combo_baudrate->currentText().toInt();

    switch(ui->sb_dev->value()) {
    case 1:
            options[1] = 0;
            break;
    case 2:
            switch(ui->sb_chan->value()) {
            case 1:
                options[1] = 1;
                break;
            case 2:
                options[1] = 2;
                break;
            default:
                options[1] = 1;
                break;
            }
            break;
    default:
            options[1] = 0;
            break;
    }
    qDebug() << "dialog ACCEPT options is:" << options[0] << options[1] << options[2] << options[3]<< options[4];
    adc_thld_reset(options[1]);
    adc_thld_set(options[1], options[2], options[3]);
}

void Dialog::on_buttonSave_clicked()
{
    options[0] = ui->sb_port->value();
    options[2] = ui->sb_p_grub->value();
    options[3] = ui->sb_p_toch->value();
    options[4] = ui->combo_baudrate->currentText().toInt();

    switch(ui->sb_dev->value()) {
    case 1:
            options[1] = 0;
            break;
    case 2:
            switch(ui->sb_chan->value()) {
            case 1:
                options[1] = 1;
                break;
            case 2:
                options[1] = 2;
                break;
            default:
                options[1] = 1;
                break;
            }
            break;
    default:
            options[1] = 0;
            break;
    }

    settings_save(options);
    qDebug() << "dialog SAVE options is:" << options[0] << options[1] << options[2] << options[3] << options[4];
    adc_thld_reset(options[1]);
    adc_thld_set(options[1], options[2], options[3]);
}


void Dialog::on_checkBox_stateChanged()
{
    if (ui->checkBox->isChecked()) {
            ui->sb_dev->setEnabled(false);
            force_dt = 0;
    } else {
            ui->sb_dev->setEnabled(true);
            force_dt = 1;
    }
}


void Dialog::on_checkBox_2_stateChanged()
{
    if (ui->checkBox->isChecked()) force_rmmod = 1;
    else force_rmmod = 0;
}

