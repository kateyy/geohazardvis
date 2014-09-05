#include "VectorMappingChooser.h"
#include "ui_VectorMappingChooser.h"

#include <QDebug>

#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>
#include <core/vector_mapping/VectorsToSurfaceMapping.h>

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
}

VectorMappingChooser::~VectorMappingChooser()
{
    delete m_ui;
}

void VectorMappingChooser::setMapping(int rendererId, VectorsToSurfaceMapping * mapping)
{
    if (mapping == m_mapping)
        return;

    m_mapping = mapping;

    updateWindowTitle(rendererId);

    m_listModel->setMapping(mapping);
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
