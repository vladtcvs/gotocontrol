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

std::tuple<bool, QString> MountController::read()
{
    QByteArray buffer;
    while (true)
    {
        if (port->bytesAvailable() == 0)
            port->waitForReadyRead(3000);
        QByteArray data = port->readLine();
        buffer.append(data);
        if (buffer.length() == 0)
            return std::make_tuple(false, "");
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
        return std::make_tuple(true, line);
    }
}

std::tuple<int, int, int> MountController::ParsePosition(const QString &state)
{
    QStringList items = state.split(" ");
    int tid = items[0].toInt(nullptr, 8);
    int x = items[1].toInt(nullptr, 8);
    int y = items[2].toInt(nullptr, 8);
    return std::make_tuple(tid, x, y);
}

QString MountController::CmdReadPosition()
{
    return "P";
}

QString MountController::CmdDisable()
{
    return "D";
}

static QString toOctal(int x)
{
    QString s;
    if (x < 0)
    {
        s = "-";
        x = -x;
    }
    return s + QString::number(x, 8);
}

QString MountController::CmdGoto(int dx, int dy, int time)
{
    int period;
    if (dx != 0 || dy != 0)
    {
        int steps;
        if (abs(dx) > abs(dy))
            steps = abs(dx);
        else
            steps = abs(dy);
        period = time / steps;
    }
    else
    {
        period = 100;
    }

    return "G " + toOctal(tid_next()) + " " + toOctal(dx) + " " + toOctal(dy) + " " + toOctal(period);
}

QString MountController::CmdSetPos(int x, int y)
{
    return "S " + toOctal(x) + " " + toOctal(y);
}

int MountController::tid_next()
{
    if (tid < 128)
        tid = tid+1;
    else
        tid = 1;
    return tid;
}

int MountController::tid_delta(int t)
{
    t -= 1;
    int tt = tid - 1;
    if (tt < t)
        tt += 128;
    return tt - t;
}

int MountController::free_queue_lines(int t)
{
    if (t == 0)
        return queue_size;
    int delta = tid_delta(t);
    return queue_size - delta - 1;
}

MountController::MountController(QSerialPort *port)
{
    this->tid = 1;
    this->port = port;
}

std::tuple<bool, int, int, int> MountController::_ReadPosition()
{
    mutex.lock();
    send(CmdReadPosition());
    std::tuple<bool, QString> ans = read();
    mutex.unlock();
    if (std::get<0>(ans) == false)
        return std::make_tuple(false, 0, 0, 0);
    std::tuple<int, int, int> state = ParsePosition(std::get<1>(ans));
    int t = std::get<0>(state);
    int x = std::get<1>(state);
    int y = std::get<2>(state);
    return std::make_tuple(true, t, x, y);
}

std::tuple<bool, int, int> MountController::ReadPosition()
{
    auto res = _ReadPosition();
    return std::make_tuple(std::get<0>(res), std::get<2>(res), std::get<3>(res));
}

void MountController::DisableSteppers()
{
    mutex.lock();
    send(CmdDisable());
    read();
    mutex.unlock();
}

bool MountController::Goto(int dx, int dy, int time)
{
    if (!HasQueueSpace())
        return false;
    mutex.lock();
    send(CmdGoto(dx, dy, time));
    read();
    mutex.unlock();
    return true;
}

void MountController::SetPosition(int x, int y)
{
    mutex.lock();
    send(CmdSetPos(x, y));
    read();
    mutex.unlock();
}

bool MountController::HasQueueSpace()
{
    auto res = _ReadPosition();
    if (std::get<0>(res) == false)
        return false;

    return free_queue_lines(std::get<1>(res));
}
