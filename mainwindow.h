#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QButtonGroup>
#include <QMainWindow>
#include "mountsystem.h"
#include "lx200server.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void on_connect_clicked();
    void on_disableSteppers_clicked();
    void on_gotoPosition_clicked(bool checked);
    void on_setPosition_clicked(bool checked);
    void on_setRotationSpeed_clicked();
    void on_lx200listen_clicked();
    void on_lx200pty_toggled(bool checked);
    void on_lx200serial_toggled(bool checked);

    void on_normalizeCS_clicked();
    void on_switchCS_clicked();
    void on_setCS1_toggled(bool checked);
    void on_setCS2_toggled(bool checked);

    void on_mountport_returnPressed();
    void on_lx200port_returnPressed();

    bool read_position();
    void serialPortError(QSerialPort::SerialPortError error);
private:
    QTimer *timer;
    Ui::MainWindow *ui;
    MountSystem *system;
    LX200Server *server;
    MountController *ctl;
    CoordinateSystem *cs;
    QSerialPort *mountport;
    QSerialPort *lx200port;
    bool mountconnected;
    bool lx200running;
    bool useSerial;
private:
    const int subseconds = 2;
    const int baudrate = 9600;
    const QString ptmx = "/dev/ptmx";
private:
    void connect_port();
    void disconnect_port();
    void Init();
    void start_lx200_server();
    void stop_lx200_server();
    QString toHMS(double x);
    double fromHMS(QString hms);
    QString toDMS(double x);
    double fromDMS(QString hms);
};
#endif // MAINWINDOW_H
