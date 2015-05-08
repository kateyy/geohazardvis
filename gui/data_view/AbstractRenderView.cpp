#include <gui/data_view/AbstractRenderView.h>

#include <cassert>

#include <QDebug>


AbstractRenderView::AbstractRenderView(int index, QWidget * parent, Qt::WindowFlags flags)
    : AbstractDataView(index, parent, flags)
    , m_axesEnabled(true)
{
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

void AbstractRenderView::ShowInfo(const QStringList & info)
{
    setToolTip(info.join('\n'));
}
