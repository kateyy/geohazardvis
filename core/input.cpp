#include "input.h"

#include <vtkInformationStringKey.h>
#include <vtkActor.h>
#include <vtkDataSetMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataAlgorithm.h>

// macro defining the information key to access the name of the input in the mapper
vtkInformationKeyMacro(Input, NameKey, String);

Input::Input(const std::string & name)
: name(name)
{
}

Input::~Input()
{
}

PolyDataInput::PolyDataInput(const std::string & name)
: Input(name)
{
}

ProcessedInput::ProcessedInput(const std::string & name)
: PolyDataInput(name)
{
}

vtkDataSet * Input::data() const
{
    return m_data;
}

vtkActor * Input::createActor()
{
    vtkActor * actor = vtkActor::New();
    actor->SetMapper(mapper());

    return actor;
}

vtkMapper * Input::mapper()
{
    if (m_mapper == nullptr) {
        m_mapper = createDataMapper();
        NameKey()->Set(m_mapper->GetInformation(), name.c_str());
    }
    return m_mapper;
}

DataSetInput::DataSetInput(const std::string & name)
: Input(name)
{
}

void DataSetInput::setDataSet(vtkDataSet & dataSet)
{
    m_data = &dataSet;
}

vtkDataSet * DataSetInput::dataSet() const
{
    vtkDataSet * dataSet = dynamic_cast<vtkDataSet*>(m_data.Get());
    assert(dataSet);
    return dataSet;
}

vtkDataSetMapper * DataSetInput::dataSetMapper()
{
    vtkDataSetMapper * _dataSetMapper = dynamic_cast<vtkDataSetMapper*>(mapper());
    assert(_dataSetMapper);
    return _dataSetMapper;
}

vtkMapper * DataSetInput::createDataMapper() const
{
    vtkDataSetMapper * mapper = vtkDataSetMapper::New();
    mapper->SetInputData(dataSet());
    return mapper;
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
