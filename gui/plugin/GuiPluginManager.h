#pragma once

#include <memory>
#include <vector>

#include <QStringList>


class GuiPlugin;
class GuiPluginInterface;
class GuiPluginLibrary;


/**
Plugin loading and management based on gloperate's plugin system
@see https://github.com/cginternals/gloperate
*/
class GuiPluginManager
{
public:
    GuiPluginManager();

    virtual ~GuiPluginManager();

    /** Access the paths that will be searched for plugins */
    QStringList & searchPaths();
    const QStringList & searchPaths() const;

    /** Load all plugins found in the search paths */
    void scan(GuiPluginInterface pluginInterface);

    const QList<GuiPlugin *> & plugins() const;
    QList<GuiPluginLibrary *> pluginLibraries() const;

private:
    bool loadLibrary(const QString & filePath, GuiPluginInterface && pluginInterface);
    void unloadLibrary(std::unique_ptr<GuiPluginLibrary> library);

private:
    QStringList m_searchPaths;

    std::vector<std::unique_ptr<GuiPluginLibrary>> m_libraries;
    QList<GuiPlugin *> m_plugins;
};
