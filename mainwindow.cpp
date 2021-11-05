#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <QSerialPortInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    timer = new QTimer(this);

    connect(timer, SIGNAL(timeout()), this, SLOT(read_position()));

    ui->setupUi(this);
    mountconnected = false;
    lx200port = nullptr;
    lx200running = false;
    system = nullptr;
    mountport = nullptr;
}

MainWindow::~MainWindow()
{
    disconnect(timer, SIGNAL(timeout()), this, SLOT(read_position()));
    delete timer;
    delete ui;
}

void MainWindow::connect_port()
{
    try
    {
        Init();
    }
    catch (...)
    {
        return;
    }

    if (read_position())
    {
        ui->connect->setText("Disconnect");
        mountconnected = true;

        timer->start(500);
        ui->lx200listen->setEnabled(true);
    }
}

void MainWindow::disconnect_port()
{
    timer->stop();
    mountconnected = false;
    ui->connect->setText("Connect");
    ui->lx200listen->setEnabled(false);

    if (lx200running)
    {
        stop_lx200_server();
    }
    if (system)
    {
        delete system;
        system = nullptr;
    }
    if (ctl)
    {
        delete ctl;
        ctl = nullptr;
    }
    if (cs)
    {
        delete cs;
        cs = nullptr;
    }
    if (mountport)
    {
        disconnect(mountport, SIGNAL(error(QSerialPort::SerialPortError)),this,SLOT(serialPortError(QSerialPort::SerialPortError)));
        mountport->close();
        delete mountport;
        mountport = nullptr;
    }

}

void MainWindow::on_connect_clicked()
{
    if (!mountconnected)
    {
        connect_port();
    }
    else
    {
        disconnect_port();
    }
}

void MainWindow::on_mountport_returnPressed()
{
    on_connect_clicked();
}

void MainWindow::on_lx200port_returnPressed()
{
    if (useSerial)
    {
        if (mountconnected)
        {
            if (!lx200running)
            {
                start_lx200_server();
            }
            else
            {
                stop_lx200_server();
                start_lx200_server();
            }
        }
    }
}

void MainWindow::on_disableSteppers_clicked()
{
    if (mountconnected)
        system->DisableSteppers();
}

void MainWindow::on_gotoPosition_clicked()
{
    if (mountconnected)
    {
        double dec = fromHMS(ui->gotoDecd->text().toDouble(), ui->gotoDecm->text().toDouble(), ui->gotoDecs->text().toDouble());
        if (ui->gotoHA->isChecked())
        {
            double ha = fromHMS(ui->gotoHAh->text().toDouble(), ui->gotoHAm->text().toDouble(), ui->gotoHAs->text().toDouble());
            system->GotoPosition_HA_Dec(ha, dec);
        }
        else if (ui->gotoRA->isChecked())
        {
            double ra = fromHMS(ui->gotoRAh->text().toDouble(), ui->gotoRAm->text().toDouble(), ui->gotoRAs->text().toDouble());
            system->GotoPosition_RA_Dec(ra, dec);
        }
    }
}

void MainWindow::on_setPosition_clicked()
{
    if (mountconnected)
    {
        double dec = fromHMS(ui->setDecd->text().toDouble(), ui->setDecm->text().toDouble(), ui->setDecs->text().toDouble());
        if (ui->setHA->isChecked())
        {
            double ha = fromHMS(ui->setHAh->text().toDouble(), ui->setHAm->text().toDouble(), ui->setHAs->text().toDouble());
            system->SetPosition_HA_Dec(ha, dec);
        }
        else if (ui->setRA->isChecked())
        {
            double ra = fromHMS(ui->setRAh->text().toDouble(), ui->setRAm->text().toDouble(), ui->setRAs->text().toDouble());
            system->SetPosition_RA_Dec(ra, dec);
        }
    }
}

void MainWindow::on_setRotationSpeed_clicked()
{
    if (mountconnected)
    {
        double haspeed = ui->haRotationSpeed->text().toDouble();
        double decspeed = ui->decRotationSpeed->text().toDouble();
        system->SetSpeed_HA_Dec(haspeed, decspeed);
    }
}

void MainWindow::on_normalizeCS_clicked()
{
    system->NormalizeCoordinates();
    if (system->DecAxisDirection())
    {
        ui->selectCS1->setChecked(false);
        ui->selectCS2->setChecked(true);
    }
    else
    {
        ui->selectCS1->setChecked(true);
        ui->selectCS2->setChecked(false);
    }
}

void MainWindow::on_switchCS_clicked()
{
    system->InvertCoordinates();
    if (system->DecAxisDirection())
    {
        ui->selectCS1->setChecked(false);
        ui->selectCS2->setChecked(true);
    }
    else
    {
        ui->selectCS1->setChecked(true);
        ui->selectCS2->setChecked(false);
    }
}

void MainWindow::on_setCS1_toggled(bool checked)
{
    if (!checked)
        return;
    system->SetDecAxisDirection(false);
}

void MainWindow::on_setCS2_toggled(bool checked)
{
    if (!checked)
        return;
    system->SetDecAxisDirection(true);
}

void MainWindow::on_lx200listen_clicked()
{
    if (!lx200running)
    {
        if (mountconnected)
            start_lx200_server();
    }
    else
    {
        stop_lx200_server();
    }
}

void MainWindow::on_lx200pty_toggled(bool checked)
{
    if (checked)
    {
        ui->lx200port->setReadOnly(true);
        useSerial = false;
    }
}

void MainWindow::on_lx200serial_toggled(bool checked)
{
    if (checked)
    {
        ui->lx200port->setReadOnly(false);
        useSerial = true;
    }
}

bool MainWindow::read_position()
{
    if (!system->ReadPosition())
    {
        disconnect_port();
        return false;
    }

    std::tuple<double, double> hadec = system->CurrentPosition_HA_Dec();
    std::tuple<double, double> radec = system->CurrentPosition_RA_Dec();
    double ra = std::get<0>(radec);
    double ha = std::get<0>(hadec);
    double dec = std::get<1>(radec);

    auto [ha_h, ha_m, ha_s] = toHMS(ha);
    auto [ra_h, ra_m, ra_s] = toHMS(ra);
    auto [dec_d, dec_m, dec_s] = toHMS(dec);

    ui->curHAh->setText(QString::number(ha_h));
    ui->curHAm->setText(QString::number(ha_m));
    ui->curHAs->setText(QString::number(ha_s));

    ui->curRAh->setText(QString::number(ra_h));
    ui->curRAm->setText(QString::number(ra_m));
    ui->curRAs->setText(QString::number(ra_s));

    ui->curDecd->setText(QString::number(dec_d));
    ui->curDecm->setText(QString::number(dec_m));
    ui->curDecs->setText(QString::number(dec_s));
    return true;
}

void MainWindow::serialPortError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
    {
        return;
    }
    qWarning() << "Serial port error" << error;
}

void MainWindow::Init()
{
    mountport = new QSerialPort();
    connect(mountport, SIGNAL(error(QSerialPort::SerialPortError)),this,SLOT(serialPortError(QSerialPort::SerialPortError)));
    mountport->setPortName(ui->mountport->text());
    mountport->setBaudRate(QSerialPort::Baud9600);
    mountport->setParity(QSerialPort::NoParity);
    mountport->setDataBits(QSerialPort::Data8);
    mountport->setStopBits(QSerialPort::OneStop);
    mountport->setFlowControl(QSerialPort::NoFlowControl);
    if (!mountport->open(QIODevice::ReadWrite))
    {
        QMessageBox box;
        box.setText("Can not open port");
        box.exec();
        throw "Can not open port";
    }

    QTimeZone tz = QTimeZone(ui->timezone->text().toLatin1());
    double lon = ui->longitude->text().toDouble();
    cs = new CoordinateSystem(tz, lon);
    ctl = new MountController(mountport, subseconds);
    system = new MountSystem(ctl, cs);
}

void MainWindow::start_lx200_server()
{
    if (lx200port)
        lx200port->close();
    if (ui->lx200serial->isChecked())
    {
        lx200port = new QSerialPort(this);
        lx200port->setPortName(ui->lx200port->text());
        lx200port->setBaudRate(9600);
        lx200port->setParity(QSerialPort::Parity::NoParity);
        lx200port->setDataBits(QSerialPort::DataBits::Data8);
        if (!lx200port->open(QIODevice::ReadWrite))
        {
            delete lx200port;
            QMessageBox msg;
            msg.setText("Can not listen serial port");
            return;
        }
    }
    else if (ui->lx200pty->isChecked())
    {
        lx200port = new QSerialPort(this);
        lx200port->setPortName(ptmx);
        lx200port->setBaudRate(9600);
        lx200port->setParity(QSerialPort::Parity::NoParity);
        lx200port->setDataBits(QSerialPort::DataBits::Data8);
        if (!lx200port->open(QIODevice::ReadWrite))
        {
            delete lx200port;
            QMessageBox msg;
            msg.setText("Can not open " + ptmx);
            return;
        }
        int master_fd = lx200port->handle();
        constexpr size_t PTSNAME_BUFFER_LENGTH = 128;
        char ptsname_buffer[PTSNAME_BUFFER_LENGTH];
        if (ptsname_r(master_fd, ptsname_buffer, PTSNAME_BUFFER_LENGTH) != 0 ||
            grantpt(master_fd) != 0 ||
            unlockpt(master_fd) != 0)
        {
            lx200port->close();
            delete lx200port;
            QMessageBox msg;
            msg.setText("Can not handle " + ptmx);
            return;
        }
        ui->lx200port->setText(QString(ptsname_buffer));
    }
    ui->lx200listen->setText("Stop");
    server = new LX200Server(system, lx200port);
    lx200running = true;
}

void MainWindow::stop_lx200_server()
{
    if (lx200port)
    {
        delete server;
        lx200port->close();
        delete lx200port;
        lx200port = nullptr;
    }
    ui->lx200listen->setText("Listen");
    lx200running = false;
}

std::tuple<int, int, double> MainWindow::toHMS(double x)
{
    int sign = (x >= 0) ? 1 : -1;
    x = abs(x);
    int h = x;
    int m = x * 60 - h * 60;
    int s = x * 3600 - h * 3600 - m * 60;
    return std::make_tuple(h * sign, m, s);
}

double MainWindow::fromHMS(int h, int m, double s)
{
    int sign = (h >= 0) ? 1 : -1;
    return  sign * (abs(h) + m / 60.0 + s / 3600.0);
}
