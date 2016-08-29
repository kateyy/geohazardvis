#pragma once

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

    void updateTitle(const QString & message = {});

    QToolBar * toolBar();
    bool toolBarIsVisible() const;
    void setToolBarVisible(bool visible);

    virtual bool isTable() const = 0;
    virtual bool isRenderer() const = 0;

    /** name the id and represented data */
    virtual QString friendlyName() const = 0;
    /** A title specifying contents of a sub-view */
    virtual QString subViewFriendlyName(unsigned int subViewIndex) const;

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

    void selectionChanged(AbstractDataView * view, const DataSelection & selection);

protected:
    virtual QWidget * contentWidget() = 0;

    void showEvent(QShowEvent * event) override;
    void focusInEvent(QFocusEvent * event) override;

    bool eventFilter(QObject * obj, QEvent * ev) override;

    /** Handle selection changes in concrete sub-classes */
    virtual void onSetSelection(const DataSelection & selection) = 0;
    virtual void onClearSelection() = 0;

private:
    DataMapping & m_dataMapping;
    const int m_index;
    bool m_initialized;

    QToolBar * m_toolBar;

    DataSelection m_selection;
};
