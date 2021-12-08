#include <QtMath>
#include "coordinatesystem.h"

CoordinateSystem::CoordinateSystem(QTimeZone tz, double longitude, double latitude)
{
    this->tz = tz;
    this->longitude = longitude;
    this->latitude = latitude;
    datetime2000 = QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0, 0), QTimeZone::utc());
}


qint64 CoordinateSystem::Time2000(QDateTime datetime)
{
    qint64 timenow = datetime.toSecsSinceEpoch();
    qint64 time2000 = datetime2000.toSecsSinceEpoch();
    return timenow - time2000;
}

std::tuple<qint64, double, double> CoordinateSystem::GlobalSidericTime(QDateTime datetime)
{
    double delta = Time2000(datetime) / 86400.0;
    const double L0 = 99.967794687;
    const double L1 = 360.98564736628603;
    const double L2 = 2.907879e-13;
    const double L3 = -5.302e-22;

    double angle = L0 + L1 * delta + L2 * delta*delta + L3 * delta*delta*delta;
    qint64 rounds = floor(angle / 360);
    double gst = angle - rounds * 360;
    return std::make_tuple(rounds, gst * 24 / 360, angle * 24/360);
}

std::tuple<qint64, double, double> CoordinateSystem::LocalSidericTime(QDateTime datetime)
{
    std::tuple<qint64, double, double> gst = GlobalSidericTime(datetime);
    double total_gst = std::get<2>(gst);
    double total_lst = total_gst + longitude * 24/360;
    qint64 rounds = floor(total_lst / 24);
    double lst = total_lst - rounds * 24;
    return std::make_tuple(rounds, lst, total_lst);
}

double CoordinateSystem::ra2ha(double ra, double lst)
{
    double ha = lst - ra;
    if (ha < 0)
        ha += 24;
    else if (ha >= 24)
        ha -= 24;
    return ha;
}

double CoordinateSystem::ha2ra(double ha, double lst)
{
    double ra = lst - ha;
    if (ra < 0)
        ra += 24;
    else if (ra >= 24)
        ra -= 24;
    return ra;
}

double CoordinateSystem::Convert_HA2RA(double ha, QDateTime datetime)
{
    std::tuple<qint64, double, double> lst = LocalSidericTime(datetime);
    return ha2ra(ha, std::get<1>(lst));
}

double CoordinateSystem::Convert_RA2HA(double ra, QDateTime datetime)
{
    std::tuple<qint64, double, double> lst = LocalSidericTime(datetime);
    return ra2ha(ra, std::get<1>(lst));
}

/* http://www.stargazing.net/kepler/altaz.html
 *
 * sin(ALT) = sin(DEC)*sin(LAT)+cos(DEC)*cos(LAT)*cos(HA)
 *
 *               sin(DEC) - sin(ALT)*sin(LAT)
 * cos(A)   = ---------------------------------
 *                    cos(ALT)*cos(LAT)
 *
 * If sin(HA) is negative, then AZ = A, otherwise
 * AZ = 360 - A
 */

std::tuple<double, double> CoordinateSystem::Convert_to_Az_Alt(double ha, double dec)
{
    std::tuple<bool, double, double> normed = Normalized_HA_Dec_Coordinates(ha, dec);
    ha = std::get<1>(normed);
    dec = std::get<2>(normed);

    double LAT = latitude * M_PI/180;
    dec = dec * M_PI / 180;
    ha = ha * M_PI / 12;
    double sinalt = sin(dec)*sin(LAT) + cos(dec)*cos(LAT)*cos(ha);
    if (sinalt > 1)
        sinalt = 1;
    if (sinalt < -1)
        sinalt = -1;

    double alt = asin(sinalt);
    double cosa = (sin(dec) - sinalt*sin(LAT)) / (cos(alt)*cos(LAT));
    if (cosa > 1)
        cosa = 1;
    if (cosa < -1)
        cosa = -1;

    double a = acos(cosa);
    double az;
    if (sin(ha) < 0)
        az = a;
    else
        az = 2*M_PI-a;
    return std::make_tuple(az*180/M_PI, alt*180/M_PI);
}

/*
 * "Practical astronomy with your calculator" Duffett-Smith
 * https://www.cloudynights.com/topic/448682-help-w-conversion-of-altaz-to-radec-for-dsc/
 *
 * sin(D) = sin(A) * sin(L) + cos(A) * cos(L) * cos(AZ)
 * cos(H) = (sin(A)-sin(L) * sin(D)) / (cos(L) * cos(D))
 *
 * D=declination
 * H=hour angle
 * A=altitude
 * AZ-azimuth
 * L=latitude
 */

std::tuple<double, double> CoordinateSystem::Convert_from_Az_Alt(double az, double alt)
{
    double LAT = latitude * M_PI/180;
    az = az * M_PI/180;
    alt = alt * M_PI/180;
    double sindec = sin(alt)*sin(LAT) + cos(alt)*cos(LAT)*cos(az);
    if (sindec > 1)
        sindec = 1;
    if (sindec < -1)
        sindec = -1;
    double dec = asin(sindec);
    double cosha = (sin(alt)-sin(LAT)*sindec) / (cos(LAT)*cos(dec));
    if (cosha > 1)
        cosha = 1;
    if (cosha < -1)
        cosha = -1;
    double ha = acos(cosha);
    if (sin(az) > 0)
        ha = 2*M_PI - ha;

    return std::make_tuple(ha*12/M_PI, dec*180/M_PI);
}

std::tuple<double, double> CoordinateSystem::Inverted_HA_Dec_Coordinates(double ha, double dec)
{
    if (dec > 0)
        dec = 180-dec;
    else
        dec = -180-dec;
    ha = ha + 12;
    while (ha < 0)
        ha += 24;
    while (ha > 24)
        ha -= 24;
    return std::make_tuple(ha, dec);
}

std::tuple<bool, double, double> CoordinateSystem::Normalized_HA_Dec_Coordinates(double ha, double dec)
{
    if (dec > 90 || dec < -90)
    {
        auto inv = Inverted_HA_Dec_Coordinates(ha, dec);
        return std::make_tuple(true, std::get<0>(inv), std::get<1>(inv));
    }
    while (ha < 0)
        ha += 24;
    while (ha > 24)
        ha -= 24;
    return std::make_tuple(false, ha, dec);
}
