#include "GlyphMappingChooser.h"
#include "ui_GlyphMappingChooser.h"
#include "GlyphMappingChooserListModel.h"

#include <algorithm>

#include <reflectionzeug/PropertyGroup.h>
#include <reflectionzeug/StringProperty.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData3D.h>
#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>
#include <core/utility/macros.h>
#include <core/utility/qthelper.h>

#include <gui/data_view/AbstractRenderView.h>
#include <gui/propertyguizeug_extension/ColorEditorRGB.h>


GlyphMappingChooser::GlyphMappingChooser(QWidget * parent, Qt::WindowFlags flags)
    : QDockWidget(parent, flags)
    , m_ui{ new Ui_GlyphMappingChooser() }
    , m_renderView{ nullptr }
    , m_mapping{ nullptr }
    , m_listModel{ new GlyphMappingChooserListModel() }
{
    m_ui->setupUi(this);
    m_ui->propertyBrowser->addEditorPlugin<ColorEditorRGB>();
    m_ui->propertyBrowser->addPainterPlugin<ColorEditorRGB>();

    m_listModel->setParent(m_ui->vectorsListView);
    m_ui->vectorsListView->setModel(m_listModel);

    updateTitle();

    connect(m_listModel, &GlyphMappingChooserListModel::glyphVisibilityChanged, this, &GlyphMappingChooser::renderSetupChanged);
    connect(m_listModel, &QAbstractItemModel::dataChanged,
        [this] (const QModelIndex &topLeft, const QModelIndex & /*bottomRight*/, const QVector<int> & /*roles*/) {
        m_ui->vectorsListView->setCurrentIndex(topLeft);
    });
    connect(m_ui->vectorsListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &GlyphMappingChooser::updateGuiForSelection);
}

GlyphMappingChooser::~GlyphMappingChooser()
{
    setCurrentRenderView(nullptr);
}

void GlyphMappingChooser::setCurrentRenderView(AbstractRenderView * renderView)
{
    if (m_renderView == renderView)
    {
        return;
    }

    disconnectAll(m_viewConnections);

    m_renderView = renderView;

    setSelectedData(m_renderView ? m_renderView->selection().dataObject : nullptr);

    if (m_renderView)
    {
        m_viewConnections.emplace_back(connect(renderView, &AbstractRenderView::beforeDeleteVisualizations,
            this, &GlyphMappingChooser::checkRemovedData));
        m_viewConnections.emplace_back(connect(m_renderView, &AbstractDataView::selectionChanged,
            [this] (AbstractDataView * DEBUG_ONLY(view), const DataSelection & selection)
        {
            assert(view == m_renderView);
            setSelectedData(selection.dataObject);
        }));
        m_viewConnections.emplace_back(connect(this, &GlyphMappingChooser::renderSetupChanged, m_renderView, &AbstractRenderView::render));
    }
}

void GlyphMappingChooser::setSelectedData(DataObject * dataObject)
{
    AbstractVisualizedData * currentVisualization = nullptr;

    if (m_renderView)
    {
        currentVisualization = m_renderView->visualizationFor(dataObject, m_renderView->activeSubViewIndex());
        if (!currentVisualization)
        {   // fall back to an object in any of the sub views
            currentVisualization = m_renderView->visualizationFor(dataObject);
        }
    }

    auto renderedData = dynamic_cast<RenderedData3D *>(currentVisualization);

    // An object was selected that is not contained in the current view or doesn't implement glyph mapping,
    // so stick to the current selection.
    if (dataObject && !renderedData)
    {
        return;
    }

    auto newMapping = renderedData ? &renderedData->glyphMapping() : nullptr;

    if (newMapping == m_mapping)
    {
        return;
    }

    disconnect(m_vectorListConnection);
    m_vectorListConnection = {};

    m_mapping = newMapping;

    updateTitle();
    updateVectorsList();

    if (m_mapping)
    {
        m_vectorListConnection = connect(m_mapping, &GlyphMapping::vectorsChanged, this, &GlyphMappingChooser::updateVectorsList);
    }
}

void GlyphMappingChooser::updateVectorsList()
{
    disconnectAll(m_vectorsRenderConnections);

    updateGuiForSelection({});

    m_ui->propertyBrowser->setRoot(nullptr);
    m_propertyGroups.clear();

    m_listModel->setMapping(m_mapping);

    if (m_mapping)
    {
        for (auto vectors : m_mapping->vectors())
        {
            m_propertyGroups.emplace_back(vectors->createPropertyGroup());
            m_vectorsRenderConnections.emplace_back(connect(vectors, &GlyphMappingData::geometryChanged, this, &GlyphMappingChooser::renderSetupChanged));
        }

        QModelIndex index(m_listModel->index(0, 0));
        m_ui->vectorsListView->selectionModel()->select(index, QItemSelectionModel::Select);
    }
}

void GlyphMappingChooser::checkRemovedData(const QList<AbstractVisualizedData *> & content)
{
    if (!m_mapping)
    {
        return;
    }

    auto it = std::find(content.begin(), content.end(), &m_mapping->renderedData());
    if (it != content.end())
    {
        setSelectedData(nullptr);
    }
}

void GlyphMappingChooser::updateGuiForSelection(const QItemSelection & selection)
{
    if (selection.indexes().isEmpty())
    {
        m_ui->propertyBrowser->setRoot(nullptr);
        m_ui->propertyBrowserContainer->setTitle("(No selection)");
    }
    else
    {
        const int index = selection.indexes().first().row();
        const auto vectorsName = m_mapping->vectorNames()[index];

        m_ui->propertyBrowser->setRoot(m_propertyGroups[index].get());
        m_ui->propertyBrowserContainer->setTitle(vectorsName);
    }
}

void GlyphMappingChooser::updateTitle()
{
    const QString title = m_mapping
        ? QString::number(m_renderView->index()) + ": " + m_mapping->renderedData().dataObject().name()
        : QString("(No object selected)");

    m_ui->relatedDataObject->setText(title);
}
