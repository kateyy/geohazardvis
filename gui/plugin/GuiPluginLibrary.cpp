#include "GuiPluginLibrary.h"


GuiPluginLibrary::GuiPluginLibrary(const QString & filePath)
    : m_filePath(filePath)
    , m_initPtr(nullptr)
    , m_releasePtr(nullptr)
{
}

GuiPluginLibrary::~GuiPluginLibrary() = default;

const QString & GuiPluginLibrary::filePath() const
{
    return m_filePath;
}

bool GuiPluginLibrary::isValid() const
{
    return (m_initPtr && m_pluginPtr && m_releasePtr);
}

void GuiPluginLibrary::initialize(GuiPluginInterface && pluginInterface)
{
    if (m_initPtr != nullptr)
        (*m_initPtr)(pluginInterface);
}

void GuiPluginLibrary::release()
{
    if (m_releasePtr != nullptr)
        (*m_releasePtr)();
}

GuiPlugin * GuiPluginLibrary::plugin() const
{
    if (m_pluginPtr != nullptr)
        return (*m_pluginPtr)();

    return nullptr;
}
