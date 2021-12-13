#ifndef TRACKER_H
#define TRACKER_H

#include "coordinatesystem.h"
#include "mountcontroller.h"
#include "config.h"

enum TrackerMode
{
    TrackerHoldNone = 0,
    TrackerHoldHADec,
    TrackerHoldRADec,
    TrackerHoldAzAlt,
};

class Tracker
{
private:
    TrackerMode mode;
    CoordinateSystem *cs;
    MountController *ctl;
    Config *cfg;

    double target_ra;
    double target_ha;
    double target_dec;
    double target_az;
    double target_alt;

    double point_ha;
    double point_dec;
    QDateTime finish_time;
public:
    Tracker(CoordinateSystem *cs, MountController *ctl, Config *cfg);
    void Init_Track_RA_Dec(double ra, double dec);
    void Init_Track_HA_Dec(double ha, double dec);
    void Init_Track_Az_Alt(double az, double alt);
    void StopTracking();
    void Set_Target_RA_Dec(double ra, double dec);
    void Set_Target_HA_Dec(double ha, double dec);
    void Set_Target_Az_Alt(double az, double alt);
    std::tuple<double, double, double> ProcessTrack(double delta_t);
    void InvertCoordinates();
private:
    std::tuple<double, double> Track(double target_ha, double target_dec, QDateTime new_finish_time, double delta_t);
    std::tuple<double, double, double> Track_RA_Dec(double delta_t, double new_target_ra, double new_target_dec);
    std::tuple<double, double, double> Track_HA_Dec(double delta_t, double new_target_ha, double new_target_dec);
    std::tuple<double, double, double> Track_Az_Alt(double delta_t, double new_target_az, double new_target_alt);
    QDateTime NextFinishTime(double dt);
};

#endif // TRACKER_H
