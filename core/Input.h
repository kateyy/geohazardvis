#pragma once

#include <string>
#include <memory>

#include <vtkSmartPointer.h>

#include <core/core_api.h>

class vtkDataSet;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkActor;
class vtkInformationStringKey;
class vtkLookupTable;
class vtkTexture;
class vtkImageData;


enum class ModelType {
    triangles,
    grid2d
};

class CORE_API Input {
public:
    static std::shared_ptr<Input> createType(ModelType type, const std::string & name);

    virtual ~Input();

    const std::string name;

    virtual vtkSmartPointer<vtkDataSet> data() const;
    virtual const double * bounds() const = 0;

    static vtkInformationStringKey * NameKey();

    const ModelType type;
    
protected:
    Input(const std::string & name, const ModelType type);

    vtkSmartPointer<vtkDataSet> m_data;
};

class GridDataInput : public Input {
public:
    GridDataInput(const std::string & name);
    void setData(vtkSmartPointer<vtkImageData> data);
    virtual void setMinMaxValue(double min, double max);
    virtual double * minMaxValue();
    virtual const double * minMaxValue() const;

    virtual vtkSmartPointer<vtkImageData> imageData() const;

    virtual void setMapper(vtkSmartPointer<vtkPolyDataMapper> mapper);
    virtual void setTexture(vtkSmartPointer<vtkTexture> texture);

    virtual vtkActor * createTexturedPolygonActor() const;

    const double * bounds() const override;
    double * bounds();

    vtkSmartPointer<vtkLookupTable> lookupTable;

protected:
    vtkSmartPointer<vtkPolyDataMapper> m_mapper;
    vtkSmartPointer<vtkTexture> m_texture;
    double m_minMaxValue[2];
    double m_bounds[6];
};

class PolyDataInput : public Input {
public:
    PolyDataInput(const std::string & name, ModelType type);

    virtual vtkPolyData * polyData() const;
    virtual void setPolyData(vtkSmartPointer<vtkPolyData> polyData);

    const double * bounds() const override;

    vtkPolyDataMapper * createNamedMapper() const;
};
