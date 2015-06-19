#include "GlyphMappingChooser.h"
#include "ui_GlyphMappingChooser.h"

#include <reflectionzeug/PropertyGroup.h>
#include <reflectionzeug/StringProperty.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData3D.h>
#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/propertyguizeug_extension/ColorEditorRGB.h>

#include "GlyphMappingChooserListModel.h"


GlyphMappingChooser::GlyphMappingChooser(QWidget * parent, Qt::WindowFlags flags)
    : QDockWidget(parent, flags)
    , m_ui(new Ui_GlyphMappingChooser())
    , m_renderView(nullptr)
    , m_mapping(nullptr)
    , m_listModel(new GlyphMappingChooserListModel())
{
    m_ui->setupUi(this);
    m_ui->propertyBrowser->addEditorPlugin<ColorEditorRGB>();
    m_ui->propertyBrowser->addPainterPlugin<ColorEditorRGB>();

    m_listModel->setParent(m_ui->vectorsListView);
    m_ui->vectorsListView->setModel(m_listModel);

    connect(m_listModel, &GlyphMappingChooserListModel::glyphVisibilityChanged, this, &GlyphMappingChooser::renderSetupChanged);
    connect(m_listModel, &QAbstractItemModel::dataChanged,
        [this] (const QModelIndex &topLeft, const QModelIndex & /*bottomRight*/, const QVector<int> & /*roles*/) {
        m_ui->vectorsListView->setCurrentIndex(topLeft);
    });
    connect(m_ui->vectorsListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &GlyphMappingChooser::updateGuiForSelection);
}

GlyphMappingChooser::~GlyphMappingChooser()
{
    m_renderView = nullptr;
    m_mapping = nullptr;

    updateVectorsList();

    delete m_ui;
}

void GlyphMappingChooser::setCurrentRenderView(AbstractRenderView * renderView)
{
    GlyphMapping * newMapping = nullptr;
    if (renderView && renderView->selectedDataVisualization())
        if (RenderedData3D * new3D = dynamic_cast<RenderedData3D *>(renderView->selectedDataVisualization()))
            newMapping = &new3D->glyphMapping();

    if (m_renderView)
    {
        disconnect(this, &GlyphMappingChooser::renderSetupChanged, m_renderView, &AbstractRenderView::render);
        disconnect(m_renderView, &AbstractRenderView::beforeDeleteVisualization, 
            this, &GlyphMappingChooser::checkRemovedData);
    }
    if (m_mapping)
        disconnect(m_mapping, &GlyphMapping::vectorsChanged, this, &GlyphMappingChooser::updateVectorsList);

    m_renderView = renderView;
    m_mapping = newMapping;

    if (renderView)
    {
        connect(renderView, &AbstractRenderView::beforeDeleteVisualization,
            this, &GlyphMappingChooser::checkDeletedContent);
        connect(m_renderView, &AbstractRenderView::selectedDataChanged,
            this, static_cast<void (GlyphMappingChooser::*)(AbstractRenderView *, DataObject *)>(&GlyphMappingChooser::setSelectedData));
    }

    updateTitle();

    updateVectorsList();

    if (m_mapping)
        connect(m_mapping, &GlyphMapping::vectorsChanged, this, &GlyphMappingChooser::updateVectorsList);
    if (m_renderView)
    {
        connect(this, &GlyphMappingChooser::renderSetupChanged, m_renderView, &AbstractRenderView::render);
        connect(m_renderView, &AbstractRenderView::beforeDeleteVisualization, 
            this, &GlyphMappingChooser::checkRemovedData);
    }
}

void GlyphMappingChooser::setSelectedData(DataObject * dataObject)
{
    if (!m_renderView)
        return;

    RenderedData3D * renderedData = nullptr;
    for (AbstractVisualizedData * it : m_renderView->visualizations())
    {
        RenderedData3D * r = dynamic_cast<RenderedData3D *>(it); // glyph mapping is only implemented for 3D data

        if (r && &r->dataObject() == dataObject)
        {
            renderedData = r;
            break;
        }
    }

    // here the user selected an object in the Browser, that is not rendered in the current view
    if (dataObject && !renderedData)
        return;

    GlyphMapping * newMapping = renderedData ? &renderedData->glyphMapping() : nullptr;

    if (newMapping == m_mapping)
        return;

    if (m_mapping)
        disconnect(m_mapping, &GlyphMapping::vectorsChanged, this, &GlyphMappingChooser::updateVectorsList);

    m_mapping = newMapping;

    updateTitle();
    updateVectorsList();

    if (m_mapping)
        connect(m_mapping, &GlyphMapping::vectorsChanged, this, &GlyphMappingChooser::updateVectorsList);
}

void GlyphMappingChooser::setSelectedData(AbstractRenderView * renderView, DataObject * dataObject)
{
    if (renderView != m_renderView)
        return;

    setSelectedData(dataObject);
}

void GlyphMappingChooser::updateVectorsList()
{
    updateGuiForSelection({});

    m_ui->propertyBrowser->setRoot(nullptr);
    qDeleteAll(m_propertyGroups);
    m_propertyGroups.clear();

    m_listModel->setMapping(m_mapping);

    if (m_mapping)
    {
        for (GlyphMappingData * vectors : m_mapping->vectors().values())
        {
            m_propertyGroups << vectors->createPropertyGroup();
            connect(vectors, &GlyphMappingData::geometryChanged, this, &GlyphMappingChooser::renderSetupChanged);
        }

        QModelIndex index(m_listModel->index(0, 0));
        m_ui->vectorsListView->selectionModel()->select(index, QItemSelectionModel::Select);
    }
}

void GlyphMappingChooser::checkRemovedData(AbstractVisualizedData * content)
{
    if (m_mapping && &m_mapping->renderedData() == content)
        setSelectedData(nullptr);
}

void GlyphMappingChooser::updateGuiForSelection(const QItemSelection & selection)
{
    disconnect(m_startingIndexConnection);

    if (selection.indexes().isEmpty())
    {
        m_ui->propertyBrowser->setRoot(nullptr);
        m_ui->propertyBrowserContainer->setTitle("(No selection)");
    }
    else
    {
        int index = selection.indexes().first().row();
        QString vectorsName = m_mapping->vectorNames()[index];

        m_ui->propertyBrowser->setRoot(m_propertyGroups[index]);
        m_ui->propertyBrowserContainer->setTitle(vectorsName);
    }
}

void GlyphMappingChooser::updateTitle()
{
    QString title;
    if (!m_mapping)
        title = "(No object selected)";
    else
        title = QString::number(m_renderView->index()) + ": " + m_mapping->renderedData().dataObject().name();

    m_ui->relatedDataObject->setText(title);
}

void GlyphMappingChooser::checkDeletedContent(AbstractVisualizedData * content)
{
    if (!content || !m_mapping)
        return;

    auto rendered = dynamic_cast<RenderedData3D *>(content);
    if (!rendered)
        return;

    if (&rendered->glyphMapping() == m_mapping)
        setSelectedData(nullptr);
}
