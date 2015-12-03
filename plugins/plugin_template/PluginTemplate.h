#pragma once

#include <memory>

#include <gui/plugin/GuiPlugin.h>


class QDockWidget;
class QLineEdit;


class PluginTemplate : public GuiPlugin
{
public:
    explicit PluginTemplate(
        const QString & name,
        const QString & description,
        const QString & vendor,
        const QString & version,
        GuiPluginInterface && pluginInterface);

    ~PluginTemplate() override;

private:
    std::unique_ptr<QDockWidget> m_dockWidget;
    QLineEdit * m_lineEdit;
};
