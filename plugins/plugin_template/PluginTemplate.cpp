#include "PluginTemplate.h"

#include <QDockWidget>
#include <QLineEdit>
#include <QSettings>

#include <gui/plugin/GuiPluginInterface.h>


namespace
{
const char settingsPath[] = "PluginTemplate/EditText";
}


PluginTemplate::PluginTemplate(const QString & name, const QString & description, const QString & vendor, const QString & version, GuiPluginInterface && pluginInterface)
    : GuiPlugin(name, description, vendor, version, std::move(pluginInterface))
    , m_dockWidget(std::make_unique<QDockWidget>())
{
    QString text;
    m_pluginInterface.readSettings([&text] (const QSettings & settings) {
        text = settings.value(settingsPath, "Hello World").toString();
    });

    auto widget = new QWidget();
    m_dockWidget->setWidget(widget);

    m_lineEdit = new QLineEdit(text, widget);

    m_pluginInterface.addWidget(m_dockWidget.get());
}

PluginTemplate::~PluginTemplate()
{
    m_pluginInterface.removeWidget(m_dockWidget.get());

    m_pluginInterface.readWriteSettings([this] (QSettings & settings) {
        settings.setValue(settingsPath, m_lineEdit->text());
    });
}
