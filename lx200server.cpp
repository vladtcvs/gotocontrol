#include "lx200server.h"

LX200Server::LX200Server(MountSystem *system, QSerialPort *port)
{
    this->system = system;
    this->port = port;
    connect(port, SIGNAL(readyRead()), this, SLOT(Process()));
}

LX200Server::~LX200Server()
{
    disconnect(port, SIGNAL(readyRead()), this, SLOT(Process()));
}

void LX200Server::Process()
{
    buf += port->readAll();
    int index;
    while ((index = buf.indexOf('#')) != -1)
    {
        QString line = buf.left(index+1);
        buf = buf.right(buf.length() - (index + 1));
        QByteArray res = HandleCommand(line).toLatin1();
        if (res.length() > 0)
        {
            port->write(res);
            port->flush();
        }
    }
}

QString LX200Server::HandleCommand(const QString &cmd)
{
    if (cmd == ":GR#")
    {
        std::tuple<double, double> radec = system->CurrentPosition_RA_Dec();
        double ra = std::get<0>(radec);
        int ra_h = ra;
        int ra_m = ra*60 - ra_h*60;
        int ra_s = ra*3600 - ra_h*3600 - ra_m*60;
        return QString().sprintf("%02i:%02i:%02i#", ra_h, ra_m, ra_s);
    }
    else if (cmd == ":GD#")
    {
        std::tuple<double, double> radec = system->CurrentPosition_RA_Dec();
        double dec = std::get<1>(radec);
        int dec_h = dec;
        int dec_m = abs(dec)*60 - abs(dec_h)*60;
        int dec_s = abs(dec)*3600 - abs(dec_h)*3600 - abs(dec_m)*60;
        return QString().sprintf("%+03i*%02i:%02i#", dec_h, dec_m, dec_s);
    }
    else if (cmd == ":Q#")
    {
        system->SetSpeed_HA_Dec(system->siderial_sync_speed, 0);
        return "";
    }
    else if (cmd.left(3) == ":Sr")
    {
        QString cmdr = cmd;
        cmdr = cmdr.replace("#", "");
        cmdr = cmdr.replace(":Sr", "");
        QStringList ras = cmdr.split(":");
        double ra = ras[0].toInt() + ras[1].toInt() / 60.0 + ras[2].toInt() / 3600.0;
        target_ra = ra;
        return "1#";
    }
    else if (cmd.left(3) == ":Sd")
    {
        QString cmdr = cmd;
        cmdr = cmdr.replace("#", "");
        cmdr = cmdr.replace(":Sd", "");
        cmdr = cmdr.replace("*", ":");
        QStringList decs = cmdr.split(":");
        double dec = abs(decs[0].toInt()) + decs[1].toInt() / 60.0 + decs[2].toInt() / 3600.0;
        target_dec = dec * (decs[0].toInt() > 0 ? 1 : -1);
        return "1#";
    }
    else if (cmd.left(2) == ":S")
    {
        QString cmdr = cmd;
        cmdr = cmdr.replace("#", "");
        cmdr = cmdr.replace(":S", "");
        double speed = cmd.toDouble();
        system->SetSpeed_HA_Dec(speed * 3600 / 60.0, 0);
        return "1#";
    }
    else if (cmd == ":MS#")
    {
        system->GotoPosition_RA_Dec(target_ra, target_dec);
        return "0#";
    }
    else if (cmd == ":CM#")
    {
        return "#";
    }
    return "";
}
