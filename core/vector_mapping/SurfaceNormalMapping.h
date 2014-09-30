#pragma once

#include <core/vector_mapping/VectorsForSurfaceMapping.h>


class CORE_API SurfaceNormalMapping : public VectorsForSurfaceMapping
{
    Q_OBJECT

public:
    SurfaceNormalMapping(RenderedData * renderedData);
    ~SurfaceNormalMapping() override;

    QString name() const override;

private:
    static const bool s_registered;
};
