#ifndef LX200SERVER_H
#define LX200SERVER_H

#include <QObject>
#include "mountsystem.h"

class LX200Server : public QObject
{
    Q_OBJECT
private:
    MountSystem *system;
    double target_ra;
    double target_dec;
    QSerialPort *port;
    bool run;
    QByteArray buf;
private:
    QString HandleCommand(const QString &cmd);
public:
    LX200Server(MountSystem *system, QSerialPort *port);
    ~LX200Server();
public slots:
    void Process();
};

#endif // LX200SERVER_H
