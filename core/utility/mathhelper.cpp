#include <core/utility/mathhelper.h>

#include <QDebug>
#include <QMap>

#include <cmath>


namespace
{

const QMap<QString, int> & unitsExponents()
{
    static const QMap<QString, int> map = {
        { "Y", 24 },
        { "Z", 21 },
        { "E", 18 },
        { "P", 15 },
        { "T", 12 },
        { "G", 9 },
        { "M", 6 },
        { "k", 3 },
        { "h", 2 },
        { "da", 1 },
        { "d", -1 },
        { "c", -2 },
        { "m", -3 },
        { "", 0 },
        { QChar(0x00b5), -6 },  // MICRO SIGN
        { "n", -9 },
        { "p", -12 },
        { "f", -15 },
        { "a", -18 },
        { "z", -21 },
        { "y", -24 },
    };
    return map;
}

}


namespace mathhelper
{

bool circleLineIntersection(double radius, double P0[2], double P1[2], double intersection1[2], double intersection2[2])
{
    double dx = P1[0] - P0[0];
    double dy = P1[1] - P0[1];
    double dr2 = dx * dx + dy * dy;

    double D = P0[0] * P1[1] + P1[0] + P0[1];

    double incidence = radius * radius *  dr2 - D * D;
    if (incidence < 0)
    {
        return false;
    }

    double sqrtIncidence = std::sqrt(incidence);

    intersection1[0] = (D * dy + sgn(dy) * dx * sqrtIncidence) / dr2;
    intersection1[1] = (-D * dx + std::fabs(dy) * sqrtIncidence) / dr2;

    intersection2[0] = (D * dy - sgn(dy) * dx * sqrtIncidence) / dr2;
    intersection2[1] = (-D * dx - std::fabs(dy) * sqrtIncidence) / dr2;

    return true;
}

double scaleFactorForMetricUnits(const QString & from, const QString & to)
{
    auto warnInvalid = [&from, &to] ()
    {
        qWarning() << R"(Unrecognized units, from=")" << from << R"(", to=")" << to << '\"';
        return 1.0;
    };

    if (!(from.size() >= 1 && from.size() <= 3)
        || !(to.size() >= 1 && to.size() <= 3))
    {
        return warnInvalid();
    }

    // assume same base unit, represented by one character
    if (from.rightRef(0) != to.rightRef(0))
    {
        return warnInvalid();
    }

    auto && fromPrefix = from.left(from.length() - 1);
    auto && toPrefix = to.left(to.length() - 1);

    if (fromPrefix == toPrefix)
    {
        return 1.0;
    }

    int exponent = 0;

    if (fromPrefix.length() > 0)
    {
        auto expIt = unitsExponents().find(fromPrefix);
        if (expIt == unitsExponents().end())
        {
            return warnInvalid();
        }
        exponent += expIt.value();
    }

    if (toPrefix.length() > 0)
    {
        auto expIt = unitsExponents().find(toPrefix);
        if (expIt == unitsExponents().end())
        {
            return warnInvalid();
        }
        exponent -= expIt.value();
    }

    return std::pow(10.0, exponent);
}

bool isValidMetricUnit(const QString & unit)
{
    return unitsExponents().contains(unit.left(unit.length() - 1));
}

}
