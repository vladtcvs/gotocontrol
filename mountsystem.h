#ifndef SYSTEM_H
#define SYSTEM_H

#include "coordinatesystem.h"
#include "mountcontroller.h"

class MountSystem
{
private:
    MountController *ctl;
    CoordinateSystem *cs;
    double ha;
    double ra;
    double dec;
    double az;
    double alt;
    bool dec_invert;
public:
    const double siderial_sync_speed = 86400 / 86164.090530833 * 3600;
public:
    MountSystem(MountController *ctl, CoordinateSystem *cs);

    void SetPosition_HA_Dec(double ha, double dec);
    void SetPosition_RA_Dec(double ra, double dec);
    void SetPosition_Az_Alt(double az, double alt);

    void GotoPosition_HA_Dec(double ha, double dec);
    void GotoPosition_RA_Dec(double ra, double dec);
    void GotoPosition_Az_Alt(double az, double alt);

    void SetDecAxisDirection(bool invert);

    std::tuple<double, double> CurrentPosition_HA_Dec();
    std::tuple<double, double> CurrentPosition_RA_Dec();
    std::tuple<double, double> CurrentPosition_Az_Alt();

    void SetSpeed_HA_Dec(double speed_ha, double speed_dec);
    bool ReadPosition();
    bool DecAxisDirection();
    void DisableSteppers();
    void NormalizeCoordinates();
    void InvertCoordinates();
};

#endif // SYSTEM_H
