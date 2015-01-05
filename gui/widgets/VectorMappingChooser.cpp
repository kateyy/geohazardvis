#include "VectorMappingChooser.h"
#include "ui_VectorMappingChooser.h"

#include <reflectionzeug/PropertyGroup.h>
#include <reflectionzeug/StringProperty.h>
#include <propertyguizeug/PropertyBrowser.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData3D.h>
#include <core/vector_mapping/VectorMapping.h>
#include <core/vector_mapping/VectorMappingData.h>
#include <gui/propertyguizeug_extension/PropertyEditorFactoryEx.h>
#include <gui/propertyguizeug_extension/PropertyPainterEx.h>
#include <gui/data_view/RenderView.h>

#include "VectorMappingChooserListModel.h"


using namespace propertyguizeug;


VectorMappingChooser::VectorMappingChooser(QWidget * parent, Qt::WindowFlags flags)
    : QDockWidget(parent, flags)
    , m_ui(new Ui_VectorMappingChooser())
    , m_propertyBrowser(new PropertyBrowser(new PropertyEditorFactoryEx(), new PropertyPainterEx(), this))
    , m_renderView(nullptr)
    , m_mapping(nullptr)
    , m_listModel(new VectorMappingChooserListModel())
{
    m_ui->setupUi(this);
    m_ui->propertyBrowserContainer->layout()->addWidget(m_propertyBrowser);

    m_listModel->setParent(m_ui->vectorsListView);
    m_ui->vectorsListView->setModel(m_listModel);

    connect(m_listModel, &VectorMappingChooserListModel::vectorVisibilityChanged, this, &VectorMappingChooser::renderSetupChanged);
    connect(m_listModel, &QAbstractItemModel::dataChanged,
        [this] (const QModelIndex &topLeft, const QModelIndex & /*bottomRight*/, const QVector<int> & /*roles*/) {
        m_ui->vectorsListView->setCurrentIndex(topLeft);
    });
    connect(m_ui->vectorsListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &VectorMappingChooser::updateGuiForSelection);
}

VectorMappingChooser::~VectorMappingChooser()
{
    m_renderView = nullptr;
    m_mapping = nullptr;

    updateVectorsList();

    delete m_ui;
}

void VectorMappingChooser::setCurrentRenderView(RenderView * renderView)
{
    VectorMapping * newMapping = nullptr;
    if (renderView && renderView->highlightedContent())
        if (RenderedData3D * new3D = dynamic_cast<RenderedData3D *>(renderView->highlightedContent()))
            newMapping = new3D->vectorMapping();

    if (m_renderView)
    {
        disconnect(this, &VectorMappingChooser::renderSetupChanged, m_renderView, &RenderView::render);
        disconnect(m_renderView, &RenderView::beforeDeleteContent, this, &VectorMappingChooser::checkRemovedData);
    }
    if (m_mapping)
        disconnect(m_mapping, &VectorMapping::vectorsChanged, this, &VectorMappingChooser::updateVectorsList);

    m_renderView = renderView;
    m_mapping = newMapping;

    updateTitle();

    updateVectorsList();

    if (m_mapping)
        connect(m_mapping, &VectorMapping::vectorsChanged, this, &VectorMappingChooser::updateVectorsList);
    if (m_renderView)
    {
        connect(this, &VectorMappingChooser::renderSetupChanged, m_renderView, &RenderView::render);
        connect(m_renderView, &RenderView::beforeDeleteContent, this, &VectorMappingChooser::checkRemovedData);
    }
}

void VectorMappingChooser::setSelectedData(DataObject * dataObject)
{
    if (!m_renderView)
        return;

    RenderedData3D * renderedData = nullptr;
    for (AbstractVisualizedData * it : m_renderView->contents())
    {
        RenderedData3D * r = dynamic_cast<RenderedData3D *>(it); // vector mapping is only implemented for 3D data

        if (r && r->dataObject() == dataObject)
        {
            renderedData = r;
            break;
        }
    }

    assert(!renderedData || dynamic_cast<RenderedData3D *>(renderedData));
    RenderedData3D * rendered3D = static_cast<RenderedData3D *>(renderedData);

    VectorMapping * newMapping = rendered3D ? rendered3D->vectorMapping() : nullptr;

    if (newMapping == m_mapping)
        return;

    if (m_mapping)
        disconnect(m_mapping, &VectorMapping::vectorsChanged, this, &VectorMappingChooser::updateVectorsList);

    m_mapping = newMapping;

    updateTitle();
    updateVectorsList();

    if (m_mapping)
        connect(m_mapping, &VectorMapping::vectorsChanged, this, &VectorMappingChooser::updateVectorsList);
}

void VectorMappingChooser::updateVectorsList()
{
    updateGuiForSelection();

    m_propertyBrowser->setRoot(nullptr);
    qDeleteAll(m_propertyGroups);
    m_propertyGroups.clear();

    m_listModel->setMapping(m_mapping);

    if (m_mapping)
    {
        for (VectorMappingData * vectors : m_mapping->vectors().values())
        {
            m_propertyGroups << vectors->createPropertyGroup();
            connect(vectors, &VectorMappingData::geometryChanged, this, &VectorMappingChooser::renderSetupChanged);
        }

        QModelIndex index(m_listModel->index(0, 0));
        m_ui->vectorsListView->selectionModel()->select(index, QItemSelectionModel::Select);
    }
}

void VectorMappingChooser::checkRemovedData(AbstractVisualizedData * content)
{
    if (m_mapping && m_mapping->renderedData() == content)
        setSelectedData(nullptr);
}

void VectorMappingChooser::updateGuiForSelection(const QItemSelection & selection)
{
    disconnect(m_startingIndexConnection);

    if (selection.indexes().isEmpty())
    {
        m_propertyBrowser->setRoot(nullptr);
        m_ui->propertyBrowserContainer->setTitle("(no selection)");
    }
    else
    {
        int index = selection.indexes().first().row();
        QString vectorsName = m_mapping->vectorNames()[index];

        m_propertyBrowser->setRoot(m_propertyGroups[index]);
        m_ui->propertyBrowserContainer->setTitle(vectorsName);
    }
}

void VectorMappingChooser::updateTitle()
{
    QString title;
    if (!m_mapping)
        title = "(no object selected)";
    else
        title = QString::number(m_renderView->index()) + ": " + m_mapping->renderedData()->dataObject()->name();

    m_ui->relatedDataObject->setText(title);
}
