#pragma once

#include <core/rendered_data/RenderedData.h>


class vtkAlgorithm;
class vtkAssignAttribute;
class vtkImageMapToColors;
class vtkImageProperty;
class vtkImageSlice;
class vtkImageSliceMapper;

class ArrayChangeInformationFilter;
class DEMApplyShadingToColors;
class DEMImageNormals;
class DEMShadingFilter;
class ImageDataObject;


class CORE_API RenderedImageData : public RenderedData
{
public:
    explicit RenderedImageData(ImageDataObject & dataObject);
    ~RenderedImageData() override;

    ImageDataObject & imageDataObject();
    const ImageDataObject & imageDataObject() const;

    std::unique_ptr<reflectionzeug::PropertyGroup> createConfigGroup() override;

    bool isInterpolationEnabled() const;
    void setInterpolationEnabled(bool enable);

    bool isShadingEnabled() const;
    void setEnableShading(bool enable);

    double ambient() const;
    void setAmbient(double ambient);

    double diffuse() const;
    void setDiffuse(double diffuse);


protected:
    vtkSmartPointer<vtkPropCollection> fetchViewProps() override;
    vtkImageSlice * slice();

    vtkImageProperty * property();

    void setupColorMapping(ColorMapping & colorMapping) override;
    void scalarsForColorMappingChangedEvent() override;
    void colorMappingGradientChangedEvent() override;
    void visibilityChangedEvent(bool visible) override;

private:
    void initializePipeline();
    void configureVisPipeline();

private:
    bool m_isShadingEnabled;

    vtkSmartPointer<ArrayChangeInformationFilter> m_copyScalarsFilter;
    vtkSmartPointer<vtkImageMapToColors> m_imageScalarsToColors;

    vtkSmartPointer<vtkAlgorithm> m_colorMappingFilter;

    vtkSmartPointer<vtkAssignAttribute> m_assignElevationsForNormalComputation;
    vtkSmartPointer<DEMImageNormals> m_demNormals;
    vtkSmartPointer<DEMShadingFilter> m_demShading;
    vtkSmartPointer<vtkAssignAttribute> m_assignMappedColorsForShading;
    vtkSmartPointer<DEMApplyShadingToColors> m_applyDEMShading;

    vtkSmartPointer<vtkImageSliceMapper> m_mapper;
    vtkSmartPointer<vtkImageSlice> m_slice;
    vtkSmartPointer<vtkImageProperty> m_property;

private:
    Q_DISABLE_COPY(RenderedImageData)
};
