/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <utility>
#include <vector>

#include <vtkSmartPointer.h>

#include <core/types.h>

#include <gui/widgets/DockableWidget.h>


class QToolBar;

class DataMapping;
class DataSetHandler;


class GUI_API AbstractDataView : public DockableWidget
{
    Q_OBJECT

public:
    AbstractDataView(DataMapping & dataMapping, int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~AbstractDataView() override;

    DataMapping & dataMapping() const;
    DataSetHandler & dataSetHandler() const;
    int index() const;

    QToolBar * toolBar();
    bool toolBarIsVisible() const;
    void setToolBarVisible(bool visible);

    virtual bool isTable() const = 0;
    virtual bool isRenderer() const = 0;

    /** Human readable name of the data view. Defaults to "ID: list of contained data" */
    const QString & friendlyName();
    /** A title specifying contents of a sub-view */
    const QString & subViewFriendlyName(unsigned int subViewIndex);

    virtual unsigned int numberOfSubViews() const;
    unsigned int activeSubViewIndex() const;
    void setActiveSubView(unsigned int subViewIndex);

    /** highlight requested index */
    void setSelection(const DataSelection & selection);
    void clearSelection();
    const DataSelection & selection() const;

public:
    /** highlight this view as currently selected of its type */
    void setCurrent(bool isCurrent);

signals:
    /** signaled when the widget receive the keyboard focus (focusInEvent) */
    void focused(AbstractDataView * dataView);

    void activeSubViewChanged(unsigned int activeSubViewIndex);

    void selectionChanged(AbstractDataView * view, const DataSelection & selection);

protected:
    void updateTitle(const QString & message = {});

    virtual QWidget * contentWidget() = 0;

    void showEvent(QShowEvent * event) override;
    void focusInEvent(QFocusEvent * event) override;

    bool eventFilter(QObject * obj, QEvent * ev) override;

    virtual void activeSubViewChangedEvent(unsigned int subViewIndex);

    /** Handle selection changes in concrete sub-classes */
    virtual void onSetSelection(const DataSelection & selection) = 0;
    virtual void onClearSelection() = 0;

    /** Request reevaluation of friendlyName() contents.
      * By default, this is called when the list of contained data objects of the view changes. */
    void resetFriendlyName();
    /** Override in subclasses to change the friendly name string.
      * @return friendly name for the view and list of names for the sub-views. It is not required
      *     to specify sub-view names. They will default to empty strings. */
    virtual std::pair<QString, std::vector<QString>> friendlyNameInternal() const = 0;

private:
    DataMapping & m_dataMapping;
    const int m_index;
    bool m_initialized;

    QToolBar * m_toolBar;

    QString m_friendlyName;
    std::vector<QString> m_subViewFriendlyNames;

    unsigned int m_activeSubViewIndex;

    DataSelection m_selection;

private:
    Q_DISABLE_COPY(AbstractDataView)
};
