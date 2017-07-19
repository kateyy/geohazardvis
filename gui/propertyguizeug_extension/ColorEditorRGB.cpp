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

#include "ColorEditorRGB.h"

#include <QApplication>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QStyleOptionViewItem>

#include <reflectionzeug/Property.h>
#include <reflectionzeug/ColorPropertyInterface.h>

#include <gui/propertyguizeug_extension/ColorButtonWithBorder.h>


using namespace reflectionzeug;
using namespace propertyguizeug;


void ColorEditorRGB::paint(QPainter * painter,
    const QStyleOptionViewItem & option,
    reflectionzeug::ColorPropertyInterface & property)
{
    const Color & color = property.toColor();

    QColor qcolor(color.red(),
        color.green(),
        color.blue(),
        color.alpha());

    const auto metrics = option.widget->fontMetrics();
    const auto buttonSize = ColorButton::sizeFromFontHeight(metrics.height());
    auto buttonRect = QRect{ option.rect.topLeft(), buttonSize };
    buttonRect.moveCenter({ buttonRect.center().x(), option.rect.center().y() });
    const auto topLeft = buttonRect.topLeft();

    auto widget = option.widget;
    auto style = widget ? widget->style() : QApplication::style();

    ColorButtonWithBorder::paint(painter, topLeft, qcolor);

    auto rect = option.rect;
    rect.setLeft(option.rect.left() + buttonSize.width() + 4);

    QString colorString = "R: " + QString::number(color.red()) + " G: " + QString::number(color.green()) + " B: " + QString::number(color.blue());

    style->drawItemText(painter,
        rect,
        Qt::AlignVCenter,
        option.palette,
        true,
        colorString,
        QPalette::Text);
}

ColorEditorRGB::ColorEditorRGB(reflectionzeug::ColorPropertyInterface * property, QWidget * parent)
    : PropertyEditor(parent)
    , m_property(property)
{
    const Color & color = m_property->toColor();
    QColor qcolor(color.red(), color.green(), color.blue(), color.alpha());

    boxLayout()->setSpacing(2);

    m_button = new ColorButtonWithBorder(this, qcolor);
    boxLayout()->addWidget(m_button);

    auto spinBoxLayout = new QHBoxLayout();
    boxLayout()->addLayout(spinBoxLayout);
    spinBoxLayout->setMargin(0);
    spinBoxLayout->setSpacing(0);

    for (QSpinBox * & spinBox : m_spinBoxes)
    {
        spinBox = new QSpinBox(this);
        spinBox->setRange(0x00, 0xFF);
        spinBox->setSingleStep(5);
        spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
        spinBoxLayout->addWidget(spinBox);
        connect(spinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ColorEditorRGB::readUiColor);
    }
    m_spinBoxes[0]->setValue(color.red());
    m_spinBoxes[1]->setValue(color.green());
    m_spinBoxes[2]->setValue(color.blue());

    setFocusProxy(m_spinBoxes[0]);

    this->connect(m_button, &ColorButton::pressed, this, &ColorEditorRGB::openColorPicker);
}

ColorEditorRGB::~ColorEditorRGB() = default;

void ColorEditorRGB::openColorPicker()
{
    QColor qcolor = QColorDialog::getColor(
        qColor(),
        m_button,
        "Choose Color");

    if (qcolor.isValid())
    {
        setQColor(qcolor);
    }
}

void ColorEditorRGB::readUiColor(int)
{
    QColor uiColor(m_spinBoxes[0]->value(), m_spinBoxes[1]->value(), m_spinBoxes[2]->value());

    setQColor(uiColor);
}

QColor ColorEditorRGB::qColor() const
{
    const Color & color = m_property->toColor();
    return QColor(color.red(), color.green(), color.blue(), color.alpha());
}

void ColorEditorRGB::setQColor(const QColor & qcolor)
{
    Color color(qcolor.red(), qcolor.green(), qcolor.blue(), qcolor.alpha());
    m_property->fromColor(color);

    m_button->setColor(qcolor);
    m_spinBoxes[0]->setValue(qcolor.red());
    m_spinBoxes[1]->setValue(qcolor.green());
    m_spinBoxes[2]->setValue(qcolor.blue());
}

void ColorEditorRGB::setColor(const Color & color)
{
    QColor qcolor(color.red(), color.green(), color.blue(), color.alpha());

    setQColor(qcolor);
}
