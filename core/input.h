#pragma once

#include <string>

#include <vtkSmartPointer.h>

class vtkDataSet;
class vtkAbstractMapper;
class vtkMapper; // 3d mapper
class vtkDataSetMapper;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkPolyDataMapper2D;
class vtkActor;
class vtkActor2D;
class vtkInformationStringKey;
class vtkPolyDataAlgorithm;
class vtkAlgorithmOutput;
class vtkContextItem;
class vtkContextActor;
class vtkLookupTable;
class vtkProp3D;

class Input {
public:
    Input(const std::string & name);
    virtual ~Input();
    const std::string name;

    virtual vtkDataSet * data() const;

    static vtkInformationStringKey * NameKey();
    
protected:
    void setMapperInfo(vtkAbstractMapper * mapper) const;

    vtkSmartPointer<vtkDataSet> m_data;
};

class DataSetInput : public Input {
public:
    DataSetInput(const std::string & name);
    void setData(vtkDataSet & data);
    vtkSmartPointer<vtkPolyDataMapper2D> mapper2D;
    vtkSmartPointer<vtkPolyDataMapper> mapper3D;
    vtkSmartPointer<vtkDataSetMapper> dataSetMapper;
    virtual void setMinMaxValue(double min, double max);
    virtual double * minMaxValue();
    virtual const double * minMaxValue() const;

    virtual vtkActor2D * createActor2D();
    virtual vtkActor * createActor3D();
    virtual vtkActor * createDataActor3D();

    vtkSmartPointer<vtkProp3D> prop;

    vtkSmartPointer<vtkLookupTable> lookupTable;

protected:
    double m_minMaxValue[2];
};

class Input3D : public Input {
public:
    Input3D(const std::string & name);
    virtual ~Input3D();

    virtual vtkActor * createActor();
    virtual vtkMapper * mapper();

protected:
    /** subclass should override this to create a vtkMapper mapping to specific input data */
    virtual vtkMapper * createDataMapper() const = 0;
    vtkSmartPointer<vtkMapper> m_mapper;
};

class PolyDataInput : public Input3D {
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

class Context2DInput {
public:
    virtual void setContextItem(vtkContextItem & item);
    virtual vtkContextItem * contextItem() const;

protected:
    vtkSmartPointer<vtkContextItem> m_contextItem;
};