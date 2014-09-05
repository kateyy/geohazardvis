#include "VectorMappingChooser.h"
#include "ui_VectorMappingChooser.h"

#include <reflectionzeug/PropertyGroup.h>

#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>
#include <core/vector_mapping/VectorsToSurfaceMapping.h>
#include <core/vector_mapping/VectorsForSurfaceMapping.h>

#include "VectorMappingChooserListModel.h"


namespace
{
const QString friendlyName = "vector mapping";
}

VectorMappingChooser::VectorMappingChooser(QWidget * parent, Qt::WindowFlags flags)
    : QDockWidget(parent, flags)
    , m_ui(new Ui_VectorMappingChooser())
    , m_mapping(nullptr)
    , m_listModel(new VectorMappingChooserListModel())
{
    m_ui->setupUi(this);

    m_listModel->setParent(m_ui->vectorsListView);
    m_ui->vectorsListView->setModel(m_listModel);

    connect(m_listModel, &VectorMappingChooserListModel::vectorVisibilityChanged, this, &VectorMappingChooser::renderSetupChanged);
    connect(m_ui->vectorsListView->selectionModel(), &QItemSelectionModel::selectionChanged,
        [this](const QItemSelection & selection)
    {
        m_ui->propertyBrowser->setRoot(selection.indexes().isEmpty()
            ? nullptr
            : m_propertyGroups[selection.indexes().first().row()]);
    });
}

VectorMappingChooser::~VectorMappingChooser()
{
    setMapping();

    delete m_ui;
}

void VectorMappingChooser::setMapping(int rendererId, VectorsToSurfaceMapping * mapping)
{
    if (mapping == m_mapping)
        return;

    m_mapping = mapping;

    updateWindowTitle(rendererId);

    m_ui->propertyBrowser->setRoot(nullptr);
    qDeleteAll(m_propertyGroups);
    m_propertyGroups.clear();

    m_listModel->setMapping(mapping);

    if (m_mapping)
    {
        for (VectorsForSurfaceMapping * vectors : m_mapping->vectors().values())
        {
            m_propertyGroups << vectors->createPropertyGroup();
            connect(vectors, &VectorsForSurfaceMapping::geometryChanged, this, &VectorMappingChooser::renderSetupChanged);
        }

        QModelIndex index(m_listModel->index(0, 0));
        m_ui->vectorsListView->selectionModel()->select(index, QItemSelectionModel::Select);
    }
}

const VectorsToSurfaceMapping * VectorMappingChooser::mapping() const
{
    return m_mapping;
}

void VectorMappingChooser::updateWindowTitle(int rendererId)
{
    QString title = friendlyName;
    if (rendererId >= 0)
        title += QString::number(rendererId) + m_mapping->renderedData()->dataObject()->name();

    setWindowTitle(title);
}
