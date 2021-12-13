#include "mountsystem.h"
#include "mountcontroller.h"

MountSystem::MountSystem(MountController *ctl, CoordinateSystem *cs, Tracker *tracker, Config *cfg)
{
    this->cs = cs;
    this->ctl = ctl;
    this->cfg = cfg;
    this->tracker = tracker;
    this->dec_invert = false;
}

void MountSystem::SetPosition_HA_Dec(double ha, double dec)
{
    this->ha = ha;
    this->dec = dec;
    this->ra = cs->Convert_HA2RA(ha, QDateTime::currentDateTime());
    tracker->Init_Track_HA_Dec(ha, dec);
    Set_HA_Dec(ha, dec);
}

void MountSystem::SetPosition_RA_Dec(double ra, double dec)
{
    this->ra = ra;
    this->dec = dec;
    this->ha = cs->Convert_RA2HA(ra, QDateTime::currentDateTime());
    tracker->Init_Track_RA_Dec(ra, dec);
    Set_HA_Dec(ha, dec);
}

void MountSystem::SetPosition_Az_Alt(double az, double alt)
{
    std::tuple<double, double> hadec = cs->Convert_from_Az_Alt(az, alt);
    this->az = az;
    this->alt = alt;
    this->ha = std::get<0>(hadec);
    this->dec = std::get<1>(hadec);
    this->ra = cs->Convert_HA2RA(ha, QDateTime::currentDateTime());
    tracker->Init_Track_Az_Alt(az, alt);
    Set_HA_Dec(ha, dec);
}

std::tuple<int, int> MountSystem::Convert_To_XY(double ha, double dec)
{
    int x, y;

    if (dec_invert)
    {
        auto res = cs->Inverted_HA_Dec_Coordinates(ha, dec);
        ha = std::get<0>(res);
        dec = std::get<1>(res);
    }

    x = ha/24 * cfg->x_steps;
    y = dec/360 * cfg->y_steps + cfg->y_steps / 2;
    return std::make_tuple(x, y);
}

std::tuple<double, double> MountSystem::Convert_From_XY(int x, int y)
{
    double ha = x * 24.0 / cfg->x_steps;
    double dec = (y - cfg->y_steps / 2) * 360.0 / cfg->y_steps;

    if (dec_invert)
    {
        auto res = cs->Inverted_HA_Dec_Coordinates(ha, dec);
        ha = std::get<0>(res);
        dec = std::get<1>(res);
    }
    return std::make_tuple(ha, dec);
}

bool MountSystem::Set_HA_Dec(double ha, double dec)
{
    auto r = Convert_To_XY(ha, dec);
    int x = std::get<0>(r);
    int y = std::get<1>(r);
    ctl->SetPosition(x, y);
    return true;
}

std::tuple<bool, double, double> MountSystem::InitGoto()
{
    ctl->DisableSteppers();

    auto p = ctl->ReadPosition();
    if (!std::get<0>(p))
        return std::make_tuple(false, 0, 0);

    int current_x = std::get<1>(p);
    int current_y = std::get<2>(p);

    auto r = Convert_From_XY(current_x, current_y);
    return std::make_tuple(true, std::get<0>(r), std::get<1>(r));
}

void MountSystem::GotoPosition_HA_Dec(double ha, double dec)
{
    std::tuple<bool, double, double> hadec = InitGoto();
    if (!std::get<0>(hadec))
        return;
    tracker->Init_Track_HA_Dec(std::get<1>(hadec), std::get<2>(hadec));
    tracker->Set_Target_HA_Dec(ha, dec);
}

void MountSystem::GotoPosition_RA_Dec(double ra, double dec)
{
    std::tuple<bool, double, double> hadec = InitGoto();
    if (!std::get<0>(hadec))
        return;
    double curra = cs->Convert_HA2RA(std::get<1>(hadec), QDateTime::currentDateTime());
    tracker->Init_Track_RA_Dec(curra, std::get<2>(hadec));
    tracker->Set_Target_RA_Dec(ra, dec);
}

void MountSystem::GotoPosition_Az_Alt(double az, double alt)
{
    std::tuple<bool, double, double> hadec = InitGoto();
    if (!std::get<0>(hadec))
        return;
    auto azalt = cs->Convert_to_Az_Alt(std::get<1>(hadec), std::get<2>(hadec));
    tracker->Init_Track_Az_Alt(std::get<0>(azalt), std::get<1>(azalt));
    tracker->Set_Target_Az_Alt(az, alt);
}

void MountSystem::SetDecAxisDirection(bool invert)
{
    this->dec_invert = invert;
}

std::tuple<double, double> MountSystem::CurrentPosition_HA_Dec()
{
    return std::make_tuple(ha, dec);
}

std::tuple<double, double> MountSystem::CurrentPosition_RA_Dec()
{
    return std::make_tuple(ra, dec);
}

std::tuple<double, double> MountSystem::CurrentPosition_Az_Alt()
{
    return std::make_tuple(az, alt);
}

std::tuple<TrackerMode, double, double> MountSystem::CurrentTarget()
{
    double a, b;
    auto mode = tracker->Get_Tracking_Target(&a, &b);
    return std::make_tuple(mode, a, b);
}

bool MountSystem::ReadPosition()
{
    std::tuple<bool, int, int> r = ctl->ReadPosition();
    if (!std::get<0>(r))
        return false;
    auto hadec = Convert_From_XY(std::get<1>(r), std::get<2>(r));
    this->ha = std::get<0>(hadec);
    this->dec = std::get<1>(hadec);
    this->ra = cs->Convert_HA2RA(this->ha, QDateTime::currentDateTime());
    std::tuple<double, double> azalt = cs->Convert_to_Az_Alt(this->ha, this->dec);
    this->az = std::get<0>(azalt);
    this->alt = std::get<1>(azalt);
    return true;
}

bool MountSystem::DecAxisDirection()
{
    return dec_invert;
}

void MountSystem::DisableSteppers()
{
    ctl->DisableSteppers();
}

void MountSystem::NormalizeCoordinates()
{
    if (dec > 90 || dec < -90)
        InvertCoordinates();
}

void MountSystem::InvertCoordinates()
{
    SetDecAxisDirection(!dec_invert);
    tracker->InvertCoordinates();
}

void MountSystem::StartTracking_RA_Dec()
{

}

void MountSystem::StopTracking()
{
    tracker->StopTracking();
}

void MountSystem::Move_HA_Dec(double dha, double ddec, double time)
{
    int dx = dha/24 * cfg->x_steps;
    int dy = ddec/360 * cfg->y_steps;
    if (dec_invert)
        dy = -dy;
    ctl->Goto(dx, dy, time*1e6);
}

void MountSystem::TrackingPeriodic(double dt)
{
    if (!ctl->HasQueueSpace())
        return;

    auto res = tracker->ProcessTrack(dt);
    double dha = std::get<0>(res);
    double ddec = std::get<1>(res);
    double dtime = std::get<2>(res);
    if (dha != 0 || ddec != 0)
    {
        Move_HA_Dec(dha, ddec, dtime);
    }
}
