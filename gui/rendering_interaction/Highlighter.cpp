#include <gui/rendering_interaction/Highlighter.h>

#include <cassert>
#include <cmath>

#include <QTime>
#include <QTimer>

#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCellArray.h>
#include <vtkDataSet.h>
#include <vtkDiskSource.h>
#include <vtkIdTypeArray.h>
#include <vtkImageData.h>
#include <vtkMath.h>
#include <vtkPoints.h>
#include <vtkPropAssembly.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkPolygon.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>


namespace
{

class HighlighterImpl
{
public:
    HighlighterImpl(Highlighter & highlighter)
        : m_highlighter(highlighter)
    {
    }

    virtual void setFlashColorFactor(double f) = 0;
    void show()
    {
        renderer().AddViewProp(m_prop);
    }
    void clear()
    {
        if (m_prop)
        {
            renderer().RemoveViewProp(m_prop);
        }
    }

    vtkRenderer & renderer()
    {
        return *m_highlighter.renderer();
    }

protected:
    Highlighter & m_highlighter;

    vtkSmartPointer<vtkProp> m_prop;
};


class HighlighterPoint2DImpl : public HighlighterImpl
{
public:
    using HighlighterImpl::HighlighterImpl;

    void setPosition(const double position[3])
    {
        init();
        m_baseActor->SetPosition(position[0], position[1]);
        m_innerActor->SetPosition(position[0], position[1]);
        show();
    }

    void setFlashColorFactor(double f) override
    {
        m_innerActor->GetProperty()->SetColor(1, f, f);
    }

private:
    void init()
    {
        if (m_baseDiskSource)
        {
            return;
        }

        int dpi = 72;
        auto renderer = m_highlighter.renderer();
        vtkRenderWindow * renWin = nullptr;
        if (renderer && (renWin = renderer->GetRenderWindow()))
        {
            dpi = renWin->GetDPI();
        }

        const auto dpcm = dpi / 2.54; // 1 inch = 25.4 mm

        const auto baseOuterRadius = 0.5 * dpcm; // 0.5 cm display size
        const auto baseInnerRadius = baseOuterRadius * 2.0 / 3.0;
        const auto innerOuterRadius = baseOuterRadius * 10.25 / 12.0;
        const auto innerInnerRadius = baseOuterRadius * 8.75 / 12.0;

        const auto circleResolution = 128;

        m_baseDiskSource = vtkSmartPointer<vtkDiskSource>::New();
        m_baseDiskSource->SetRadialResolution(circleResolution);
        m_baseDiskSource->SetCircumferentialResolution(circleResolution);
        m_baseDiskSource->SetInnerRadius(baseOuterRadius);
        m_baseDiskSource->SetOuterRadius(baseInnerRadius);

        m_innerDiskSource = vtkSmartPointer<vtkDiskSource>::New();
        m_innerDiskSource->SetRadialResolution(circleResolution);
        m_innerDiskSource->SetCircumferentialResolution(circleResolution);
        m_innerDiskSource->SetInnerRadius(innerOuterRadius);
        m_innerDiskSource->SetOuterRadius(innerInnerRadius);

        m_baseMapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
        m_baseMapper->SetInputConnection(m_baseDiskSource->GetOutputPort());

        m_innerMapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
        m_innerMapper->SetInputConnection(m_innerDiskSource->GetOutputPort());

        m_baseActor = vtkSmartPointer<vtkActor2D>::New();
        m_baseActor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
        m_baseActor->GetProperty()->SetDisplayLocationToForeground();
        m_baseActor->GetProperty()->SetColor(1, 0, 0);
        m_baseActor->PickableOff();
        m_baseActor->SetMapper(m_baseMapper);

        m_innerActor = vtkSmartPointer<vtkActor2D>::New();
        m_innerActor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
        m_innerActor->GetProperty()->SetDisplayLocationToForeground();
        m_innerActor->GetProperty()->SetColor(1, 1, 1);
        m_innerActor->PickableOff();
        m_innerActor->SetMapper(m_innerMapper);

        m_assembly = vtkSmartPointer<vtkPropAssembly>::New();
        m_assembly->AddPart(m_baseActor);
        m_assembly->AddPart(m_innerActor);

        m_prop = m_assembly;
    }

private:
    vtkSmartPointer<vtkDiskSource> m_baseDiskSource;
    vtkSmartPointer<vtkDiskSource> m_innerDiskSource;
    vtkSmartPointer<vtkPolyDataMapper2D> m_baseMapper;
    vtkSmartPointer<vtkPolyDataMapper2D> m_innerMapper;
    vtkSmartPointer<vtkActor2D> m_baseActor;
    vtkSmartPointer<vtkActor2D> m_innerActor;
    vtkSmartPointer<vtkPropAssembly> m_assembly;
};


class HighlighterCellImpl : public HighlighterImpl
{
public:
    using HighlighterImpl::HighlighterImpl;

    void setCell(vtkCell & selection)
    {
        init();

        const vtkIdType numberOfPoints = selection.GetNumberOfPoints();

        double cellNormal[3];
        vtkPolygon::ComputeNormal(selection.GetPoints(), cellNormal);
        vtkMath::MultiplyScalar(cellNormal, 0.001);


        auto points = m_polyData->GetPoints();
        auto polys = vtkSmartPointer<vtkCellArray>::New();

        points->SetNumberOfPoints(numberOfPoints * 2);
        std::vector<vtkIdType> front, back;
        for (vtkIdType i = 0; i < numberOfPoints; ++i)
        {
            double original[3], shifted[3];
            selection.GetPoints()->GetPoint(i, original);

            vtkMath::Add(original, cellNormal, shifted);
            points->InsertPoint(i, shifted);
            front.push_back(i);

            vtkMath::Subtract(original, cellNormal, shifted);
            points->InsertPoint(i + numberOfPoints, shifted);
            back.push_back(i + numberOfPoints);
        }

        polys->InsertNextCell(numberOfPoints, front.data());
        polys->InsertNextCell(numberOfPoints, back.data());

        m_polyData->SetPolys(polys);

        show();
    }

    void setFlashColorFactor(double f)
    {
        m_actor->GetProperty()->SetColor(1, f, f);
    }

    void clear()
    {
        if (m_prop && m_highlighter.renderer())
        {
            m_highlighter.renderer()->RemoveViewProp(m_prop);
        }
    }

private:
    void init()
    {
        if (m_polyData)
        {
            return;
        }

        m_polyData = vtkSmartPointer<vtkPolyData>::New();
        m_polyData->SetPoints(vtkSmartPointer<vtkPoints>::New());
        m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        m_mapper->SetInputData(m_polyData);

        m_actor = vtkSmartPointer<vtkActor>::New();
        m_actor->GetProperty()->EdgeVisibilityOn();
        m_actor->GetProperty()->SetEdgeColor(1, 0, 0);
        m_actor->GetProperty()->SetLineWidth(3);
        m_actor->PickableOff();
        m_actor->SetMapper(m_mapper);

        m_prop = m_actor;
    }

private:
    vtkSmartPointer<vtkPolyData> m_polyData;
    vtkSmartPointer<vtkPolyDataMapper> m_mapper;
    vtkSmartPointer<vtkActor> m_actor;
};

}


class HighlighterImplementationSwitch
{
public:
    HighlighterImplementationSwitch(Highlighter & highlighter)
        : m_point2DImpl(highlighter)
        , m_cellImpl(highlighter)
        , m_currentImpl{ nullptr }
    {
    }

    void clear()
    {
        switchTo(nullptr);
    }

    void highlightCell(vtkCell & cell)
    {
        switchTo(&m_cellImpl);
        m_cellImpl.setCell(cell);
    }

    void highlightPoint2D(double position[3])
    {
        switchTo(&m_point2DImpl);
        m_point2DImpl.setPosition(position);
    }

    void setFlashColorFactor(float f)
    {
        if (m_currentImpl)
        {
            m_currentImpl->setFlashColorFactor(f);
        }
    }

private:
    void switchTo(HighlighterImpl * impl)
    {
        if (m_currentImpl == impl)
        {
            return;
        }

        if (m_currentImpl)
        {
            m_currentImpl->clear();
        }

        m_currentImpl = impl;
    }

private:
    HighlighterCellImpl m_cellImpl;
    HighlighterPoint2DImpl m_point2DImpl;

    HighlighterImpl * m_currentImpl;
};



Highlighter::Highlighter()
    : QObject()
    , m_visualizedData{ nullptr }
    , m_visOutputPort{ -1 }
    , m_indices{ vtkSmartPointer<vtkIdTypeArray>::New() }
    , m_indexType{ IndexType::points }
    , m_impl{ std::make_unique<HighlighterImplementationSwitch>(*this) }
    , m_flashAfterSetTarget{ true }
    , m_flashTimeMilliseconds{ 2000 }
{
}

Highlighter::~Highlighter() = default;

void Highlighter::setRenderer(vtkRenderer * renderer)
{
    if (renderer == m_renderer)
    {
        return;
    }

    clear();

    m_renderer = renderer;
}

vtkRenderer * Highlighter::renderer() const
{
    return m_renderer;
}

void Highlighter::setTarget(AbstractVisualizedData * vis, int visOutputPort, vtkIdType index, IndexType indexType)
{
    if (index < 0)
    {
        clearIndices();
        return;
    }

    m_indices->SetNumberOfValues(1);
    m_indices->SetValue(0, index);

    setTargetInternal(vis, visOutputPort, indexType);

    updateHighlight();
}

void Highlighter::setTarget(AbstractVisualizedData * vis, int visOutputPort, vtkIdTypeArray & indices, IndexType indexType)
{
    std::vector<vtkIdType> validIndices;
    validIndices.reserve(indices.GetSize());
    for (vtkIdType i = 0; i < indices.GetSize(); ++i)
    {
        const auto && val = indices.GetValue(i);
        if (val >= 0)
        {
            validIndices.push_back(val);
        }
    }

    m_indices->Resize(validIndices.size());
    for (vtkIdType i = 0; i < vtkIdType(validIndices.size()); ++i)
    {
        m_indices->SetValue(i, validIndices[i]);
    }

    setTargetInternal(vis, visOutputPort, indexType);

    updateHighlight();
}

void Highlighter::setTargetInternal(AbstractVisualizedData * vis, int visOutputPort, IndexType indexType)
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
}

AbstractVisualizedData * Highlighter::targetVisualization() const
{
    return m_visualizedData;
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

    setTargetInternal(nullptr, -1, m_indexType);

    m_renderer = nullptr;
}

void Highlighter::clearIndices()
{
    m_indices->SetNumberOfValues(0);

    if (m_renderer)
    {
        m_impl->clear();
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
            const double f = 0.5 + 0.5 * std::cos(ms * 0.001 * 2.0 * vtkMath::Pi());
            m_impl->setFlashColorFactor(f);

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

    const vtkIdType index = m_indices->GetValue(0);
    double point[3];
    dataSet->GetPoint(index, point);
    m_impl->highlightPoint2D(point);

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

    const vtkIdType index = m_indices->GetValue(0);

    // extract picked triangle and create highlighting geometry
    // create two shifted polygons to work around occlusion

    vtkCell * selection = dataSet->GetCell(index);

    // this is probably a glyph or the like; we don't have an implementation for that in the moment
    if (!selection || selection->GetCellType() == VTK_VOXEL)
    {
        return;
    }

    m_impl->highlightCell(*selection);

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
