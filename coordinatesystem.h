#ifndef COORDINATESYSTEM_H
#define COORDINATESYSTEM_H

#include <QTimeZone>

class CoordinateSystem
{
private:
    QTimeZone tz;
    double latitude;
    double longitude;
    QDateTime datetime2000;
private:
    qint64 Time2000(QDateTime datetime);
    std::tuple<qint64, double, double> GlobalSidericTime(QDateTime datetime);
    std::tuple<qint64, double, double> LocalSidericTime(QDateTime datetime);
    double ra2ha(double ra, double lst);
    double ha2ra(double ha, double lst);
public:
    CoordinateSystem(QTimeZone tz, double longitude, double latitude);
    double Convert_HA2RA(double ha, QDateTime datetime);
    double Convert_RA2HA(double ra, QDateTime datetime);

    std::tuple<double, double> Convert_to_Az_Alt(double ha, double dec);
    std::tuple<double, double> Convert_from_Az_Alt(double az, double alt);

    std::tuple<double, double> Inverted_HA_Dec_Coordinates(double ha, double dec);
    std::tuple<bool, double, double> Normalized_HA_Dec_Coordinates(double ha, double dec);
};

#endif // COORDINATESYSTEM_H
