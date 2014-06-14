#include "Input.h"

#include <cassert>

#include <vtkInformationStringKey.h>

#include <vtkPolyData.h>
#include <vtkImageData.h>

#include <vtkPolyDataMapper.h>

#include <vtkActor.h>

#include "vtkhelper.h"


// macro defining the information key to access the name of the input in the mapper
vtkInformationKeyMacro(Input, NameKey, String);

std::shared_ptr<Input> Input::createType(ModelType type, const std::string & name)
{
    switch (type) {
    case ModelType::triangles:
        return std::make_shared<PolyDataInput>(name, type);
    case ModelType::grid2d:
        return std::make_shared<GridDataInput>(name);
    }
    assert(false);
    return nullptr;
}

Input::Input(const std::string & name, ModelType type)
: name(name)
, type(type)
{
}

Input::~Input()
{
}

vtkDataSet * Input::data() const
{
    return m_data;
}

PolyDataInput::PolyDataInput(const std::string & name, ModelType type)
: Input(name, type)
{
}

GridDataInput::GridDataInput(const std::string & name)
: Input(name, ModelType::grid2d)
, m_bounds()
{
}

void GridDataInput::setData(vtkImageData * data)
{
    m_data = data;
}

void GridDataInput::setMinMaxValue(double minValue, double maxValue)
{
    m_minMaxValue[0] = minValue;
    m_minMaxValue[1] = maxValue;
}

double * GridDataInput::minMaxValue()
{
    return m_minMaxValue;
}

const double * GridDataInput::minMaxValue() const
{
    return m_minMaxValue;
}

int * GridDataInput::dimensions()
{
    return m_dimensions;
}

const int * GridDataInput::dimensions() const
{
    return m_dimensions;
}

void GridDataInput::setDimensions(int x, int y, int z)
{
    m_dimensions[0] = x;
    m_dimensions[1] = y;
    m_dimensions[2] = z;
}

vtkImageData * GridDataInput::imageData() const
{
    vtkImageData * imageData = static_cast<vtkImageData*>(m_data.Get());
    assert(imageData);
    return imageData;
}

const double * GridDataInput::bounds() const
{
    return m_bounds;
}

double * GridDataInput::bounds()
{
    return m_bounds;
}

void PolyDataInput::setPolyData(vtkSmartPointer<vtkPolyData> data)
{
    m_data = data;
}

vtkPolyData * PolyDataInput::polyData() const
{
    assert(dynamic_cast<vtkPolyData*>(m_data.Get()));
    return static_cast<vtkPolyData*>(m_data.Get());
}

vtkPolyDataMapper * PolyDataInput::createNamedMapper() const
{
    vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();

    NameKey()->Set(mapper->GetInformation(), name.c_str());

    return mapper;
}

const double * PolyDataInput::bounds() const
{
    return polyData()->GetBounds();
}
