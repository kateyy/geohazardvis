#include "input.h"

#include <vtkInformationStringKey.h>
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkDataSetMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkContextItem.h>
#include <vtkProperty2D.h>

// macro defining the information key to access the name of the input in the mapper
vtkInformationKeyMacro(Input, NameKey, String);

Input::Input(const std::string & name)
: name(name)
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

Input3D::Input3D(const std::string & name)
: Input(name)
{
}

Input3D::~Input3D()
{
}

PolyDataInput::PolyDataInput(const std::string & name)
: Input3D(name)
{
}

ProcessedInput::ProcessedInput(const std::string & name)
: PolyDataInput(name)
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

DataSetInput::DataSetInput(const std::string & name)
: Input(name)
{
}

void DataSetInput::setData(vtkDataSet & data)
{
    m_data = &data;
}

void DataSetInput::setMinMaxValue(double minValue, double maxValue)
{
    m_minMaxValue[0] = minValue;
    m_minMaxValue[1] = maxValue;
}

double * DataSetInput::minMaxValue()
{
    return m_minMaxValue;
}

const double * DataSetInput::minMaxValue() const
{
    return m_minMaxValue;
}

vtkActor2D * DataSetInput::createActor2D()
{
    assert(mapper2D);
    vtkActor2D * _actor = vtkActor2D::New();
    _actor->SetMapper(mapper2D);
    _actor->GetProperty()->SetPointSize(3);
    return _actor;
}

vtkActor * DataSetInput::createActor3D()
{
    assert(mapper3D);
    vtkActor * _actor = vtkActor::New();
    _actor->SetMapper(mapper3D);
    return _actor;
}

vtkActor * DataSetInput::createDataActor3D()
{
    assert(dataSetMapper);
    vtkActor * _actor = vtkActor::New();
    _actor->SetMapper(dataSetMapper);
    return _actor;
}

void Context2DInput::setContextItem(vtkContextItem & item)
{
    m_contextItem = &item;
}

vtkContextItem * Context2DInput::contextItem() const
{
    return m_contextItem;
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
