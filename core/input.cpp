#include "input.h"

#include <vtkInformationStringKey.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataAlgorithm.h>

// macro defining the information key to access the name of the input in the mapper
vtkInformationKeyMacro(Input, NameKey, String);

Input * Input::createType(ModelType type, const std::string & name)
{
    switch (type) {
    case ModelType::triangles:
        return new PolyDataInput(name, type);
    case ModelType::grid2d:
        return new GridDataInput(name);
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

void Input::setMapperInfo(vtkAbstractMapper * mapper) const
{
    NameKey()->Set(mapper->GetInformation(), name.c_str());
}

vtkDataSet * Input::data() const
{
    return m_data;
}

Input3D::Input3D(const std::string & name, ModelType type)
: Input(name, type)
{
}

Input3D::~Input3D()
{
}

PolyDataInput::PolyDataInput(const std::string & name, ModelType type)
: Input3D(name, type)
{
}

ProcessedInput::ProcessedInput(const std::string & name, ModelType type)
: PolyDataInput(name, type)
{
}

vtkActor * Input3D::createActor()
{
    vtkActor * actor = vtkActor::New();
    actor->SetMapper(mapper());

    return actor;
}

vtkMapper * Input3D::mapper()
{
    if (m_mapper == nullptr) {
        m_mapper = createDataMapper();
    }
    setMapperInfo(m_mapper);
    return m_mapper;
}

GridDataInput::GridDataInput(const std::string & name)
: Input(name, ModelType::grid2d)
, bounds()
{
}

void GridDataInput::setData(vtkDataSet & data)
{
    m_data = &data;
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

void GridDataInput::setMapper(vtkPolyDataMapper & mapper)
{
    m_mapper = &mapper;
    setMapperInfo(m_mapper);
}

void GridDataInput::setTexture(vtkTexture & texture)
{
    m_texture = &texture;
}

vtkActor * GridDataInput::createTexturedPolygonActor() const
{
    assert(m_mapper);
    assert(m_texture);
    vtkActor * actor = vtkActor::New();
    actor->SetMapper(m_mapper);
    actor->SetTexture(m_texture);
    return actor;
}

void PolyDataInput::setPolyData(vtkPolyData & data)
{
    m_data = &data;
}

vtkPolyData * PolyDataInput::polyData() const
{
    vtkPolyData * polyData = dynamic_cast<vtkPolyData*>(m_data.Get());
    assert(polyData);
    return polyData;
}

vtkMapper * PolyDataInput::createDataMapper() const
{
    vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();
    mapper->SetInputData(polyData());
    return mapper;
}

vtkPolyDataMapper * PolyDataInput::polyDataMapper()
{
    vtkPolyDataMapper * polyMapper = dynamic_cast<vtkPolyDataMapper*>(mapper());
    assert(polyMapper);
    return polyMapper;
}

vtkMapper * ProcessedInput::createDataMapper() const
{
    vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();
    mapper->SetInputConnection(algorithm->GetOutputPort());
    return mapper;
}

vtkPolyDataMapper * ProcessedInput::createAlgorithmMapper(vtkAlgorithmOutput * mapperInput) const
{
    vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();
    NameKey()->Set(mapper->GetInformation(), name.c_str());
    mapper->SetInputConnection(mapperInput);
    return mapper;
}
