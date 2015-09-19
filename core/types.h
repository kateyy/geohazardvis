#pragma once


enum class ContentType
{
    Rendered3D,
    Rendered2D,
    Context2D,
    invalid
};

/** Specify whether indices are related to points or to cells in the data set */
enum class IndexType
{
    points,
    cells,
};
