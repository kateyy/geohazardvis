#pragma once

#include <vtkInformationIntegerKey.h>

#include <core/core_api.h>

class CORE_API vtkInformationIntegerMetaDataKey : public vtkInformationIntegerKey
{
public:
    vtkTypeMacro(vtkInformationIntegerMetaDataKey, vtkInformationIntegerKey);

    vtkInformationIntegerMetaDataKey(const char * name, const char * location);
    ~vtkInformationIntegerMetaDataKey() override;

    static vtkInformationIntegerMetaDataKey * MakeKey(const char * name, const char * location);

    /* Shallow copies the key from fromInfo to toInfo if request has the REQUEST_INFORMATION() key.
    * This is used by the pipeline to propagate this key downstream. */
    void CopyDefaultInformation(vtkInformation * request,
        vtkInformation * fromInfo, vtkInformation * toInfo) override;

private:
    vtkInformationIntegerMetaDataKey(const vtkInformationIntegerMetaDataKey &) = delete;
    void operator=(const vtkInformationIntegerMetaDataKey &) = delete;
};
