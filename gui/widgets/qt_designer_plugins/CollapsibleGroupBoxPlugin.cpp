#include "CollapsibleGroupBoxPlugin.h"

#include "../CollapsibleGroupBox.h"


CollapsibleGroupBoxPlugin::CollapsibleGroupBoxPlugin(QObject * parent)
    : QObject(parent)
    , m_isInitialized{ false }
{
}

bool CollapsibleGroupBoxPlugin::isContainer() const
{
    return true;
}

bool CollapsibleGroupBoxPlugin::isInitialized() const
{
    return m_isInitialized;
}

QIcon CollapsibleGroupBoxPlugin::icon() const
{
    return{};
}

QString CollapsibleGroupBoxPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
        " <widget class=\"CollapsibleGroupBox\" name=\"groupBox\" />\n"
        "</ui>\n";
}

QString CollapsibleGroupBoxPlugin::group() const
{
    return "Containers";
}

QString CollapsibleGroupBoxPlugin::includeFile() const
{
    return "gui/widgets/CollapsibleGroupBox.h";
}

QString CollapsibleGroupBoxPlugin::name() const
{
    return "CollapsibleGroupBox";
}

QString CollapsibleGroupBoxPlugin::toolTip() const
{
    return{};
}

QString CollapsibleGroupBoxPlugin::whatsThis() const
{
    return QString();
}

QWidget * CollapsibleGroupBoxPlugin::createWidget(QWidget * parent)
{
    return new CollapsibleGroupBox(parent);
}

void CollapsibleGroupBoxPlugin::initialize(QDesignerFormEditorInterface * /*core*/)
{
    if (m_isInitialized)
    {
        return;
    }

    m_isInitialized = true;
}
