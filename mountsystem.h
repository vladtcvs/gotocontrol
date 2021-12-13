#ifndef SYSTEM_H
#define SYSTEM_H

#include "coordinatesystem.h"
#include "mountcontroller.h"
#include "config.h"
#include "tracker.h"

class MountSystem
{
private:
    Config *cfg;
    MountController *ctl;
    CoordinateSystem *cs;
    Tracker *tracker;
    double ha;
    double ra;
    double dec;
    double az;
    double alt;
    bool dec_invert;
    double target_x, target_y;
private:
    std::tuple<bool, double, double> InitGoto();
    bool Set_HA_Dec(double ha, double dec);
    std::tuple<int, int> Convert_To_XY(double ha, double dec);
    std::tuple<double, double> Convert_From_XY(int x, int y);
public:
    const double siderial_sync_speed = 86400 / 86164.090530833 * 3600;
public:
    MountSystem(MountController *ctl, CoordinateSystem *cs, Tracker *tracker, Config *cfg);

    void SetPosition_HA_Dec(double ha, double dec);
    void SetPosition_RA_Dec(double ra, double dec);
    void SetPosition_Az_Alt(double az, double alt);

    void GotoPosition_HA_Dec(double ha, double dec);
    void GotoPosition_RA_Dec(double ra, double dec);
    void GotoPosition_Az_Alt(double az, double alt);

    void Move_HA_Dec(double dha, double ddec, double time);
    //bool AddGotoMovement_HA_Dec(double ha, double dec, double time);

    std::tuple<TrackerMode, double, double> CurrentTarget();

    void SetDecAxisDirection(bool invert);

    std::tuple<double, double> CurrentPosition_HA_Dec();
    std::tuple<double, double> CurrentPosition_RA_Dec();
    std::tuple<double, double> CurrentPosition_Az_Alt();

    void StartTracking_RA_Dec();
    void StopTracking();
    void TrackingPeriodic(double dt);

    bool ReadPosition();
    bool DecAxisDirection();
    void DisableSteppers();
    void NormalizeCoordinates();
    void InvertCoordinates();
};

#endif // SYSTEM_H
