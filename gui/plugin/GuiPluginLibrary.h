#pragma once

#include <QString>


class GuiPlugin;
class GuiPluginInterface;


class GuiPluginLibrary
{
public:
    using init_ptr = void(*)(GuiPluginInterface &);
    using plugin_ptr = GuiPlugin * (*)();
    using release_ptr = void(*)();

    explicit GuiPluginLibrary(const QString & filePath);
    virtual ~GuiPluginLibrary();

    const QString & filePath() const;

    bool isValid() const;

    void initialize(GuiPluginInterface && pluginInterface);
    void release();

    GuiPlugin * plugin() const;

protected:
    QString m_filePath;
    init_ptr m_initPtr;
    release_ptr m_releasePtr;
    plugin_ptr m_pluginPtr;
};
