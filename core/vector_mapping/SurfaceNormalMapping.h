#pragma once

#include <core/vector_mapping/VectorsForSurfaceMapping.h>


enum class NormalType
{
    CellNormal,
    PointNormal
};

class CORE_API SurfaceNormalMapping : public VectorsForSurfaceMapping
{
    Q_OBJECT

public:
    SurfaceNormalMapping(RenderedData * renderedData);
    ~SurfaceNormalMapping() override;

    QString name() const override;

    reflectionzeug::PropertyGroup * createPropertyGroup() override;

protected:
    void visibilityChangedEvent() override;

private:
    void updateGlyphs();

private:
    NormalType m_normalType;
    bool m_normalTypeChanged;

    static const bool s_registered;
};