#pragma once

#include <string>

#include <vtkSmartPointer.h>

class vtkPolyData;
class vtkPolyDataMapper;
class vtkActor;
class vtkInformationStringKey;
class vtkPolyDataAlgorithm;
class vtkAlgorithmOutput;

class Input {
public:
    Input(const std::string & name);
    const std::string name;

    virtual vtkSmartPointer<vtkPolyData> polyData() const;
    virtual void setPolyData(const vtkSmartPointer<vtkPolyData> & polyData);
    virtual vtkSmartPointer<vtkActor> createActor();
    virtual vtkSmartPointer<vtkPolyDataMapper> dataMapper();

    static vtkInformationStringKey * NameKey();

protected:
    vtkSmartPointer<vtkPolyData> m_polyData;
    vtkSmartPointer<vtkPolyDataMapper> m_dataMapper;
};


class ProcessedInput : public Input {
public:
    ProcessedInput(const std::string & name);
    vtkSmartPointer<vtkPolyDataAlgorithm> algorithm;
    virtual vtkSmartPointer<vtkPolyDataMapper> dataMapper() override;
    virtual vtkSmartPointer<vtkPolyDataMapper> createMapper(vtkAlgorithmOutput * mapperInput) const;
};