#include "VectorMappingChooser.h"
#include "ui_VectorMappingChooser.h"

#include <reflectionzeug/PropertyGroup.h>
#include <reflectionzeug/StringProperty.h>

#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>
#include <core/vector_mapping/VectorsToSurfaceMapping.h>
#include <core/vector_mapping/VectorsForSurfaceMapping.h>

#include "VectorMappingChooserListModel.h"


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
        if (selection.indexes().isEmpty())
        {
            m_ui->propertyBrowser->setRoot(nullptr);
            m_ui->propertyBrowserLabel->setText("");
        }
        else
        {
            int index = selection.indexes().first().row();
            m_ui->propertyBrowser->setRoot(m_propertyGroups[index]);
            m_ui->propertyBrowserLabel->setText(m_mapping->vectorNames()[index]);
        }        
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

    updateTitle(rendererId);

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

void VectorMappingChooser::updateTitle(int rendererId)
{
    QString title;
    if (rendererId < 0)
        title = "(no object selected)";
    else
        title = QString::number(rendererId) + ": " + m_mapping->renderedData()->dataObject()->name();

    m_ui->relatedDataObject->setText(title);
}
