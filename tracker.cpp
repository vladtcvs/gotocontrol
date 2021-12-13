#include "tracker.h"

/**
 * Как оно работает:
 *
 * В режиме слежения за объектом по RA/Dec
 *
 * target_ra, target_dec - целевые координаты
 *
 * finish_time - момент окончания всех отправленных отрезков движения
 * point_ha, point_dec - конечные координаты всех отправленных отрезков движения на момент finish_time
 *
 * При добавлении нового отрезка:
 *
 * 1) delta_t - длительность добавляемого отрезка
 *
 * 2) target_ha, target_dec - координаты (target_ra, target_dec) на момент finish_time + delta_t
 *
 * 3) delta_ha, delta_dec - на сколько надо сдвинуться от point_ha, point_dec, чтобы прийти в target_ha, target_dec
 *
 * 4) p_delta_ha, p_delta_dec - на сколько мы можем сдвинуться в направлении
 *                           delta_ha, delta_dec за время delta_t с ограничением максимальной скорости
 *
 * После этого
 * 1) finish_time := finish_time + delta_t
 *
 * 2) point_ha    := point_ha + p_delta_ha
 *    point_dec   := point_dec + p_delta_dec
 *
 * 3) Выполняем Goto по p_delta_ha, p_delta_dec
 */

Tracker::Tracker(CoordinateSystem *cs, MountController *ctl, Config *cfg)
{
    mode = TrackerHoldNone;
    this->cs = cs;
    this->ctl = ctl;
    this->cfg = cfg;
    finish_time = QDateTime::currentDateTime();
}

void Tracker::Init_Track_RA_Dec(double ra, double dec)
{
    auto time = QDateTime::currentDateTime();
    double ha = cs->Convert_RA2HA(ra, time);
    this->point_ha = ha;
    this->point_dec = dec;
    this->target_ra = ra;
    this->target_dec = dec;
    mode = TrackerHoldRADec;
}

void Tracker::Init_Track_HA_Dec(double ha, double dec)
{
    this->point_ha = ha;
    this->point_dec = dec;
    this->target_ha = ha;
    this->target_dec = dec;
    mode = TrackerHoldHADec;
}

void Tracker::Init_Track_Az_Alt(double az, double alt)
{
    std::tuple<double, double> hadec = cs->Convert_from_Az_Alt(az, alt);
    double ha = std::get<0>(hadec);
    double dec = std::get<1>(hadec);

    this->point_ha = ha;
    this->point_dec = dec;
    this->target_az = az;
    this->target_alt = alt;
    mode = TrackerHoldAzAlt;
}

void Tracker::StopTracking()
{
    mode = TrackerHoldNone;
}

void Tracker::Set_Target_RA_Dec(double ra, double dec)
{
    target_ra = ra;
    target_dec = dec;
}

void Tracker::Set_Target_HA_Dec(double ha, double dec)
{
    target_ha = ha;
    target_dec = dec;
}

void Tracker::Set_Target_Az_Alt(double az, double alt)
{
    target_az = az;
    target_alt = alt;
}

QDateTime Tracker::NextFinishTime(double delta_t)
{
    QDateTime new_finish_time;
    if (finish_time < QDateTime::currentDateTime())
        new_finish_time = QDateTime::currentDateTime().addSecs(delta_t);
    else
        new_finish_time = finish_time.addSecs(delta_t);
    return new_finish_time;
}

std::tuple<double, double> Tracker::Track(double target_ha, double target_dec, QDateTime new_finish_time, double delta_t)
{
    // вычисляем необходимую дельту для прихода в нужную точку
    double delta_ha = target_ha - point_ha;
    double delta_dec = target_dec - point_dec;

    if (delta_ha > 12)
        delta_ha = -(24 - delta_ha);
    else if (delta_ha < -12)
        delta_ha = (-24 - delta_ha);

    if (delta_dec > 180)
        delta_dec = -(360 - delta_dec);
    else if (delta_dec < -180)
        delta_dec = -(-360 - delta_dec);

    // максимальная дельта за указанное время
    double max_delta_ha = delta_t / cfg->x_rotation_time * 24;
    double max_delta_dec = delta_t / cfg->y_rotation_time * 360;

    // ограничиваем дельту максимальными значениям
    double p_delta_ha = delta_ha;
    double p_delta_dec = delta_dec;

    if (abs(p_delta_ha) > max_delta_ha)
    {
        p_delta_ha *= max_delta_ha / abs(p_delta_ha);
        p_delta_dec *= max_delta_ha / abs(p_delta_ha);
    }
    if (abs(delta_dec) > max_delta_dec)
    {
        p_delta_ha *= max_delta_dec / abs(p_delta_dec);
        p_delta_dec *= max_delta_dec / abs(p_delta_dec);
    }

    // сохраняем сдвинутый конец отрезка
    point_ha = point_ha + p_delta_ha;
    point_dec = point_dec + p_delta_dec;
    finish_time = new_finish_time;

    return std::make_tuple(p_delta_ha, p_delta_dec);
}

std::tuple<double, double, double> Tracker::Track_RA_Dec(double delta_t, double new_target_ra, double new_target_dec)
{
    QDateTime new_finish_time = NextFinishTime(delta_t);
    double new_target_ha = cs->Convert_RA2HA(new_target_ra, new_finish_time);
    std::tuple<double, double> delta = Track(new_target_ha, new_target_dec, new_finish_time, delta_t);
    return std::make_tuple(std::get<0>(delta), std::get<1>(delta), delta_t);
}

std::tuple<double, double, double> Tracker::Track_HA_Dec(double delta_t, double new_target_ha, double new_target_dec)
{
    QDateTime new_finish_time = NextFinishTime(delta_t);
    std::tuple<double, double> delta = Track(new_target_ha, new_target_dec, new_finish_time, delta_t);
    return std::make_tuple(std::get<0>(delta), std::get<1>(delta), delta_t);
}

std::tuple<double, double, double> Tracker::Track_Az_Alt(double delta_t, double new_target_az, double new_target_alt)
{
    QDateTime new_finish_time = NextFinishTime(delta_t);
    std::tuple<double, double> hadec = cs->Convert_from_Az_Alt(new_target_az, new_target_alt);
    double new_target_ha = std::get<0>(hadec);
    double new_target_dec = std::get<1>(hadec);

    std::tuple<double, double> delta = Track(new_target_ha, new_target_dec, new_finish_time, delta_t);
    return std::make_tuple(std::get<0>(delta), std::get<1>(delta), delta_t);
}

std::tuple<double, double, double> Tracker::ProcessTrack(double delta_t)
{
    delta_t *= 4;

    switch(mode)
    {
    case TrackerHoldRADec:
        return Track_RA_Dec(delta_t, target_ra, target_dec);
    case TrackerHoldHADec:
        return Track_HA_Dec(delta_t, target_ha, target_dec);
    case TrackerHoldAzAlt:
        return Track_Az_Alt(delta_t, target_az, target_alt);
    default:
        return std::make_tuple(0, 0, 0);
    }
}

void Tracker::InvertCoordinates()
{
    auto p = cs->Inverted_HA_Dec_Coordinates(point_ha, point_dec);
    point_ha = std::get<0>(p);
    point_dec = std::get<1>(p);

    switch(mode)
    {
    case TrackerHoldRADec:
    {
        target_ha = cs->Convert_RA2HA(target_ra, finish_time);
        auto r = cs->Inverted_HA_Dec_Coordinates(target_ha, target_dec);
        target_ra = cs->Convert_HA2RA(std::get<0>(r), finish_time);
        target_dec = std::get<1>(r);
        break;
    }
    case TrackerHoldHADec:
    {
        auto r = cs->Inverted_HA_Dec_Coordinates(target_ha, target_dec);
        target_ha = std::get<0>(r);
        target_dec = std::get<1>(r);
        break;
    }
    case TrackerHoldAzAlt:
    {
        auto s = cs->Convert_from_Az_Alt(target_az, target_alt);
        auto r = cs->Inverted_HA_Dec_Coordinates(std::get<0>(s), std::get<1>(s));
        auto m = cs->Convert_to_Az_Alt(std::get<0>(r), std::get<1>(r));
        target_az = std::get<0>(m);
        target_alt = std::get<1>(m);
        break;
    }
    default:
        break;
    }
}

TrackerMode Tracker::Get_Tracking_Target(double *a, double *b)
{
    switch(mode)
    {
    case TrackerHoldRADec:
        *a = target_ra;
        *b = target_dec;
        break;
    case TrackerHoldHADec:
        *a = target_ha;
        *b = target_dec;
        break;
    case TrackerHoldAzAlt:
        *a = target_az;
        *b = target_alt;
        break;
    case TrackerHoldNone:
        break;
    }
    return mode;
}
