#pragma once

#include <vtkSmartPointer.h>

#include <cstdint>

class vtkPolyDataMapper;

class DataGenerator
{
public:
    vtkSmartPointer<vtkPolyDataMapper> generate(uint32_t gridSize, float xzScale, float yScale) const;

};