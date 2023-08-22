#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QLineSeries>
#include <QValueAxis>
#include <QChart>
#include <QChartView>
#include <QFileDialog>
#include <QDebug>
#include <QAreaSeries>
#include <QLogValueAxis>

#include "adc_control.h"
#include "worker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

signals:
    void call_redraw(bool);
    void startWorkSignal();
    void stopWorkSignal();
    void error(int);

private slots:
  void on_btn_quit_clicked();
  void on_btn_connect_clicked();
  void on_btn_start_clicked();
  void on_btn_clean_clicked();
  void on_btn_load_clicked();
  void on_pushButto_minus_clicked();
  void on_pushButton_plus_clicked();
  void on_btn_save_clicked();
  void on_btn_options_clicked();
  void redraw_chart(bool rflag);
  void thread_timeout();
  void on_x_min_editingFinished();
  void on_x_max_editingFinished();
  void on_check_auto_y_stateChanged();
  void on_y_max_editingFinished();
  void on_check_log10_stateChanged();
  void on_error(int errnum);
  void on_sb_mark_editingFinished();
  void on_cb_mark_stateChanged();

  void on_btn_timer_reset_clicked();

  private:
  Ui::MainWindow *ui;
  worker *myWorker;
  QThread *WorkerThread;
};

#endif // MAINWINDOW_H
