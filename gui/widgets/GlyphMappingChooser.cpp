#include "GlyphMappingChooser.h"
#include "ui_GlyphMappingChooser.h"

#include <reflectionzeug/PropertyGroup.h>
#include <reflectionzeug/StringProperty.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData3D.h>
#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>
#include <gui/data_view/RenderView.h>
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

void GlyphMappingChooser::setCurrentRenderView(RenderView * renderView)
{
    GlyphMapping * newMapping = nullptr;
    if (renderView && renderView->highlightedContent())
        if (RenderedData3D * new3D = dynamic_cast<RenderedData3D *>(renderView->highlightedContent()))
            newMapping = new3D->glyphMapping();

    if (m_renderView)
    {
        disconnect(this, &GlyphMappingChooser::renderSetupChanged, m_renderView, &RenderView::render);
        disconnect(m_renderView, &RenderView::beforeDeleteContent, this, &GlyphMappingChooser::checkRemovedData);
    }
    if (m_mapping)
        disconnect(m_mapping, &GlyphMapping::vectorsChanged, this, &GlyphMappingChooser::updateVectorsList);

    m_renderView = renderView;
    m_mapping = newMapping;

    updateTitle();

    updateVectorsList();

    if (m_mapping)
        connect(m_mapping, &GlyphMapping::vectorsChanged, this, &GlyphMappingChooser::updateVectorsList);
    if (m_renderView)
    {
        connect(this, &GlyphMappingChooser::renderSetupChanged, m_renderView, &RenderView::render);
        connect(m_renderView, &RenderView::beforeDeleteContent, this, &GlyphMappingChooser::checkRemovedData);
    }
}

void GlyphMappingChooser::setSelectedData(DataObject * dataObject)
{
    if (!m_renderView)
        return;

    RenderedData3D * renderedData = nullptr;
    for (AbstractVisualizedData * it : m_renderView->contents())
    {
        RenderedData3D * r = dynamic_cast<RenderedData3D *>(it); // glyph mapping is only implemented for 3D data

        if (r && r->dataObject() == dataObject)
        {
            renderedData = r;
            break;
        }
    }

    assert(!renderedData || dynamic_cast<RenderedData3D *>(renderedData));
    RenderedData3D * rendered3D = static_cast<RenderedData3D *>(renderedData);

    GlyphMapping * newMapping = rendered3D ? rendered3D->glyphMapping() : nullptr;

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

void GlyphMappingChooser::updateVectorsList()
{
    updateGuiForSelection();

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
    if (m_mapping && m_mapping->renderedData() == content)
        setSelectedData(nullptr);
}

void GlyphMappingChooser::updateGuiForSelection(const QItemSelection & selection)
{
    disconnect(m_startingIndexConnection);

    if (selection.indexes().isEmpty())
    {
        m_ui->propertyBrowser->setRoot(nullptr);
        m_ui->propertyBrowserContainer->setTitle("(no selection)");
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
        title = "(no object selected)";
    else
        title = QString::number(m_renderView->index()) + ": " + m_mapping->renderedData()->dataObject()->name();

    m_ui->relatedDataObject->setText(title);
}
