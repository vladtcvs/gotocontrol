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

void MountSystem::SetPosition_Az_Alt(double az, double alt)
{
    std::tuple<double, double> hadec = cs->Convert_from_Az_Alt(az, alt);
    this->az = az;
    this->alt = alt;
    this->ha = std::get<0>(hadec);
    this->dec = std::get<1>(hadec);
    this->ra = cs->Convert_HA2RA(ha, QDateTime::currentDateTime());
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

void MountSystem::GotoPosition_Az_Alt(double az, double alt)
{
    std::tuple<double, double> hadec = cs->Convert_from_Az_Alt(az, alt);
    double ha = std::get<0>(hadec);
    double dec = std::get<1>(hadec);
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

std::tuple<double, double> MountSystem::CurrentPosition_Az_Alt()
{
    return std::make_tuple(az, alt);
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
    std::tuple<double, double> inverted_crd = cs->Inverted_HA_Dec_Coordinates(ha, dec);
    ha = std::get<0>(inverted_crd);
    dec = std::get<1>(inverted_crd);
    SetPosition_HA_Dec(ha, dec);
    SetDecAxisDirection(!dec_invert);
}
