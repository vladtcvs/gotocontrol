#ifndef MOUNTCONTROLLER_H
#define MOUNTCONTROLLER_H

#include <QMutex>
#include <QSerialPort>

class MountController
{
private:
    const int queue_size = 2;
private:
    int tid;
    QSerialPort *port;
    QMutex mutex;
private:
    void send(const QString &cmd);
    std::tuple<bool, QString> read();

    int tid_next();
    int tid_delta(int t);
    int free_queue_lines(int t);

    std::tuple<int, int, int>  ParsePosition(const QString &state);
    std::tuple<bool, int, int, int> _ReadPosition();

    QString CmdReadPosition();
    QString CmdDisable();
    QString CmdGoto(int dx, int dy, int period);
    QString CmdSetPos(int x, int y);
public:
    MountController(QSerialPort *port);
    std::tuple<bool, int, int> ReadPosition();
    void DisableSteppers();
    bool Goto(int dx, int dy, int time);
    void SetPosition(int x, int y);
    bool HasQueueSpace();
};

#endif // MOUNTCONTROLLER_H
