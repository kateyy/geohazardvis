#include "GuiPlugin.h"


GuiPlugin::GuiPlugin(
    const QString & name, const QString & description, const QString & vendor, const QString & version, GuiPluginInterface && pluginInterface)
    : m_name(name)
    , m_description(description)
    , m_vendor(vendor)
    , m_version(version)
    , m_pluginInterface(pluginInterface)
{
}

GuiPlugin::~GuiPlugin() = default;

const QString & GuiPlugin::name() const
{
    return m_name;
}

const QString & GuiPlugin::description() const
{
    return m_description;
}

const QString & GuiPlugin::vendor() const
{
    return m_vendor;
}

const QString & GuiPlugin::version() const
{
    return m_version;
}
