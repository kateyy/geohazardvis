#pragma once

#include <string>
#include <memory>

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


enum class ModelType {
    triangles,
    grid2d
};

class Input {
public:
    static std::shared_ptr<Input> createType(ModelType type, const std::string & name);

    virtual ~Input();

    const std::string name;

    virtual vtkSmartPointer<vtkDataSet> data() const;

    static vtkInformationStringKey * NameKey();

    const ModelType type;
    
protected:
    Input(const std::string & name, const ModelType type);

    void setMapperInfo(vtkAbstractMapper & mapper) const;

    vtkSmartPointer<vtkDataSet> m_data;
};

class GridDataInput : public Input {
public:
    GridDataInput(const std::string & name);
    void setData(vtkSmartPointer<vtkDataSet> data);
    virtual void setMinMaxValue(double min, double max);
    virtual double * minMaxValue();
    virtual const double * minMaxValue() const;

    virtual void setMapper(vtkSmartPointer<vtkPolyDataMapper> mapper);
    virtual void setTexture(vtkSmartPointer<vtkTexture> texture);

    virtual vtkSmartPointer<vtkActor> createTexturedPolygonActor() const;

    double bounds[6];

    vtkSmartPointer<vtkLookupTable> lookupTable;

protected:
    vtkSmartPointer<vtkPolyDataMapper> m_mapper;
    vtkSmartPointer<vtkTexture> m_texture;
    double m_minMaxValue[2];
};

class Input3D : public Input {
public:
    Input3D(const std::string & name, ModelType type);
    virtual ~Input3D();

    virtual vtkSmartPointer<vtkActor> createActor();
    virtual vtkSmartPointer<vtkMapper> mapper();

protected:
    /** subclass should override this to create a vtkMapper mapping to specific input data */
    virtual vtkSmartPointer<vtkMapper> createDataMapper() const = 0;
    vtkSmartPointer<vtkMapper> m_mapper;
};

class PolyDataInput : public Input3D {
public:
    PolyDataInput(const std::string & name, ModelType type);

    virtual vtkSmartPointer<vtkPolyData> polyData() const;
    virtual void setPolyData(vtkSmartPointer<vtkPolyData> polyData);
    virtual vtkSmartPointer<vtkPolyDataMapper> polyDataMapper();

protected:
    virtual vtkSmartPointer<vtkMapper> createDataMapper() const override;
};

class ProcessedInput : public PolyDataInput {
public:
    ProcessedInput(const std::string & name, ModelType type);
    vtkSmartPointer<vtkPolyDataAlgorithm> algorithm;
    virtual vtkSmartPointer<vtkPolyDataMapper> createAlgorithmMapper(vtkAlgorithmOutput * mapperInput) const;

protected:
    virtual vtkSmartPointer<vtkMapper> createDataMapper() const override;
};
