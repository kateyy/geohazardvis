#pragma once

#include <string>

#include <vtkSmartPointer.h>

class vtkDataSet;
class vtkMapper;
class vtkDataSetMapper;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkActor;
class vtkInformationStringKey;
class vtkPolyDataAlgorithm;
class vtkAlgorithmOutput;

class Input {
public:
    Input(const std::string & name);
    virtual ~Input();
    const std::string name;

    virtual vtkActor * createActor();
    virtual vtkMapper * mapper();
    virtual vtkDataSet * data() const;

    static vtkInformationStringKey * NameKey();

protected:
    /** subclass should override this to create a vtkMapper mapping to specific input data */
    virtual vtkMapper * createDataMapper() const = 0;
    vtkSmartPointer<vtkDataSet> m_data;
    vtkSmartPointer<vtkMapper> m_mapper;
};

class DataSetInput : public Input {
public:
    DataSetInput(const std::string & name);
    virtual void setDataSet(vtkDataSet & dataSet);
    virtual vtkDataSet * dataSet() const;
    virtual vtkDataSetMapper * dataSetMapper();
    virtual void setMinMaxValue(double min, double max);
    virtual double * minMaxValue();
    virtual const double * minMaxValue() const;

protected:
    double m_minMaxValue[2];
    virtual vtkMapper * createDataMapper() const override;
    friend class Loader;
};

class PolyDataInput : public Input {
public:
    PolyDataInput(const std::string & name);

    virtual vtkPolyData * polyData() const;
    virtual void setPolyData(vtkPolyData & polyData);
    virtual vtkPolyDataMapper * polyDataMapper();

protected:
    virtual vtkMapper * createDataMapper() const override;
};

class ProcessedInput : public PolyDataInput {
public:
    ProcessedInput(const std::string & name);
    vtkSmartPointer<vtkPolyDataAlgorithm> algorithm;
    virtual vtkPolyDataMapper * createAlgorithmMapper(vtkAlgorithmOutput * mapperInput) const;

protected:
    virtual vtkMapper * createDataMapper() const override;
};