#include "input.h"

#include <vtkInformationStringKey.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataAlgorithm.h>

// macro defining the information key to access the name of the input in the mapper
vtkInformationKeyMacro(Input, NameKey, String);

Input::Input(const std::string & name)
: name(name)
{
}

ProcessedInput::ProcessedInput(const std::string & name)
: Input(name)
{
}

vtkSmartPointer<vtkPolyData> Input::polyData() const
{
    return m_polyData;
}

void Input::setPolyData(const vtkSmartPointer<vtkPolyData> & polyData)
{
    m_polyData = polyData;
}

vtkSmartPointer<vtkActor> Input::createActor()
{
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(dataMapper());

    return actor;
}

vtkSmartPointer<vtkPolyDataMapper> Input::dataMapper()
{
    if (m_dataMapper == nullptr) {
        m_dataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        m_dataMapper->SetInputData(m_polyData);
        NameKey()->Set(m_dataMapper->GetInformation(), name.c_str());
    }
    return m_dataMapper;
}

vtkSmartPointer<vtkPolyDataMapper> ProcessedInput::dataMapper()
{
    if (m_dataMapper == nullptr) {
        m_dataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        m_dataMapper->SetInputConnection(algorithm->GetOutputPort());
        NameKey()->Set(m_dataMapper->GetInformation(), name.c_str());
    }
    return m_dataMapper;
}

vtkSmartPointer<vtkPolyDataMapper> ProcessedInput::createMapper(vtkAlgorithmOutput * mapperInput) const
{
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    NameKey()->Set(mapper->GetInformation(), name.c_str());
    mapper->SetInputConnection(mapperInput);
    return mapper;
}
