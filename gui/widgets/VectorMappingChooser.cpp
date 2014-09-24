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
    , m_rendererId(-1)
    , m_mapping(nullptr)
    , m_listModel(new VectorMappingChooserListModel())
{
    m_ui->setupUi(this);

    m_listModel->setParent(m_ui->vectorsListView);
    m_ui->vectorsListView->setModel(m_listModel);

    connect(m_listModel, &VectorMappingChooserListModel::vectorVisibilityChanged, this, &VectorMappingChooser::renderSetupChanged);
    connect(m_ui->vectorsListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &VectorMappingChooser::updateGuiForSelection);
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

    m_rendererId = rendererId;
    m_mapping = mapping;

    updateTitle();

    updateVectorsList();

    if (m_mapping)
        connect(m_mapping, &VectorsToSurfaceMapping::vectorsChanged, this, &VectorMappingChooser::updateVectorsList);
}

void VectorMappingChooser::updateVectorsList()
{
    updateGuiForSelection();

    m_ui->propertyBrowser->setRoot(nullptr);
    qDeleteAll(m_propertyGroups);
    m_propertyGroups.clear();

    m_listModel->setMapping(m_mapping);

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

int VectorMappingChooser::rendererId() const
{
    return m_rendererId;
}

const VectorsToSurfaceMapping * VectorMappingChooser::mapping() const
{
    return m_mapping;
}

void VectorMappingChooser::updateGuiForSelection(const QItemSelection & selection)
{
    disconnect(m_startingIndexConnection);

    if (selection.indexes().isEmpty())
    {
        m_ui->firstIndexSlider->setEnabled(false);
        m_ui->propertyBrowser->setRoot(nullptr);
        m_ui->propertyBrowserContainer->setTitle("(no selection)");
        m_ui->firstIndexLabel->setText("first &index");
    }
    else
    {
        int index = selection.indexes().first().row();
        QString vectorsName = m_mapping->vectorNames()[index];

        m_ui->propertyBrowser->setRoot(m_propertyGroups[index]);
        m_ui->propertyBrowserContainer->setTitle(vectorsName);

        VectorsForSurfaceMapping * vectors = m_mapping->vectors()[vectorsName];

        m_startingIndexConnection =
            connect(m_ui->firstIndexSlider, static_cast<void(QAbstractSlider::*)(int)>(&QAbstractSlider::valueChanged),
            [this, vectors](int value) {
            vectors->setStartingIndex(value);
            m_ui->firstIndexLabel->setText("first &index (" + QString::number(value) + "/" + QString::number(vectors->maximumStartingIndex()) + ")");
        });
        m_ui->firstIndexSlider->setMaximum(vectors->maximumStartingIndex());
        m_ui->firstIndexSlider->setValue(vectors->startingIndex());
        m_ui->firstIndexSlider->setEnabled(vectors->maximumStartingIndex() > 0);
    }
}

void VectorMappingChooser::updateTitle()
{
    QString title;
    if (!m_mapping)
        title = "(no object selected)";
    else
        title = QString::number(m_rendererId) + ": " + m_mapping->renderedData()->dataObject()->name();

    m_ui->relatedDataObject->setText(title);
}
