#pragma once

#include <functional>

#include <QList>

#include <gui/gui_api.h>


class QDockWidget;
class QSettings;
class QString;
class DataSetHandler;
class DataMapping;
class MainWindow;


class GUI_API GuiPluginInterface
{
public:
    explicit GuiPluginInterface(
        MainWindow & mainWindow,
        DataMapping & dataMapping);
    virtual ~GuiPluginInterface();

    /** Adds a dock widget to the applications main window and a associated main menu entry to show the widget. */
    void addWidget(QDockWidget * widget);
    void removeWidget(QDockWidget * widget);

    /** Read/Write access to plugin settings stored with user defined application settings.
      * Such settings should always be accessed via these function, to ensure some consistency. */

    void readSettings(const std::function<void(const QSettings & settings)> & func);
    void readSettings(const QString & group, const std::function<void(const QSettings & settings)> & func);
    void readWriteSettings(const std::function<void(QSettings & settings)> & func);
    void readWriteSettings(const QString & group, const std::function<void(QSettings & settings)> & func);

    DataSetHandler & dataSetHandler() const;
    DataMapping & dataMapping() const;

    GuiPluginInterface(const GuiPluginInterface & other);
    friend void swap(GuiPluginInterface & lhs, GuiPluginInterface & rhs);
    GuiPluginInterface & operator=(GuiPluginInterface other);
    GuiPluginInterface(GuiPluginInterface && other);

private:
    GuiPluginInterface();

private:
    MainWindow * m_mainWindow;
    DataMapping * m_dataMapping;
    QList<QDockWidget *> m_widgets;
};
