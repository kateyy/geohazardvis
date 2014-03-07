#include "input.h"

#include <vtkInformationStringKey.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataAlgorithm.h>

// macro defining the information key to access the name of the input in the mapper
vtkInformationKeyMacro(Input, NameKey, String);

#define VTK_CREATE(type, name) \
    vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


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

void Input::setMapperInfo(vtkAbstractMapper & mapper) const
{
    NameKey()->Set(mapper.GetInformation(), name.c_str());
}

vtkSmartPointer<vtkDataSet> Input::data() const
{
    return m_data;
}

PolyDataInput::PolyDataInput(const std::string & name, ModelType type)
: Input(name, type)
{
}

ProcessedInput::ProcessedInput(const std::string & name, ModelType type)
: PolyDataInput(name, type)
{
}

vtkSmartPointer<vtkActor> PolyDataInput::createActor()
{
    VTK_CREATE(vtkActor, actor);
    actor->SetMapper(mapper());

    return actor;
}

vtkSmartPointer<vtkMapper> PolyDataInput::mapper()
{
    if (m_mapper == nullptr) {
        m_mapper = createDataMapper();
    }
    setMapperInfo(*m_mapper);
    return m_mapper;
}

GridDataInput::GridDataInput(const std::string & name)
: Input(name, ModelType::grid2d)
, bounds()
{
}

void GridDataInput::setData(vtkSmartPointer<vtkDataSet> data)
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

void GridDataInput::setMapper(vtkSmartPointer<vtkPolyDataMapper> mapper)
{
    m_mapper = mapper;
    setMapperInfo(*m_mapper);
}

void GridDataInput::setTexture(vtkSmartPointer<vtkTexture> texture)
{
    m_texture = texture;
}

vtkSmartPointer<vtkActor> GridDataInput::createTexturedPolygonActor() const
{
    assert(m_mapper);
    assert(m_texture);
    VTK_CREATE(vtkActor, actor);
    actor->SetMapper(m_mapper);
    actor->SetTexture(m_texture);
    return actor;
}

void PolyDataInput::setPolyData(vtkSmartPointer<vtkPolyData> data)
{
    m_data = data;
}

vtkSmartPointer<vtkPolyData> PolyDataInput::polyData() const
{
    vtkPolyData * polyData = dynamic_cast<vtkPolyData*>(m_data.Get());
    assert(polyData);
    return polyData;
}

vtkSmartPointer<vtkMapper> PolyDataInput::createDataMapper() const
{
    VTK_CREATE(vtkPolyDataMapper, mapper);
    mapper->SetInputData(polyData());
    return mapper;
}

vtkSmartPointer<vtkPolyDataMapper> PolyDataInput::polyDataMapper()
{
    vtkPolyDataMapper * polyDataMapper = dynamic_cast<vtkPolyDataMapper*>(mapper().Get());
    assert(polyDataMapper);

    return polyDataMapper;
}

vtkSmartPointer<vtkMapper> ProcessedInput::createDataMapper() const
{
    VTK_CREATE(vtkPolyDataMapper, mapper);
    mapper->SetInputConnection(algorithm->GetOutputPort());
    return mapper;
}

vtkSmartPointer<vtkPolyDataMapper> ProcessedInput::createAlgorithmMapper(vtkAlgorithmOutput * mapperInput) const
{
    VTK_CREATE(vtkPolyDataMapper, mapper);
    NameKey()->Set(mapper->GetInformation(), name.c_str());
    mapper->SetInputConnection(mapperInput);
    return mapper;
}
