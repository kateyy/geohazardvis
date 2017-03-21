#pragma once

#include <core/filters/vtkInformationCharVectorKey.h>


class CORE_API vtkInformationCharVectorMetaDataKey : public vtkInformationCharVectorKey
{
public:
    vtkTypeMacro(vtkInformationCharVectorMetaDataKey, vtkInformationCharVectorKey);

    vtkInformationCharVectorMetaDataKey(const char * name, const char * location);
    ~vtkInformationCharVectorMetaDataKey() override;

    static vtkInformationCharVectorMetaDataKey * MakeKey(const char * name, const char * location);

    /* Shallow copies the key from fromInfo to toInfo if request has the REQUEST_INFORMATION() key.
     * This is used by the pipeline to propagate this key downstream. */
    void CopyDefaultInformation(vtkInformation * request,
        vtkInformation * fromInfo, vtkInformation * toInfo) override;

private:
    vtkInformationCharVectorMetaDataKey(const vtkInformationCharVectorMetaDataKey &) = delete;
    void operator=(const vtkInformationCharVectorMetaDataKey &) = delete;
};
