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

#pragma once

#include <memory>

#include <QDoubleSpinBox>


#if COMPILE_QT_DESIGNER_PLUGIN
#define GUI_API
#else
#include <gui/gui_api.h>
#endif


class DoubleSpinBoxPrivate;


class GUI_API DoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
    Q_PROPERTY(Notation notation READ notation WRITE setNotation)

public:
    explicit DoubleSpinBox(QWidget * parent = nullptr);
    ~DoubleSpinBox() override;

    enum Notation
    {
        Standard,
        Scientific,
        MostConcise
    };
    Q_ENUM(Notation)
    void setNotation(Notation notation);
    Notation notation() const;

signals:
    void notationChanged(Notation notation);

protected:
    QValidator::State validate(QString &input, int &pos) const override;
    double valueFromText(const QString &text) const override;
    QString textFromValue(double val) const override;

private:
    std::unique_ptr<DoubleSpinBoxPrivate> d_ptr;

private:
    Q_DISABLE_COPY(DoubleSpinBox)
};
