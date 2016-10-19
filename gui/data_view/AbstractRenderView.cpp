#include <gui/data_view/AbstractRenderView.h>

#include <cassert>

#include <QDebug>
#include <QLayout>
#include <QMouseEvent>
#include <QScreen>
#include <QWindow>

#include <vtkGenericOpenGLRenderWindow.h>

#include <core/AbstractVisualizedData.h>
#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <gui/data_view/RendererImplementation.h>
#include <gui/data_view/t_QVTKWidget.h>


AbstractRenderView::AbstractRenderView(DataMapping & dataMapping, int index, QWidget * parent, Qt::WindowFlags flags)
    : AbstractDataView(dataMapping, index, parent, flags)
    , m_qvtkWidget{ nullptr }
    , m_onFirstPaintInitialized{ false }
    , m_coordSystem{}
    , m_axesEnabled{ true }
    , m_activeSubViewIndex{ 0u }
{
    auto layout = new QBoxLayout(QBoxLayout::Direction::TopToBottom);
    layout->setMargin(0);
    layout->setSpacing(0);

    m_qvtkWidget = new t_QVTKWidget();
    m_qvtkWidget->setMinimumSize(300, 300);
    layout->addWidget(m_qvtkWidget);

    setLayout(layout);

    m_qvtkWidget->installEventFilter(this);
}

AbstractRenderView::~AbstractRenderView() = default;

bool AbstractRenderView::isTable() const
{
    return false;
}

bool AbstractRenderView::isRenderer() const
{
    return true;
}

const CoordinateSystemSpecification & AbstractRenderView::currentCoordinateSystem() const
{
    return m_coordSystem;
}

bool AbstractRenderView::setCurrentCoordinateSystem(const CoordinateSystemSpecification & spec)
{
    if (m_coordSystem == spec)
    {
        return true;
    }

    if (spec.type != CoordinateSystemType::unspecified && !spec.isValid(true))
    {
        return false;
    }

    // If the coordinate system is specified, only allow to switch if all data objects support the transformation.
    // Setting it to CoordinateSystemType::unspecified is okay, if this user insists in setting this.
    // (this results in a pass-through)
    if (spec.isValid(false) && !canShowDataObjectInCoordinateSystem(spec))
    {
        return false;
    }

    onCoordinateSystemChanged(spec);

    m_coordSystem = spec;

    emit currentCoordinateSystemChanged(m_coordSystem);

    return true;
}

bool AbstractRenderView::canShowDataObjectInCoordinateSystem(const CoordinateSystemSpecification & spec)
{
    auto contents = visualizations();

    if (contents.isEmpty())
    {
        return true;
    }

    QList<CoordinateTransformableDataObject *> transformableObjects;

    for (auto visualization : contents)
    {
        if (auto transf = dynamic_cast<CoordinateTransformableDataObject *>(&visualization->dataObject()))
        {
            transformableObjects.push_back(transf);
        }
    }

    if (spec.type == CoordinateSystemType::unspecified || transformableObjects.isEmpty())
    {
        return (spec.type == CoordinateSystemType::unspecified) 
            == transformableObjects.isEmpty();
    }

    if (transformableObjects.size() != contents.size())
    {
        qDebug() << "Unexpectedly rendering transformable and non-transformable objects.";
        return false;
    }

    for (auto dataObject : transformableObjects)
    {
        if (!dataObject->canTransformTo(spec))
        {
            return false;
        }
    }

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

    QList<CoordinateTransformableDataObject *> transformableObjects;

    bool hasTransformables = false;
    for (auto dataObject : dataObjects)
    {
        auto transformable = dynamic_cast<CoordinateTransformableDataObject *>(dataObject);
        transformableObjects << transformable;
        if (transformable && transformable->coordinateSystem().isValid(false))
        {
            hasTransformables = true;
        }
    }

    if (visualizations().isEmpty())
    {
        setCurrentCoordinateSystem({});
    }

    QList<DataObject *> possibleCompatibleObjects;
    bool haveValidSystem = currentCoordinateSystem().isValid(false);

    // Once there is a valid coordinate system specification, don't allow to mix incompatible
    // representations
    if (hasTransformables || haveValidSystem)
    {
        for (int inIdx = 0; inIdx < dataObjects.size(); ++inIdx)
        {
            auto transformable = transformableObjects[inIdx];
            if (!transformable)
            {
                incompatibleObjects << dataObjects[inIdx];
                continue;
            }

            if (!haveValidSystem)
            {
                // set the coordinate system to the system of the first loaded object
                setCurrentCoordinateSystem(transformable->coordinateSystem());
                haveValidSystem = true;
                possibleCompatibleObjects << dataObjects[inIdx];
                continue;
            }

            if (!transformable->canTransformTo(currentCoordinateSystem()))
            {
                incompatibleObjects << dataObjects[inIdx];
                continue;
            }

            possibleCompatibleObjects << dataObjects[inIdx];
        }
    }
    else
    {
        possibleCompatibleObjects = dataObjects;
    }

    initializeForFirstPaint();

    showDataObjectsImpl(possibleCompatibleObjects, incompatibleObjects, subViewIndex);
}

void AbstractRenderView::hideDataObjects(const QList<DataObject *> & dataObjects, int subViewIndex)
{
    assert(subViewIndex < 0 || static_cast<unsigned>(subViewIndex) < numberOfSubViews());
    if (subViewIndex >= 0 && static_cast<unsigned>(subViewIndex) >= numberOfSubViews())
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

void AbstractRenderView::setVisualizationSelection(const VisualizationSelection & selection)
{
    if (selection == visualzationSelection())
    {
        return;
    }

    implementation().setSelection(selection);

    // Trigger update according to the more generic AbstractDataView interface.
    // This doesn't contain information regarding the actual visualization.
    setSelection(DataSelection(selection));

    visualizationSelectionChangedEvent(implementation().selection());

    emit visualizationSelectionChanged(this, implementation().selection());
}

const VisualizationSelection & AbstractRenderView::visualzationSelection() const
{
    return implementation().selection();
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
    {
        return;
    }

    if (m_activeSubViewIndex == subViewIndex)
    {
        return;
    }

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
    {
        return;
    }

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

void AbstractRenderView::setInfoTextCallback(std::function<QString()> callback)
{
    disconnect(m_infoTextConnection);
    m_infoTextCallback = callback;

    if (m_infoTextCallback)
    {
        m_infoTextConnection = connect(&qvtkWidget(), &t_QVTKWidget::beforeTooltipPopup, 
            [this] ()
        {
            showInfoText(m_infoTextCallback());
        });
    }
}

QWidget * AbstractRenderView::contentWidget()
{
    assert(m_qvtkWidget);
    return m_qvtkWidget;
}

t_QVTKWidget & AbstractRenderView::qvtkWidget()
{
    assert(m_qvtkWidget);
    return *m_qvtkWidget;
}

void AbstractRenderView::showEvent(QShowEvent * event)
{
    AbstractDataView::showEvent(event);

    initializeForFirstPaint();
}

void AbstractRenderView::paintEvent(QPaintEvent * event)
{
    AbstractDataView::paintEvent(event);

    initializeForFirstPaint();
}

bool AbstractRenderView::eventFilter(QObject * watched, QEvent * event)
{
    if (event->type() != QEvent::MouseButtonPress || watched != contentWidget())
        return AbstractDataView::eventFilter(watched, event);

    auto mouseEvent = static_cast<QMouseEvent *>(event);
    setActiveSubView(implementation().subViewIndexAtPos(mouseEvent->pos()));

    return AbstractDataView::eventFilter(watched, event);
}

void AbstractRenderView::onSetSelection(const DataSelection & selection)
{
    assert(selection.dataObject && dataObjects().contains(selection.dataObject));

    const auto vis = visualizationFor(selection.dataObject);
    assert(vis);

    auto && newVisSelection = VisualizationSelection(
        selection,
        vis,
        vis->defaultVisualizationPort()
    );

    setVisualizationSelection(newVisSelection);
}

void AbstractRenderView::onClearSelection()
{
    implementation().clearSelection();

    visualizationSelectionChangedEvent(implementation().selection());

    emit visualizationSelectionChanged(this, implementation().selection());
}

void AbstractRenderView::onCoordinateSystemChanged(const CoordinateSystemSpecification & spec)
{
    implementation().applyCurrentCoordinateSystem(spec);
}

void AbstractRenderView::visualizationSelectionChangedEvent(const VisualizationSelection & /*selection*/)
{
}

void AbstractRenderView::activeSubViewChangedEvent(unsigned int /*subViewIndex*/)
{
}

void AbstractRenderView::initializeForFirstPaint()
{
    if (m_onFirstPaintInitialized)
    {
        return;
    }

    m_onFirstPaintInitialized = true;

    // let the subclass create a context first
    initializeRenderContext();

    auto updateRenWinDpi = [this] () -> void
    {
        auto renWin = renderWindow();
        auto nativeParent = nativeParentWidget();
        if (!renWin || !nativeParent)
        {
            return;
        }

        const auto dpi = static_cast<int>(nativeParent->windowHandle()->screen()->logicalDotsPerInch());
        if (dpi != renWin->GetDPI())
        {
            renWin->SetDPI(dpi);
        }
    };

    if (auto nativeParent = nativeParentWidget())
    {
        connect(nativeParent->windowHandle(), &QWindow::screenChanged, updateRenWinDpi);
    }

    updateRenWinDpi();
}
