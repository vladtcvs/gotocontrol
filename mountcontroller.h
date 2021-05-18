#ifndef MOUNTCONTROLLER_H
#define MOUNTCONTROLLER_H

#include <QMutex>
#include <QSerialPort>

class MountController
{
private:
    enum HMode {
        H_Mode,
        G_Mode,
    };
private:
    int subseconds;
    QSerialPort *port;
    QMutex mutex;
private:
    void send(const QString &cmd);
    std::tuple<bool, QString> read();

    std::tuple<double, double, HMode>  ParseState_HA_Dec(const QString &state);
    QString CmdReadPosition();
    QString CmdDisable();
    QString CmdSetSpeed(double haspeed, double decspeed);
    QString CmdGotoHADec(double ha, double dec);
    QString CmdSetHADec(double ha, double dec);
public:
    MountController(QSerialPort *port, int subseconds);
    std::tuple<bool, double, double> ReadPositionHA();
    void DisableSteppers();
    void SetSpeed(double haspeed, double decspeed);
    void GotoHADec(double ha, double dec);
    void SetHADec(double ha, double dec);
};

#endif // MOUNTCONTROLLER_H
