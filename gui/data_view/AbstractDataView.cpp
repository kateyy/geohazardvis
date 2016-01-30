#include "AbstractDataView.h"

#include <cassert>

#include <QDockWidget>
#include <QEvent>
#include <QLayout>
#include <QToolBar>

#include <vtkIdTypeArray.h>

#include <gui/DataMapping.h>


AbstractDataView::AbstractDataView(
    DataMapping & dataMapping, int index, QWidget * parent, Qt::WindowFlags flags)
    : DockableWidget(parent, flags)
    , m_dataMapping{ dataMapping }
    , m_index{ index }
    , m_initialized{ false }
    , m_toolBar{ nullptr }
    , m_selectedDataObject{ nullptr }
    , m_selectedIndices{ vtkSmartPointer<vtkIdTypeArray>::New() }
{
}

AbstractDataView::~AbstractDataView() = default;

DataMapping & AbstractDataView::dataMapping() const
{
    return m_dataMapping;
}

DataSetHandler & AbstractDataView::dataSetHandler() const
{
    return m_dataMapping.dataSetHandler();
}

int AbstractDataView::index() const
{
    return m_index;
}

void AbstractDataView::updateTitle(QString message)
{
    QString title;

    if (message.isEmpty())
        title = friendlyName();
    else
        title = QString::number(index()) + ": " + message;

    setWindowTitle(title);
    if (hasDockWidgetParent())
        dockWidgetParent()->setWindowTitle(title);
}

QToolBar * AbstractDataView::toolBar()
{
    if (m_toolBar)
        return m_toolBar;

    m_toolBar = new QToolBar();
    auto font = m_toolBar->font();
    font.setBold(false);
    m_toolBar->setFont(font);
    m_toolBar->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);

    layout()->addWidget(m_toolBar);

    return m_toolBar;
}

bool AbstractDataView::toolBarIsVisible() const
{
    if (!m_toolBar)
        return false;

    return m_toolBar->isVisible();
}

void AbstractDataView::setToolBarVisible(bool visible)
{
    if (!visible && !m_toolBar)
        return;

    toolBar()->setVisible(visible);
}

QString AbstractDataView::subViewFriendlyName(unsigned int /*subViewIndex*/) const
{
    return "";
}

void AbstractDataView::setSelection(DataObject * dataObject, vtkIdType selectionIndex, IndexType indexType)
{
    bool oldSingleSelection = m_selectedIndices->GetSize() == 1;
    auto singleSelection = oldSingleSelection ? m_selectedIndices->GetValue(0) : vtkIdType(-1);

    if (m_selectedDataObject == dataObject && singleSelection == selectionIndex && m_selectionIndexType == indexType)
        return;

    m_selectedDataObject = dataObject;
    m_selectedIndices->SetNumberOfValues(1);
    m_selectedIndices->SetValue(0, selectionIndex);
    m_selectionIndexType = indexType;

    selectionChangedEvent(dataObject, m_selectedIndices, m_selectionIndexType);
}

void AbstractDataView::setSelection(DataObject * dataObject, vtkIdTypeArray & selectionIndices, IndexType indexType)
{
    bool changed = 
        m_selectedDataObject != dataObject
        || m_selectionIndexType != indexType
        || m_selectedIndices->GetSize() != selectionIndices.GetSize();

    assert(selectionIndices.GetNumberOfComponents() == 1);
    const auto numIndices = selectionIndices.GetNumberOfTuples();
    m_selectedIndices->SetNumberOfValues(numIndices);

    for (vtkIdType i = 0; i < numIndices; ++i)
    {
        auto newValue = selectionIndices.GetValue(i);
        auto storedValuePtr = m_selectedIndices->GetPointer(i);
        changed = changed || (*storedValuePtr != newValue);
        *storedValuePtr = newValue;
    }

    if (!changed)
        return;

    m_selectedDataObject = dataObject;
    m_selectionIndexType = indexType;

    selectionChangedEvent(dataObject, m_selectedIndices, m_selectionIndexType);
}

void AbstractDataView::clearSelection()
{
    auto empty = vtkSmartPointer<vtkIdTypeArray>::New();
    setSelection(nullptr, *empty, m_selectionIndexType);
}

vtkIdType AbstractDataView::lastSelectedIndex() const
{
    auto numSelections = m_selectedIndices->GetSize();

    if (numSelections == 0)
        return -1;

    return m_selectedIndices->GetValue(numSelections - 1);
}

vtkIdTypeArray * AbstractDataView::selectedIndices()
{
    return m_selectedIndices;
}

IndexType AbstractDataView::selectedIndexType() const
{
    return m_selectionIndexType;
}

DataObject * AbstractDataView::selectedDataObject()
{
    return m_selectedDataObject;
}

const DataObject * AbstractDataView::selectedDataObject() const
{
    return m_selectedDataObject;
}

void AbstractDataView::showEvent(QShowEvent * /*event*/)
{
    if (m_initialized)
        return;

    contentWidget()->installEventFilter(this);

    m_initialized = true;
}

void AbstractDataView::focusInEvent(QFocusEvent * /*event*/)
{
    emit focused(this);
}

void AbstractDataView::setCurrent(bool isCurrent)
{
    auto mainWidget = hasDockWidgetParent() ? static_cast<QWidget *>(dockWidgetParent()) : this;
    auto f = mainWidget->font();
    f.setBold(isCurrent);
    mainWidget->setFont(f);
}

bool AbstractDataView::eventFilter(QObject * obj, QEvent * ev)
{
    DockableWidget::eventFilter(obj, ev);

    if (ev->type() == QEvent::FocusIn)
        emit focused(this);

    return false;
}
