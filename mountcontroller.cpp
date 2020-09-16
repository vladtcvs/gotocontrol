#include "mountcontroller.h"
#include <QDebug>
#include <QThread>

void MountController::send(const QString &cmd)
{
    QByteArray array = (cmd + "\r\n").toLatin1();
    qDebug() << "Sending to port" << cmd;
    port->write(array);
    port->flush();
}

QString MountController::read()
{
    QByteArray buffer;
    while (true)
    {
        if (port->bytesAvailable() == 0)
            port->waitForReadyRead(3000);
        QByteArray data = port->readLine();
        buffer.append(data);
        if (buffer.length() == 0)
            continue;
        char last = buffer[buffer.length()-1];
        if (last != '\n' && last != '\r')
            continue;
        QString line = QString::fromLatin1(buffer);
        buffer.clear();
        line.replace("\r","");
        line.replace("\n","");
        qDebug() << "Received" << line;
        if (line.length() == 0 || line[0] == ':')
        {
            continue;
        }
        return line;
    }
}

std::tuple<double, double, MountController::HMode> MountController::ParseState_HA_Dec(const QString &state)
{
    QStringList items = state.split(" ");
    double HA = items[0].toDouble() / 3600;
    double Dec = items[1].toDouble() / 3600;
    HMode mode = items[2] == "H" ? HMode::H_Mode : HMode::G_Mode;
    return std::make_tuple(HA, Dec, mode);
}

QString MountController::CmdReadPosition()
{
    return "P";
}

QString MountController::CmdDisable()
{
    return "D";
}

QString MountController::CmdSetSpeed(double haspeed, double decspeed)
{
    int ssha = haspeed * subseconds;
    int ssdec = decspeed * subseconds;
    return "H " + QString::number(ssha) + " " + QString::number(ssdec);
}

QString MountController::CmdGotoHADec(double ha, double dec)
{
    return "G " + QString::number((int)(ha*3600)) + " " + QString::number((int)(dec*3600));
}

QString MountController::CmdSetHADec(double ha, double dec)
{
    return "S " + QString::number((int)(ha*3600)) + " " + QString::number((int)(dec*3600));
}

MountController::MountController(QSerialPort *port, int subseconds)
{
    this->port = port;
    this->subseconds = subseconds;
}

std::tuple<double, double> MountController::ReadPositionHA()
{
    mutex.lock();
    send(CmdReadPosition());
    QString ans = read();
    mutex.unlock();
    std::tuple<double, double, HMode> state = ParseState_HA_Dec(ans);
    double ha = std::get<0>(state);
    double dec = std::get<1>(state);
    return std::make_tuple(ha, dec);
}

void MountController::DisableSteppers()
{
    mutex.lock();
    send(CmdDisable());
    read();
    mutex.unlock();
}

void MountController::SetSpeed(double haspeed, double decspeed)
{
    mutex.lock();
    send(CmdSetSpeed(haspeed, decspeed));
    read();
    mutex.unlock();
}

void MountController::GotoHADec(double ha, double dec)
{
    mutex.lock();
    send(CmdGotoHADec(ha, dec));
    read();
    mutex.unlock();
}

void MountController::SetHADec(double ha, double dec)
{
    mutex.lock();
    send(CmdSetHADec(ha, dec));
    read();
    mutex.unlock();
}
