#pragma once

#include <core/vector_mapping/VectorsForSurfaceMapping.h>


class CORE_API CellDataVectorMapping : public VectorsForSurfaceMapping
{
    Q_OBJECT

public:
    CellDataVectorMapping(RenderedData * renderedData);
    ~CellDataVectorMapping() override;

    QString name() const override;

protected:
    void initialize() override;

private:
    static const bool s_registered;
};
