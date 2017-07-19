/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <core/utility/mathhelper.h>

#include <cmath>
#include <map>

#include <QDebug>


namespace
{

const std::map<QString, int> & unitsExponents()
{
    static const std::map<QString, int> map = {
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
        { "", 0 },
        { "d", -1 },
        { "c", -2 },
        { "m", -3 },
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
        exponent += expIt->second;
    }

    if (toPrefix.length() > 0)
    {
        auto expIt = unitsExponents().find(toPrefix);
        if (expIt == unitsExponents().end())
        {
            return warnInvalid();
        }
        exponent -= expIt->second;
    }

    return std::pow(10.0, exponent);
}

bool isValidMetricUnit(const QString & unit)
{
    return unitsExponents().find(unit.left(unit.length() - 1)) != unitsExponents().end();
}

}
