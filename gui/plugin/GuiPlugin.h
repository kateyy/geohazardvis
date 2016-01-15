#pragma once

#include <QString>

#include <gui/gui_api.h>
#include <gui/plugin/GuiPluginInterface.h>


class GUI_API GuiPlugin
{
public:
    explicit GuiPlugin(
        const QString & name,
        const QString & description,
        const QString & vendor,
        const QString & version,
        GuiPluginInterface && pluginInterface);

    virtual ~GuiPlugin();

    const QString & name() const;
    const QString & description() const;
    const QString & vendor() const;
    const QString & version() const;

protected:
    QString m_name;
    QString m_description;
    QString m_vendor;
    QString m_version;

    GuiPluginInterface m_pluginInterface;
};
