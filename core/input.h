#pragma once

#include <string>

#include <vtkSmartPointer.h>

class vtkDataSet;
class vtkAbstractMapper;
class vtkMapper; // 3d mapper
class vtkPolyData;
class vtkPolyDataMapper;
class vtkActor;
class vtkInformationStringKey;
class vtkPolyDataAlgorithm;
class vtkAlgorithmOutput;
class vtkLookupTable;
class vtkTexture;

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

class GridDataInput : public Input {
public:
    GridDataInput(const std::string & name);
    void setData(vtkDataSet & data);
    virtual void setMinMaxValue(double min, double max);
    virtual double * minMaxValue();
    virtual const double * minMaxValue() const;

    virtual void setMapper(vtkPolyDataMapper & mapper);
    virtual void setTexture(vtkTexture & texture);

    virtual vtkActor * createTexturedPolygonActor() const;

    double bounds[6];

    vtkSmartPointer<vtkLookupTable> lookupTable;

protected:
    vtkSmartPointer<vtkPolyDataMapper> m_mapper;
    vtkSmartPointer<vtkTexture> m_texture;
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
