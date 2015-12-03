#pragma once

#include <utility>

#include <gui/plugin/GuiPlugin.h>


class GuiPluginInterface;


#ifdef _MSC_VER
#   define GUI_PLUGIN_API __declspec(dllexport)
#elif __GNUC__
#   define GUI_PLUGIN_API __attribute__ ((visibility ("default")))
#else
#   define GUI_PLUGIN_API
#endif

#define GUI_PLUGIN_DEFINE(CLASS, NAME, DESCRIPTION, VENDOR, VERSION) \
    GuiPlugin * g_plugin = nullptr; \
    \
    extern "C" GUI_PLUGIN_API void initialize(GuiPluginInterface && pluginInterface) \
    { \
        g_plugin = new CLASS(NAME, DESCRIPTION, VENDOR, VERSION, std::move(pluginInterface)); \
    } \
    \
    extern "C" GUI_PLUGIN_API void release() \
    { \
        delete g_plugin; \
    } \
    \
    extern "C" GUI_PLUGIN_API GuiPlugin * plugin() \
    { \
        return g_plugin; \
    }
