#include <gui/data_view/AbstractRenderView.h>

#include <cassert>

#include <QDebug>
#include <QLayout>
#include <QMouseEvent>
#include <QScreen>
#include <QWindow>

#include <vtkGenericOpenGLRenderWindow.h>

#include <core/AbstractVisualizedData.h>
#include <gui/data_view/RendererImplementation.h>
#include <gui/data_view/t_QVTKWidget.h>


AbstractRenderView::AbstractRenderView(DataMapping & dataMapping, int index, QWidget * parent, Qt::WindowFlags flags)
    : AbstractDataView(dataMapping, index, parent, flags)
    , m_qvtkWidget{ nullptr }
    , m_onShowInitialized{ false }
    , m_axesEnabled{ true }
    , m_activeSubViewIndex{ 0u }
{
    auto layout = new QBoxLayout(QBoxLayout::Direction::TopToBottom);
    layout->setMargin(0);
    layout->setSpacing(0);

    m_qvtkWidget = new t_QVTKWidget();
    m_qvtkWidget->setMinimumSize(300, 300);

#if defined (Q_VTK_WIDGET2_H)
    if (auto qvtkWidget2 = dynamic_cast<QVTKWidget2 *>(m_qvtkWidget))
    {
        auto renWin = qvtkWidget2->GetRenderWindow();
        renWin->SetMultiSamples(1);
        qvtkWidget2->setFormat(QVTKWidget2::GetDefaultVTKFormat(renWin));
    }
#endif

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

void AbstractRenderView::showEvent(QShowEvent * /*event*/)
{
    if (m_onShowInitialized)
    {
        return;
    }

    m_onShowInitialized = true;

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

void AbstractRenderView::visualizationSelectionChangedEvent(const VisualizationSelection & /*selection*/)
{
}

void AbstractRenderView::activeSubViewChangedEvent(unsigned int /*subViewIndex*/)
{
}
