#pragma once

#include <vtkInformationKey.h>

#include <core/core_api.h>


class QString;


class CORE_API vtkInformationCharVectorKey : public vtkInformationKey
{
public:
    vtkTypeMacro(vtkInformationCharVectorKey, vtkInformationKey);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    vtkInformationCharVectorKey(const char * name, const char * location, int length = -1);
    ~vtkInformationCharVectorKey() override;

    void SetQString(vtkInformation * info, const QString & qString);
    QString GetQString(vtkInformation * info);

    static vtkInformationCharVectorKey * MakeKey(const char * name, const char * location);

    void Append(vtkInformation * info, char value);
    void Set(vtkInformation * info, const char * value, int length);
    void Set(vtkInformation * info);
    char * Get(vtkInformation * info);
    char Get(vtkInformation * info, int idx);
    void Get(vtkInformation * info, char * value);
    int Length(vtkInformation * info);

    void ShallowCopy(vtkInformation * from, vtkInformation * to) override;

    void Print(ostream & os, vtkInformation * info) override;

protected:
    int RequiredLength;

    char * GetWatchAddress(vtkInformation * info);

private:
    vtkInformationCharVectorKey(const vtkInformationCharVectorKey&) = delete;
    void operator=(const vtkInformationCharVectorKey&) = delete;
};
