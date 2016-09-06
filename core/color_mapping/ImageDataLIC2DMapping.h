#pragma once

#include <QMap>

#include <core/color_mapping/ColorMappingData.h>


template<typename T> class QVector;
class vtkImageDataLIC2D;
class vtkRenderWindow;

class NoiseImageSource;


class CORE_API ImageDataLIC2DMapping : public ColorMappingData
{
public:
    explicit ImageDataLIC2DMapping(const QList<AbstractVisualizedData *> & visualizedData);
    ~ImageDataLIC2DMapping() override;

    QString name() const override;
    QString scalarsName(AbstractVisualizedData & vis) const override;
    IndexType scalarsAssociation(AbstractVisualizedData & vis) const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData & visualizedData, int connection) override;
    bool usesFilter() const override;


protected:
    std::vector<ValueRange<>> updateBounds() override;

    vtkRenderWindow * glContext();

private:
    vtkSmartPointer<NoiseImageSource> m_noiseImage;
    QMap<AbstractVisualizedData *, QVector<vtkSmartPointer<vtkImageDataLIC2D>>> m_lic2D;

    vtkSmartPointer<vtkRenderWindow> m_glContext;

    static const QString s_name;
    static const bool s_isRegistered;
};
