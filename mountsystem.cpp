#include "mountsystem.h"
#include "mountcontroller.h"

MountSystem::MountSystem(MountController *ctl, CoordinateSystem *cs)
{
    this->cs = cs;
    this->ctl = ctl;

    this->dec_invert = false;
}

void MountSystem::SetPosition_HA_Dec(double ha, double dec)
{
    this->ha = ha;
    this->dec = dec;
    this->ra = cs->Convert_HA2RA(ha, QDateTime::currentDateTime());
    ctl->SetHADec(ha, dec);
}

void MountSystem::SetPosition_RA_Dec(double ra, double dec)
{
    this->ra = ra;
    this->dec = dec;
    this->ha = cs->Convert_RA2HA(ra, QDateTime::currentDateTime());
    ctl->SetHADec(ha, dec);
}

void MountSystem::GotoPosition_HA_Dec(double ha, double dec)
{
    ctl->GotoHADec(ha, dec);
}

void MountSystem::GotoPosition_RA_Dec(double ra, double dec)
{
    double ha = cs->Convert_RA2HA(ra, QDateTime::currentDateTime());
    ctl->GotoHADec(ha, dec);
}

void MountSystem::SetDecAxisDirection(bool invert)
{
    this->dec_invert = invert;
    ctl->SetDecAxisDirection(dec_invert);
}

std::tuple<double, double> MountSystem::CurrentPosition_HA_Dec()
{
    return std::make_tuple(ha, dec);
}

std::tuple<double, double> MountSystem::CurrentPosition_RA_Dec()
{
    return std::make_tuple(ra, dec);
}

void MountSystem::SetSpeed_HA_Dec(double speed_ha, double speed_dec)
{
    ctl->SetSpeed(speed_ha, speed_dec);
}

bool MountSystem::ReadPosition()
{
    std::tuple<bool, double, double> hadec = ctl->ReadPositionHA();
    if (!std::get<0>(hadec))
        return false;
    this->ha = std::get<1>(hadec);
    this->dec = std::get<2>(hadec);
    this->ra = cs->Convert_HA2RA(this->ha, QDateTime::currentDateTime());
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
    bool inv = false;
    if (dec > 90)
    {
        dec = 180 - dec;
        inv = true;
    }
    else if (dec < -90)
    {
        dec = -180 - dec;
        inv = true;
    }

    SetPosition_RA_Dec(ra, dec);
    if (inv)
        SetDecAxisDirection(!dec_invert);
}

void MountSystem::InvertCoordinates()
{
    NormalizeCoordinates();
    if (dec > 0)
        dec = 180 - dec;
    else
        dec = -180 - dec;
    SetDecAxisDirection(!dec_invert);
    SetPosition_RA_Dec(ra, dec);
}
