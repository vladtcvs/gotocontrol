#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <QSerialPortInfo>
#include <QDir>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    timer = new QTimer(this);

    connect(timer, SIGNAL(timeout()), this, SLOT(periodic_callback()));

    ui->setupUi(this);
    mountconnected = false;
    lx200port = nullptr;
    lx200running = false;
    system = nullptr;
    mountport = nullptr;
    period_dt = 0.5;
}

MainWindow::~MainWindow()
{
    disconnect(timer, SIGNAL(timeout()), this, SLOT(periodic_callback()));
    delete timer;
    delete ui;
}

void MainWindow::periodic_callback()
{
    read_position();
    system->TrackingPeriodic(period_dt);
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

        timer->start(period_dt*1000);
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

void MainWindow::ShowPosition(bool show_target)
{
    std::tuple<double, double> hadec = system->CurrentPosition_HA_Dec();
    std::tuple<double, double> radec = system->CurrentPosition_RA_Dec();
    std::tuple<double, double> azalt = system->CurrentPosition_Az_Alt();
    double ra = std::get<0>(radec);
    double ha = std::get<0>(hadec);
    double dec = std::get<1>(radec);
    double az = std::get<0>(azalt);
    double alt = std::get<1>(azalt);
    auto ha_hms = toHMS(ha);
    auto ra_hms = toHMS(ra);
    auto dec_dms = toDMS(dec);
    auto az_dms = toDMS(az);
    auto alt_dms = toDMS(alt);

    if (show_target)
    {
        std::tuple<TrackerMode, double, double> target = system->CurrentTarget();
        switch (std::get<0>(target))
        {
        case TrackerHoldHADec:
            ha_hms = ha_hms + " (" + toHMS(std::get<1>(target)) + ")";
            dec_dms = dec_dms + " (" + toDMS(std::get<2>(target)) + ")";
            break;
        case TrackerHoldRADec:
            ra_hms = ra_hms + " (" + toHMS(std::get<1>(target)) + ")";
            dec_dms = dec_dms + " (" + toDMS(std::get<2>(target)) + ")";
            break;
        case TrackerHoldAzAlt:
            az_dms = az_dms + " (" + toDMS(std::get<1>(target)) + ")";
            alt_dms = alt_dms + " (" + toDMS(std::get<2>(target)) + ")";
            break;
        case TrackerHoldNone:
            break;
        }
    }

    ui->posHA->setText(ha_hms);
    ui->posRA->setText(ra_hms);
    ui->posDEC->setText(dec_dms);
    ui->posAZ->setText(az_dms);
    ui->posALT->setText(alt_dms);
}

void MainWindow::on_gotoPosition_clicked(bool checked)
{
    if (checked)
    {
        ShowPosition(false);
        return;
    }
    if (mountconnected)
    {
        if (ui->modeEQ->isChecked())
        {
            double dec = fromDMS(ui->posDEC->text());
            if (ui->modeHA->isChecked())
            {
                double ha = fromHMS(ui->posHA->text());
                system->GotoPosition_HA_Dec(ha, dec);
            }
            else if (ui->modeRA->isChecked())
            {
                double ra = fromHMS(ui->posRA->text());
                system->GotoPosition_RA_Dec(ra, dec);
            }
        }
        else if (ui->modeAZALT->isChecked())
        {
            double az = fromDMS(ui->posAZ->text());
            double alt = fromDMS(ui->posALT->text());
            system->GotoPosition_Az_Alt(az, alt);
        }
    }
}

void MainWindow::on_setPosition_clicked(bool checked)
{
    if (checked)
    {
        ShowPosition(false);
        return;
    }
    if (mountconnected)
    {
        if (ui->modeEQ->isChecked())
        {
            double dec = fromDMS(ui->posDEC->text());
            if (ui->modeHA->isChecked())
            {
                double ha = fromHMS(ui->posHA->text());
                system->SetPosition_HA_Dec(ha, dec);
            }
            else if (ui->modeRA->isChecked())
            {
                double ra = fromHMS(ui->posRA->text());
                system->SetPosition_RA_Dec(ra, dec);
            }
        }
        else if (ui->modeAZALT->isChecked())
        {
            double az = fromDMS(ui->posAZ->text());
            double alt = fromDMS(ui->posALT->text());
            system->SetPosition_Az_Alt(az, alt);
        }
    }
}

void MainWindow::on_rotate_clicked(bool checked)
{
    if (mountconnected)
    {
        if (!checked)
            system->StopTracking();
        else
            system->StartTracking_RA_Dec();
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

void MainWindow::on_selectCS1_toggled(bool checked)
{
    if (!checked)
        return;
    if (system->DecAxisDirection())
    {
        system->InvertCoordinates();
        system->SetDecAxisDirection(false);
    }
}

void MainWindow::on_selectCS2_toggled(bool checked)
{
    if (!checked)
        return;
    if (!system->DecAxisDirection())
    {
        system->InvertCoordinates();
        system->SetDecAxisDirection(true);
    }
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

    if (!ui->setPosition->isChecked() && !ui->gotoPosition->isChecked())
    {
        ShowPosition(true);
    }
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
    double lat = ui->latitude->text().toDouble();
    cs = new CoordinateSystem(tz, lon, lat);
    ctl = new MountController(mountport);
    cfg = new Config();
    tracker = new Tracker(cs, ctl, cfg);
    system = new MountSystem(ctl, cs, tracker, cfg);
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

        QString ptsname(ptsname_buffer);
        /*QString fn = QDir::tempPath() + "/telescope";
        if (QFile::exists(fn))
        {
            QFile::remove(fn);
        }
        QFile::link(ptsname, fn);*/
        ui->lx200port->setText(ptsname);
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

QString MainWindow::toHMS(double x)
{
    QString hms;
    hms.sprintf("%07.4f", x);
    return hms;
}

double MainWindow::fromHMS(QString hms)
{
    return  hms.toDouble();
}

QString MainWindow::toDMS(double x)
{
    QString dms;
    dms.sprintf("%07.4f", x);
    return dms;
}

double MainWindow::fromDMS(QString dms)
{
    return  dms.toDouble();
}
