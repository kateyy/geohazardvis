#include "ColorEditorRGB.h"

#include <QColorDialog>
#include <QSpinBox>
#include <QLabel>
#include <QHBoxLayout>

#include <reflectionzeug/Property.h>

#include <reflectionzeug/ColorPropertyInterface.h>

#include <propertyguizeug/ColorButton.h>

using namespace reflectionzeug;
using namespace propertyguizeug;


ColorEditorRGB::ColorEditorRGB(reflectionzeug::ColorPropertyInterface * property, QWidget * parent)
    : PropertyEditor(parent)
    , m_property(property)
{
    const Color & color = m_property->toColor();
    QColor qcolor(color.red(), color.green(), color.blue(), color.alpha());

    boxLayout()->setSpacing(2);

    m_button = new ColorButton(this, qcolor);
    boxLayout()->addWidget(m_button);

    QHBoxLayout * spinBoxLayout = new QHBoxLayout();
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
    QColor qcolor = QColorDialog::getColor(this->qcolor(),
        m_button,
        "Choose Color",
        QColorDialog::ShowAlphaChannel);
    if (qcolor.isValid())
        setQColor(qcolor);
}

void ColorEditorRGB::readUiColor(int)
{
    QColor uiColor(m_spinBoxes[0]->value(), m_spinBoxes[1]->value(), m_spinBoxes[2]->value());

    setQColor(uiColor);
}

QColor ColorEditorRGB::qcolor() const
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
