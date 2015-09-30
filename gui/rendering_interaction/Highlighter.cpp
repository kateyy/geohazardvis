#include <gui/rendering_interaction/Highlighter.h>

#include <cassert>

#include <QTime>
#include <QTimer>

#include <vtkActor.h>
#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCellArray.h>
#include <vtkDataSet.h>
#include <vtkDiskSource.h>
#include <vtkIdTypeArray.h>
#include <vtkMath.h>
#include <vtkPoints.h>
#include <vtkProperty.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolygon.h>
#include <vtkRenderer.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>


Highlighter::Highlighter()
    : m_visualizedData(nullptr)
    , m_visOutputPort(-1)
    , m_indices(vtkSmartPointer<vtkIdTypeArray>::New())
    , m_indexType(IndexType::points)
    , m_flashAfterSetTarget(true)
    , m_flashTimeMilliseconds(2000)
{
}

Highlighter::~Highlighter() = default;

void Highlighter::setRenderer(vtkRenderer * renderer)
{
    clear();

    m_renderer = renderer;
}

vtkRenderer * Highlighter::renderer() const
{
    return m_renderer;
}

void Highlighter::setTarget(AbstractVisualizedData * vis, vtkIdType visOutputPort, vtkIdType index, IndexType indexType)
{
    m_indices->SetNumberOfValues(1);
    m_indices->SetValue(0, index);

    setTargetInternal(vis, visOutputPort, indexType);
}

void Highlighter::setTarget(AbstractVisualizedData * vis, vtkIdType visOutputPort, vtkIdTypeArray & indices, IndexType indexType)
{
    m_indices->DeepCopy(&indices);

    setTargetInternal(vis, visOutputPort, indexType);
}

void Highlighter::setTargetInternal(AbstractVisualizedData * vis, vtkIdType visOutputPort, IndexType indexType)
{
    if (m_visualizedData)
    {
        disconnect(m_visualizedData, &AbstractVisualizedData::visibilityChanged, this, &Highlighter::checkDataVisibility);
    }

    m_visualizedData = vis;
    m_visOutputPort = visOutputPort;
    m_indexType = indexType;

    if (vis)
    {
        connect(vis, &AbstractVisualizedData::visibilityChanged, this, &Highlighter::checkDataVisibility);
    }

    updateHighlight();
}

AbstractVisualizedData * Highlighter::targetVisualization() const
{
    return nullptr;
}

vtkIdTypeArray * Highlighter::targetIndices()
{
    return m_indices;
}

vtkIdType Highlighter::lastTargetIndex() const
{
    if (m_indices->GetSize() == 0)
        return -1;

    return m_indices->GetValue(m_indices->GetSize() - 1);
}

IndexType Highlighter::targetIndexType() const
{
    return m_indexType;
}

void Highlighter::clear()
{
    clearIndices();

    m_visualizedData = nullptr;
    m_visOutputPort = -1;
    m_renderer = nullptr;
}

void Highlighter::clearIndices()
{
    m_indices->SetNumberOfValues(0);

    if (m_renderer)
    {
        m_renderer->RemoveViewProp(m_highlightActor);
    }
}

void Highlighter::flashTargets()
{
    if (!m_renderer)
        return;

    if (m_indices->GetSize() == 0)
        return;

    assert(m_visualizedData);


    if (!m_highlightFlashTimer)
    {
        m_highlightFlashTime = std::make_unique<QTime>();
        m_highlightFlashTimer = std::make_unique<QTimer>();
        m_highlightFlashTimer->setSingleShot(false);
        m_highlightFlashTimer->setInterval(1);

        connect(m_highlightFlashTimer.get(), &QTimer::timeout,
                [this] () {
            int ms = (1000 + m_highlightFlashTime->msec() - QTime::currentTime().msec()) % 1000;
            if (*m_highlightFlashTime < QTime::currentTime())
            {
                m_highlightFlashTimer->stop();
                ms = 0;
            }
            double bg = 0.5 + 0.5 * std::cos(ms * 0.001 * 2.0 * vtkMath::Pi());
            m_highlightActor->GetProperty()->SetColor(1, bg, bg);

            emit geometryChanged();
        });
    }
    else if (m_highlightFlashTimer->isActive())
    {
        return;
    }

    *m_highlightFlashTime = QTime::currentTime().addMSecs(m_flashTimeMilliseconds);
    m_highlightFlashTimer->start();
}

void Highlighter::updateHighlight()
{
    if (!m_renderer)
    {
        clear();
        return;
    }
    if (!m_visualizedData || m_indices->GetSize() == 0)
    {
        clearIndices();
        return;
    }

    if (!m_highlightActor)
    {
        m_highlightMapper = vtkSmartPointer<vtkPolyDataMapper>::New();

        m_highlightActor = vtkSmartPointer<vtkActor>::New();
        m_highlightActor->GetProperty()->EdgeVisibilityOn();
        m_highlightActor->GetProperty()->SetEdgeColor(1, 0, 0);
        m_highlightActor->GetProperty()->SetLineWidth(3);
        m_highlightActor->PickableOff();
        m_highlightActor->SetMapper(m_highlightMapper);
    }


    switch (m_indexType)
    {
    case IndexType::points:
        highlightPoints();
        break;
    case IndexType::cells:
        highlightCells();
        break;
    }

    if (m_flashAfterSetTarget)
    {
        flashTargets();
    }
}

void Highlighter::highlightPoints()
{
    assert(m_renderer && m_visualizedData && m_indices->GetSize() > 0);

    auto dataSet = m_visualizedData->colorMappingInputData(m_visOutputPort);
    if (!dataSet)
    {
        return;
    }
    

    // TODO multi selections
    vtkIdType index = m_indices->GetValue(0);

    // this actually assumes a data set / image parallel to the XY-plane

    if (!m_pointHighlightDisk)
    {
        m_pointHighlightDisk = vtkSmartPointer<vtkDiskSource>::New();
        m_pointHighlightDisk->SetRadialResolution(128);
        m_pointHighlightDisk->SetCircumferentialResolution(128);
        m_pointHighlightDisk->SetInnerRadius(1);
        m_pointHighlightDisk->SetOuterRadius(2);
    }

    m_highlightMapper->SetInputConnection(m_pointHighlightDisk->GetOutputPort());

    double point[3];
    dataSet->GetPoint(index, point);
    point[2] += 0.1;    // show in front of the image
    m_highlightActor->SetPosition(point);

    m_renderer->AddViewProp(m_highlightActor);

    emit geometryChanged();
}

void Highlighter::highlightCells()
{
    assert(m_renderer && m_visualizedData && m_indices->GetSize() > 0);

    auto dataSet = m_visualizedData->colorMappingInputData(m_visOutputPort);
    if (!dataSet)
    {
        return;
    }

    // TODO implement highlighting of multiple targets, get inspired by ParaView
    vtkIdType index = m_indices->GetValue(0);

    if (!m_cellHighlightPolys)
    {
        m_cellHighlightPolys = vtkSmartPointer<vtkPolyData>::New();
        m_cellHighlightPolys->SetPoints(vtkSmartPointer<vtkPoints>::New());
    }

    // extract picked triangle and create highlighting geometry
    // create two shifted polygons to work around occlusion

    vtkCell * selection = dataSet->GetCell(index);

    // this is probably a glyph or the like; we don't have an implementation for that in the moment
    if (selection->GetCellType() == VTK_VOXEL)
        return;

    vtkIdType numberOfPoints = selection->GetNumberOfPoints();

    double cellNormal[3];
    vtkPolygon::ComputeNormal(selection->GetPoints(), cellNormal);
    vtkMath::MultiplyScalar(cellNormal, 0.001);


    auto points = m_cellHighlightPolys->GetPoints();
    auto polys = vtkSmartPointer<vtkCellArray>::New();

    points->SetNumberOfPoints(numberOfPoints * 2);
    std::vector<vtkIdType> front, back;
    for (vtkIdType i = 0; i < numberOfPoints; ++i)
    {
        double original[3], shifted[3];
        selection->GetPoints()->GetPoint(i, original);

        vtkMath::Add(original, cellNormal, shifted);
        points->InsertPoint(i, shifted);
        front.push_back(i);

        vtkMath::Subtract(original, cellNormal, shifted);
        points->InsertPoint(i + numberOfPoints, shifted);
        back.push_back(i + numberOfPoints);
    }

    polys->InsertNextCell(numberOfPoints, front.data());
    polys->InsertNextCell(numberOfPoints, back.data());

    m_cellHighlightPolys->SetPolys(polys);

    
    m_highlightMapper->SetInputData(m_cellHighlightPolys);
    m_highlightActor->SetPosition(0, 0, 0);

    m_renderer->AddViewProp(m_highlightActor);

    emit geometryChanged();
}

void Highlighter::checkDataVisibility()
{
    assert(sender() == m_visualizedData);
    assert(!m_visualizedData->isVisible());

    clear();
}

void Highlighter::setFlashAfterTarget(bool doFlash)
{
    m_flashAfterSetTarget = doFlash;
}

bool Highlighter::flashAfterSetTarget() const
{
    return m_flashAfterSetTarget;
}

void Highlighter::setFlashTimeMilliseconds(unsigned int milliseconds)
{
    m_flashTimeMilliseconds = milliseconds;
}

unsigned int Highlighter::flashTimeMilliseconds() const
{
    return m_flashTimeMilliseconds;
}
