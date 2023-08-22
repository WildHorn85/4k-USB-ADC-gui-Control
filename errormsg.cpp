#include "errormsg.h"
#include "ui_errormsg.h"

ErrorMsg::ErrorMsg(int errnum, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ErrorMsg)
{
    ui->setupUi(this);

    this->setWindowTitle(QStringLiteral("ERROR %1").arg(errnum));
    //QString errmsg;

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
        sprintf(err_msg,"ERROR %d: WH_DEVICE_MEMORY_ERROR",errnum);
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
    ui->label_err_msg->setText(err_msg);
}

ErrorMsg::~ErrorMsg()
{
    delete ui;
}
