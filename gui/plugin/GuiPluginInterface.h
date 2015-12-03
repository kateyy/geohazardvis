#pragma once

#include <functional>

#include <QMap>
#include <QString>

#include <gui/gui_api.h>


class QAction;
class QDockWidget;
class QSettings;
class MainWindow;


class GUI_API GuiPluginInterface
{
public:
    explicit GuiPluginInterface(MainWindow & mainWindow, const QString & settingsFilePath);
    virtual ~GuiPluginInterface();

    /** Adds a dock widget to the applications main window and a associated main menu entry to show the widget. */
    void addWidget(QDockWidget * widget, const QString & mainMenuEntry);
    void removeWidget(QDockWidget * widget);

    void readSettings(const std::function<void(const QSettings & settings)> & func);
    void readWriteSettings(const std::function<void(QSettings & settings)> & func);

private:
    void removeWidget(QDockWidget * widget, QAction * action);

private:
    MainWindow * m_mainWindow;
    QString m_settingsFilePath;
    QMap<QDockWidget *, QAction *> m_widgets;
};
