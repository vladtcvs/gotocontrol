#include <QtMath>
#include "coordinatesystem.h"

CoordinateSystem::CoordinateSystem(QTimeZone tz, double longitude)
{
    this->tz = tz;
    this->longitude = longitude;
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
