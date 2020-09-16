#ifndef COORDINATESYSTEM_H
#define COORDINATESYSTEM_H

#include <QTimeZone>

class CoordinateSystem
{
private:
    QTimeZone tz;
    double longitude;
    QDateTime datetime2000;
private:
    qint64 Time2000(QDateTime datetime);
    std::tuple<qint64, double, double> GlobalSidericTime(QDateTime datetime);
    std::tuple<qint64, double, double> LocalSidericTime(QDateTime datetime);
    double ra2ha(double ra, double lst);
    double ha2ra(double ha, double lst);
public:
    CoordinateSystem(QTimeZone tz, double longitude);
    double Convert_HA2RA(double ha, QDateTime datetime);
    double Convert_RA2HA(double ra, QDateTime datetime);
};

#endif // COORDINATESYSTEM_H
