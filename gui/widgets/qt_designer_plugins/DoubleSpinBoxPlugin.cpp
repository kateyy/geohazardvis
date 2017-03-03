#include "DoubleSpinBoxPlugin.h"

#include "../DoubleSpinBox.h"


DoubleSpinBoxPlugin::DoubleSpinBoxPlugin(QObject * parent)
    : QObject(parent)
    , m_isInitialized{ false }
{
}

bool DoubleSpinBoxPlugin::isContainer() const
{
    return false;
}

bool DoubleSpinBoxPlugin::isInitialized() const
{
    return m_isInitialized;
}

QIcon DoubleSpinBoxPlugin::icon() const
{
    return{};
}

QString DoubleSpinBoxPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
        " <widget class=\"DoubleSpinBox\" name=\"doubleSpinBox\" />\n"
        "</ui>\n";
}

QString DoubleSpinBoxPlugin::group() const
{
    return "Input Widgets";
}

QString DoubleSpinBoxPlugin::includeFile() const
{
    return "gui/widgets/DoubleSpinBox.h";
}

QString DoubleSpinBoxPlugin::name() const
{
    return "DoubleSpinBox";
}

QString DoubleSpinBoxPlugin::toolTip() const
{
    return{};
}

QString DoubleSpinBoxPlugin::whatsThis() const
{
    return QString();
}

QWidget * DoubleSpinBoxPlugin::createWidget(QWidget * parent)
{
    return new DoubleSpinBox(parent);
}

void DoubleSpinBoxPlugin::initialize(QDesignerFormEditorInterface * /*core*/)
{
    if (m_isInitialized)
    {
        return;
    }

    m_isInitialized = true;
}
