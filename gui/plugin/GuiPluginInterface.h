#pragma once

#include <functional>
#include <map>
#include <memory>

#include <QAction>
#include <QString>

#include <gui/gui_api.h>


class QAction;
class QDockWidget;
class QSettings;
class DataSetHandler;
class DataMapping;
class MainWindow;


class GUI_API GuiPluginInterface
{
public:
    explicit GuiPluginInterface(
        MainWindow & mainWindow, 
        const QString & settingsFilePath,
        DataMapping & dataMapping);
    virtual ~GuiPluginInterface();

    /** Adds a dock widget to the applications main window and a associated main menu entry to show the widget. */
    void addWidget(QDockWidget * widget, const QString & mainMenuEntry);
    void removeWidget(QDockWidget * widget);

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

    void removeWidget(QDockWidget * widget, QAction * action);

private:
    MainWindow * m_mainWindow;
    QString m_settingsFilePath;
    DataMapping * m_dataMapping;
    std::map<QDockWidget *, std::unique_ptr<QAction>> m_widgets;
};
