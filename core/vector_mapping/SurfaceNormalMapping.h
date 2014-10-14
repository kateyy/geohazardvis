#pragma once

#include <core/vector_mapping/VectorMappingData.h>


class CORE_API SurfaceNormalMapping : public VectorMappingData
{
    Q_OBJECT

public:
    SurfaceNormalMapping(RenderedData * renderedData);
    ~SurfaceNormalMapping() override;

    QString name() const override;

private:
    static const bool s_registered;
};
