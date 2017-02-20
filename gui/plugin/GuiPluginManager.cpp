#include "gui/plugin/GuiPluginManager.h"

#include <algorithm>
#include <sys/stat.h>

#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#include <libgen.h>
#include <dirent.h>
#endif

#include <QDebug>
#include <QDir>

#include <gui/plugin/GuiPlugin.h>
#include <gui/plugin/GuiPluginLibrary.h>


namespace
{

QString libFileNameFilter()
{
#ifdef WIN32
    return "*.dll";
#elif __APPLE__
    return "*.dylib";
#else
    return "*.so";
#endif
}

#ifdef WIN32
const int RTLD_LAZY(0); // ignore for win32 - see dlopen
#endif

class PluginLibraryImpl : public GuiPluginLibrary
{
public:
    explicit PluginLibraryImpl(const QString & filePath)
        : GuiPluginLibrary(filePath)
        , m_handle{ nullptr }
    {
        m_handle = dlopen(filePath.toUtf8().data(), RTLD_LAZY);
        if (!m_handle)
        {
            qDebug() << dlerror();
            return;
        }

        *reinterpret_cast<void**>(&m_initPtr) = dlsym(m_handle, "initialize");
        *reinterpret_cast<void**>(&m_pluginPtr) = dlsym(m_handle, "plugin");
        *reinterpret_cast<void**>(&m_releasePtr) = dlsym(m_handle, "release");
    }

    ~PluginLibraryImpl() override
    {
        if (m_handle)
        {
            dlclose(m_handle);
        }
    }

private:

#ifdef WIN32
    // provide posix handles for windows funcs :P
    inline HMODULE dlopen(LPCSTR lpFileName, int /*ignore*/)
    {
        return LoadLibraryA(lpFileName);
    }
    inline FARPROC dlsym(HMODULE hModule, LPCSTR lpProcName)
    {
        return GetProcAddress(hModule, lpProcName);
    }
    inline    BOOL dlclose(HMODULE hModule)
    {
        return FreeLibrary(hModule);
    }
    inline   DWORD dlerror()
    {
        return GetLastError();
    }

    HMODULE m_handle;
#else
    void * m_handle;
#endif
};
}


GuiPluginManager::GuiPluginManager()
{
}

GuiPluginManager::~GuiPluginManager()
{
    for (auto && it : m_libraries)
    {
        unloadLibrary(std::move(it));
    }
}

QStringList & GuiPluginManager::searchPaths()
{
    return m_searchPaths;
}

const QStringList & GuiPluginManager::searchPaths() const
{
    return m_searchPaths;
}

void GuiPluginManager::scan(GuiPluginInterface pluginInterface)
{
    for (auto && path : m_searchPaths)
    {
        auto entries = QDir(path).entryInfoList({ libFileNameFilter() }, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);

        for (auto && entry : entries)
        {
            loadLibrary(entry.filePath(), GuiPluginInterface(pluginInterface));
        }
    }
}

const std::vector<GuiPlugin *> & GuiPluginManager::plugins() const
{
    return m_plugins;
}

std::vector<GuiPluginLibrary *> GuiPluginManager::pluginLibraries() const
{
    std::vector<GuiPluginLibrary *> libs;

    for (auto && it : m_libraries)
    {
        libs.push_back(it.get());
    }

    return libs;
}

bool GuiPluginManager::loadLibrary(const QString & filePath, GuiPluginInterface && pluginInterface)
{
    // Check if library is already loaded and reload is not requested
    if (m_libraries.end() != std::find_if(m_libraries.begin(), m_libraries.end(),
        [&filePath] (const std::unique_ptr<GuiPluginLibrary> & lib) { return lib->filePath() == filePath; }))
    {
        return true;
    }

    auto library = std::make_unique<PluginLibraryImpl>(filePath);
    if (!library->isValid())
    {
        // Loading failed. Destroy library object and return failure.
        qDebug() << "Loading plugin from " << filePath << " failed.";

        return false;
    }

    library->initialize(std::forward<GuiPluginInterface>(pluginInterface));

    if (auto plugin = library->plugin())
    {
        m_plugins.push_back(plugin);
    }

    m_libraries.emplace_back(std::move(library));

    return true;
}

void GuiPluginManager::unloadLibrary(std::unique_ptr<GuiPluginLibrary> library)
{
    library->release();
}
