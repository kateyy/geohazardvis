#include <gui/data_view/AbstractRenderView.h>

#include <cassert>

#include <QDebug>
#include <QLayout>
#include <QMouseEvent>

#include <QVTKWidget.h>

#include <gui/data_view/RendererImplementation.h>


AbstractRenderView::AbstractRenderView(int index, QWidget * parent, Qt::WindowFlags flags)
    : AbstractDataView(index, parent, flags)
    , m_qvtkWidget(nullptr)
    , m_axesEnabled(true)
    , m_activeSubViewIndex(0u)
{
    auto layout = new QBoxLayout(QBoxLayout::Direction::TopToBottom);
    layout->setMargin(0);
    layout->setSpacing(0);

    m_qvtkWidget = new QVTKWidget();
    m_qvtkWidget->setMinimumSize(300, 300);

    layout->addWidget(m_qvtkWidget);

    setLayout(layout);

    m_qvtkWidget->installEventFilter(this);
}

bool AbstractRenderView::isTable() const
{
    return false;
}

bool AbstractRenderView::isRenderer() const
{
    return true;
}

void AbstractRenderView::showDataObjects(
    const QList<DataObject *> & dataObjects,
    QList<DataObject *> & incompatibleObjects,
    unsigned int subViewIndex)
{
    assert(subViewIndex < numberOfSubViews());
    if (subViewIndex >= numberOfSubViews())
    {
        qDebug() << "Trying to AbstractRenderView::showDataObjects on sub-view" << subViewIndex << "while having only" << numberOfSubViews() << "views.";
        return;
    }

    showDataObjectsImpl(dataObjects, incompatibleObjects, subViewIndex);
}

void AbstractRenderView::hideDataObjects(const QList<DataObject *> & dataObjects, unsigned int subViewIndex)
{
    assert(subViewIndex < numberOfSubViews());
    if (subViewIndex >= numberOfSubViews())
    {
        qDebug() << "Trying to AbstractRenderView::hideDataObjects on sub-view" << subViewIndex << "while having only" << numberOfSubViews() << "views.";
        return;
    }

    hideDataObjectsImpl(dataObjects, subViewIndex);
}

bool AbstractRenderView::contains(DataObject * dataObject, int subViewIndexOrAll) const
{
    // optimize as needed (cache dataObjects() results...)
    return dataObjects(subViewIndexOrAll).contains(dataObject);
}

void AbstractRenderView::prepareDeleteData(const QList<DataObject *> & dataObjects)
{
    prepareDeleteDataImpl(dataObjects);
}

QList<DataObject *> AbstractRenderView::dataObjects(int subViewIndexOrAll) const
{
    assert(subViewIndexOrAll >= -1); // -1 means all view, all smaller numbers are invalid
    if (subViewIndexOrAll >= int(numberOfSubViews()))
    {
        qDebug() << "Trying to AbstractRenderView::dataObjects on sub-view" << subViewIndexOrAll << "while having only" << numberOfSubViews() << "views.";
        return{};
    }

    return dataObjectsImpl(subViewIndexOrAll);
}

QList<AbstractVisualizedData *> AbstractRenderView::visualizations(int subViewIndex) const
{
    assert(subViewIndex == -1 || (unsigned int)(subViewIndex) < numberOfSubViews());
    return visualizationsImpl(subViewIndex);
}

unsigned int AbstractRenderView::numberOfSubViews() const
{
    return 1;
}

unsigned int AbstractRenderView::activeSubViewIndex() const
{
    return m_activeSubViewIndex;
}

void AbstractRenderView::setActiveSubView(unsigned int subViewIndex)
{
    assert(subViewIndex < numberOfSubViews());
    if (subViewIndex >= numberOfSubViews())
        return;

    if (m_activeSubViewIndex == subViewIndex)
        return;

    m_activeSubViewIndex = subViewIndex;

    activeSubViewChangedEvent(m_activeSubViewIndex);

    emit activeSubViewChanged(m_activeSubViewIndex);
}

vtkRenderWindow * AbstractRenderView::renderWindow()
{
    return qvtkWidget().GetRenderWindow();
}

const vtkRenderWindow * AbstractRenderView::renderWindow() const
{
    assert(m_qvtkWidget);
    return m_qvtkWidget->GetRenderWindow();
}

void AbstractRenderView::setEnableAxes(bool enabled)
{
    if (m_axesEnabled == enabled)
        return;

    m_axesEnabled = enabled;

    axesEnabledChangedEvent(enabled);
}

bool AbstractRenderView::axesEnabled() const
{
    return m_axesEnabled;
}

void AbstractRenderView::render()
{
    implementation().render();
}

void AbstractRenderView::showInfoText(const QString & info)
{
    qvtkWidget().setToolTip(info);
}

QString AbstractRenderView::infoText() const
{
    return toolTip();
}

QWidget * AbstractRenderView::contentWidget()
{
    assert(m_qvtkWidget);
    return m_qvtkWidget;
}

QVTKWidget & AbstractRenderView::qvtkWidget()
{
    assert(m_qvtkWidget);
    return *m_qvtkWidget;
}

bool AbstractRenderView::eventFilter(QObject * watched, QEvent * event)
{
    if (event->type() != QEvent::MouseButtonPress || watched != contentWidget())
        return AbstractDataView::eventFilter(watched, event);

    auto mouseEvent = static_cast<QMouseEvent *>(event);
    setActiveSubView(implementation().subViewIndexAtPos(mouseEvent->pos()));

    return AbstractDataView::eventFilter(watched, event);
}

void AbstractRenderView::selectionChangedEvent(DataObject * dataObject, vtkIdTypeArray * selection, IndexType indexType)
{
    assert((dataObject != nullptr) == (selection != nullptr));

    AbstractVisualizedData * vis = nullptr;
    if (!dataObject || !(vis = visualizationFor(dataObject)))
    {
        implementation().clearSelection();
        return;
    }

    implementation().setSelectedData(vis, *selection, indexType);
}

void AbstractRenderView::activeSubViewChangedEvent(unsigned int /*subViewIndex*/)
{
}
