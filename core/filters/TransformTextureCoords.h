#pragma once

#include <vtkTransformTextureCoords.h>

#include <core/core_api.h>


class CORE_API TransformTextureCoords : public vtkTransformTextureCoords
{
public:
    vtkTypeMacro(TransformTextureCoords, vtkTransformTextureCoords);
    static TransformTextureCoords * New();

protected:
    TransformTextureCoords() = default;

    int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
    TransformTextureCoords(const TransformTextureCoords&) = delete;
    void operator=(const TransformTextureCoords&) = delete;
};
