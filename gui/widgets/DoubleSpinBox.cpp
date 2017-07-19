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

#include "DoubleSpinBox.h"

#include <cmath>

#include <QDoubleValidator>
#include <QLineEdit>


namespace
{

char notationToFormatLetter(const DoubleSpinBox::Notation notation)
{
    switch (notation)
    {
    case DoubleSpinBox::Notation::Standard:
        return 'e';
    case DoubleSpinBox::Notation::Scientific:
        return 'f';
    case DoubleSpinBox::Notation::MostConcise:
        return 'g';
    default:
        return 0;
    }
}

template<typename T>
T clamp(const T value, const T min, const T max)
{
    return std::min(std::max(min, value), max);
}

}


class DoubleSpinBoxPrivate
{
public:
    DoubleSpinBox & q_ptr;
    std::unique_ptr<QDoubleValidator> validator;
    DoubleSpinBox::Notation notation;

    explicit DoubleSpinBoxPrivate(DoubleSpinBox & q_ptr)
        : q_ptr{ q_ptr }
        , validator{ std::make_unique<QDoubleValidator>() }
        , notation{ DoubleSpinBox::Notation::MostConcise }
    {
    }

    QString stripped(const QString & t) const
    {
        auto text = t;

        if (q_ptr.specialValueText().isEmpty() || text != q_ptr.specialValueText())
        {
            int from = 0;
            int size = text.size();
            bool changed = false;
            if (!q_ptr.prefix().isEmpty() && text.startsWith(q_ptr.prefix()))
            {
                from += q_ptr.prefix().size();
                size -= from;
                changed = true;
            }
            if (!q_ptr.suffix().isEmpty() && text.endsWith(q_ptr.suffix()))
            {
                size -= q_ptr.suffix().size();
                changed = true;
            }
            if (changed)
            {
                text = text.mid(from, size);
            }
        }

        return text.trimmed();
    }

    double validate(QString & text, int & pos, QValidator::State & state) const
    {
        auto copy = stripped(text);
        state = validator->validate(copy, pos);
        text = q_ptr.prefix() + copy + q_ptr.suffix();

        switch (state)
        {
        case QValidator::State::Acceptable:
            return copy.toDouble();
        case QValidator::State::Intermediate:
            return clamp(copy.toDouble(), q_ptr.minimum(), q_ptr.maximum());
        //case QValidator::State::Invalid:
        default:
            return q_ptr.minimum();
        }
    }
};


DoubleSpinBox::DoubleSpinBox(QWidget * parent)
    : QDoubleSpinBox(parent)
    , d_ptr{ std::make_unique<DoubleSpinBoxPrivate>(*this) }
{
    setDecimals(d_ptr->validator->decimals());
    setRange(d_ptr->validator->bottom(), d_ptr->validator->top());

    lineEdit()->setValidator(d_ptr->validator.get());
}

DoubleSpinBox::~DoubleSpinBox() = default;

void DoubleSpinBox::setNotation(Notation notation)
{
    if (d_ptr->notation == notation)
    {
        return;
    }

    d_ptr->notation = notation;
    lineEdit()->update();
}

DoubleSpinBox::Notation DoubleSpinBox::notation() const
{
    return d_ptr->notation;
}

QValidator::State DoubleSpinBox::validate(QString & input, int & pos) const
{
    QValidator::State state;
    d_ptr->validate(input, pos, state);
    return state;
}

double DoubleSpinBox::valueFromText(const QString & text) const
{
    auto copy = text;
    int pos = 0;
    QValidator::State state;
    return d_ptr->validate(copy, pos, state);
}

QString DoubleSpinBox::textFromValue(double val) const
{
    auto str = QString::number(val,
        notationToFormatLetter(d_ptr->notation),
        decimals());
    fixup(str);
    return str;
}
