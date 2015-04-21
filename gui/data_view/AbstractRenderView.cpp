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

void AbstractRenderView::addDataObjects(
    const QList<DataObject *> & dataObjects,
    QList<DataObject *> & incompatibleObjects,
    unsigned int subViewIndex)
{
    assert(subViewIndex < numberOfSubViews());
    if (subViewIndex >= numberOfSubViews())
    {
        qDebug() << "Trying to AbstractRenderView::addDataObjects on sub-view" << subViewIndex << "while having only" << numberOfSubViews() << "views.";
        return;
    }

    if (m_dataObjects.size() <= subViewIndex)
        m_dataObjects.resize(subViewIndex + 1);

    auto && subViewObjects = m_dataObjects.at(subViewIndex);

    assert((subViewObjects.toSet() & dataObjects.toSet()).isEmpty());

    addDataObjectsImpl(dataObjects, incompatibleObjects, subViewIndex);

    subViewObjects.append((dataObjects.toSet() - incompatibleObjects.toSet()).toList());

    assert(subViewObjects.size() == subViewObjects.toSet().size());
}

void AbstractRenderView::hideDataObjects(const QList<DataObject *> & dataObjects, unsigned int subViewIndex)
{
    assert(subViewIndex < numberOfSubViews());
    if (subViewIndex >= numberOfSubViews())
    {
        qDebug() << "Trying to AbstractRenderView::hideDataObjects on sub-view" << subViewIndex << "while having only" << numberOfSubViews() << "views.";
        return;
    }

    assert(m_dataObjects.size() > subViewIndex);

    auto && subViewObjects = m_dataObjects.at(subViewIndex);

    assert((subViewObjects.toSet() & dataObjects.toSet()) == dataObjects.toSet());

    hideDataObjectsImpl(dataObjects, subViewIndex);

    for (const auto & data : dataObjects)
        subViewObjects.removeOne(data);

    assert((subViewObjects.toSet() & dataObjects.toSet()).isEmpty());
}

bool AbstractRenderView::contains(DataObject * dataObject, int subViewIndexOrAll) const
{
    if (subViewIndexOrAll == -1)
    {
        for (const auto & list : m_dataObjects)
            if (list.contains(dataObject))
                return true;

        return false;
    }

    assert(subViewIndexOrAll >= 0);
    auto subViewIndex = static_cast<unsigned int>(subViewIndexOrAll);

    assert(subViewIndex < numberOfSubViews());
    if (subViewIndex >= numberOfSubViews())
    {
        qDebug() << "Trying to AbstractRenderView::contains on sub-view" << subViewIndex << "while having only" << numberOfSubViews() << "views.";
        return false;
    }

    return m_dataObjects.at(subViewIndex).contains(dataObject);
}

void AbstractRenderView::removeDataObjects(const QList<DataObject *> & dataObjects)
{
    removeDataObjectsImpl(dataObjects);

    for (auto && list : m_dataObjects)
    {
        for (auto * data : dataObjects)
            list.removeOne(data);
    }

#ifdef _DEBUG
    QSet<DataObject *> allObjects;
    for (auto && list : m_dataObjects)
        allObjects += list.toSet();
    assert((allObjects & dataObjects.toSet()).isEmpty());
#endif
}

QList<DataObject *> AbstractRenderView::dataObjects(int subViewIndexOrAll) const
{
    if (subViewIndexOrAll == -1)
    {
        QSet<DataObject *> allObjects;
        for (const auto & list : m_dataObjects)
            allObjects += list.toSet();

        return allObjects.toList();
    }

    assert(subViewIndexOrAll >= 0);
    auto subViewIndex = static_cast<unsigned int>(subViewIndexOrAll);

    assert(subViewIndex < numberOfSubViews());
    if (subViewIndex >= numberOfSubViews())
    {
        qDebug() << "Trying to AbstractRenderView::dataObjects on sub-view" << subViewIndex << "while having only" << numberOfSubViews() << "views.";
        return{};
    }

    return m_dataObjects.at(subViewIndex);
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
