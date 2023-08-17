#ifndef ERRORMSG_H
#define ERRORMSG_H

#include <QDialog>

namespace Ui {
class ErrorMsg;
}

class ErrorMsg : public QDialog
{
    Q_OBJECT

public:
    explicit ErrorMsg(int errnum, QWidget *parent = nullptr);
    ~ErrorMsg();

private:
    Ui::ErrorMsg *ui;
};

#endif // ERRORMSG_H
