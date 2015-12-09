#include "GuiPluginInterface.h"

#include <cassert>

#include <QAction>
#include <QDockWidget>
#include <QMenuBar>
#include <QSettings>

#include <gui/DataMapping.h>
#include <gui/MainWindow.h>


GuiPluginInterface::GuiPluginInterface(MainWindow & mainWindow, const QString & settingsFilePath, DataMapping & dataMapping)
    : m_mainWindow(&mainWindow)
    , m_settingsFilePath(settingsFilePath)
    , m_dataMapping(&dataMapping)
{
}

GuiPluginInterface::~GuiPluginInterface()
{
    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it)
    {
        removeWidget(it->first, it->second.get());
    }
}

void GuiPluginInterface::addWidget(QDockWidget * widget, const QString & mainMenuEntry)
{
    assert(m_widgets.find(widget) == m_widgets.end());
    if (m_widgets.find(widget) != m_widgets.end())
    {
        return;
    }

    m_mainWindow->addDockWidget(Qt::RightDockWidgetArea, widget);
    widget->hide();

    auto widgetShowAction = std::make_unique<QAction>(mainMenuEntry, m_mainWindow);
    widgetShowAction->setCheckable(true);
    QObject::connect(widgetShowAction.get(), &QAction::triggered, widget, &QDockWidget::setVisible);

    // Insert the action to the end of the menu bar, before the "Help" menu
    m_mainWindow->menuBar()->insertAction(*(m_mainWindow->menuBar()->actions().end() - 1), widgetShowAction.get());

    m_widgets.emplace(widget, std::move(widgetShowAction));
}

void GuiPluginInterface::removeWidget(QDockWidget * widget)
{
    auto it = m_widgets.find(widget);

    if (it == m_widgets.end())
    {
        assert(false);
        return;
    }

    removeWidget(widget, it->second.get());

    m_widgets.erase(it);
}

void GuiPluginInterface::readSettings(const std::function<void(const QSettings & settings)> & func)
{
    QSettings settings(m_settingsFilePath, QSettings::IniFormat);

    func(settings);
}

void GuiPluginInterface::readSettings(const QString & group, const std::function<void(const QSettings & settings)> & func)
{
    QSettings settings(m_settingsFilePath, QSettings::IniFormat);
    settings.beginGroup(group);

    func(settings);
}

void GuiPluginInterface::readWriteSettings(const std::function<void(QSettings & settings)> & func)
{
    QSettings settings(m_settingsFilePath, QSettings::IniFormat);

    func(settings);
}

void GuiPluginInterface::readWriteSettings(const QString & group, const std::function<void(QSettings & settings)> & func)
{
    QSettings settings(m_settingsFilePath, QSettings::IniFormat);
    settings.beginGroup(group);

    func(settings);
}

DataSetHandler & GuiPluginInterface::dataSetHandler() const
{
    return m_dataMapping->dataSetHandler();
}

DataMapping & GuiPluginInterface::dataMapping() const
{
    return *m_dataMapping;
}

void GuiPluginInterface::updateActionCheckStates()
{
    for (auto && it : m_widgets)
    {
        it.second->blockSignals(true);
        it.second->setChecked(it.first->isVisible());
        it.second->blockSignals(false);
    }
}

void GuiPluginInterface::removeWidget(QDockWidget * widget, QAction * action)
{
    m_mainWindow->menuBar()->removeAction(action);

    // removeDockWidget creates a placeholder item, which is identified by the widget's object name
    // The object name (QString) might be set by the ui-file in the plugin's resources. After unloading
    // the plugin, this QString object is not available anymore, which leads to invalid memory access.
    // [Anyway, why is this not handled correctly by Qt?]
    widget->setObjectName(QString());
    m_mainWindow->removeDockWidget(widget);
}

GuiPluginInterface::GuiPluginInterface(const GuiPluginInterface & other)
    : m_mainWindow(other.m_mainWindow)
    , m_settingsFilePath(other.m_settingsFilePath)
    , m_dataMapping(other.m_dataMapping)
{
    assert(m_widgets.empty() && other.m_widgets.empty());
}

void swap(GuiPluginInterface & lhs, GuiPluginInterface & rhs)
{
    // https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom

    using std::swap;

    swap(lhs.m_mainWindow, rhs.m_mainWindow);
    swap(lhs.m_settingsFilePath, rhs.m_settingsFilePath);
    swap(lhs.m_dataMapping, rhs.m_dataMapping);
    assert(lhs.m_widgets.empty() && rhs.m_widgets.empty());
}

GuiPluginInterface::GuiPluginInterface()
    : m_mainWindow(nullptr)
    , m_dataMapping(nullptr)
{
}

GuiPluginInterface & GuiPluginInterface::operator=(GuiPluginInterface other)
{
    swap(*this, other);

    return *this;
}

GuiPluginInterface::GuiPluginInterface(GuiPluginInterface && other)
{
    swap(*this, other);
}