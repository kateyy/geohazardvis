#include "vtkInformationCharVectorKey.h"

#include "vtkInformation.h"

#include <algorithm>
#include <vector>

#include <QString>


vtkInformationCharVectorKey::vtkInformationCharVectorKey(
    const char * name, const char* location, int length)
    : vtkInformationKey(name, location)
    , RequiredLength(length)
{
}

vtkInformationCharVectorKey::~vtkInformationCharVectorKey() = default;

void vtkInformationCharVectorKey::SetQString(vtkInformation * info, const QString & qString)
{
    const auto data = qString.toUtf8();
    if (qString.isEmpty())
    {
        this->Set(info);
        return;
    }
    this->Set(info, data.data(), data.size());
}

QString vtkInformationCharVectorKey::GetQString(vtkInformation * info)
{
    if (!this->Has(info))
    {
        return{};
    }

    return QString::fromUtf8(this->Get(info), this->Length(info));
}

vtkInformationCharVectorKey * vtkInformationCharVectorKey::MakeKey(const char * name, const char * location)
{
    return new vtkInformationCharVectorKey(name, location);
}

void vtkInformationCharVectorKey::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);
}

class vtkInformationCharVectorValue : public vtkObjectBase
{
public:
    vtkBaseTypeMacro(vtkInformationCharVectorValue, vtkObjectBase);
    std::vector<char> Value;
};

void vtkInformationCharVectorKey::Append(vtkInformation * info, char value)
{
    if (const auto v = static_cast<vtkInformationCharVectorValue *>(this->GetAsObjectBase(info)))
    {
        v->Value.push_back(value);
    }
    else
    {
        this->Set(info, &value, 1);
    }
}

void vtkInformationCharVectorKey::Set(vtkInformation * info)
{
    char someVal;
    this->Set(info, &someVal, 0);
}

void vtkInformationCharVectorKey::Set(vtkInformation * info, const char * value, int length)
{
    if (!value)
    {
        this->SetAsObjectBase(info, 0);
        return;
    }

    if (this->RequiredLength >= 0 && length != this->RequiredLength)
    {
        vtkErrorWithObjectMacro(
            info,
            "Cannot store integer vector of length " << length
            << " with key " << this->Location << "::" << this->Name
            << " which requires a vector of length "
            << this->RequiredLength << ".  Removing the key instead.");
        this->SetAsObjectBase(info, 0);
        return;
    }

    const auto oldv = static_cast<vtkInformationCharVectorValue *>(this->GetAsObjectBase(info));
    if (oldv && static_cast<int>(oldv->Value.size()) == length)
    {
        // Replace the existing value.
        std::copy(value, value + length, oldv->Value.begin());
        // Since this sets a value without call SetAsObjectBase(),
        // the info has to be modified here (instead of
        // vtkInformation::SetAsObjectBase()
        info->Modified(this);
    }
    else
    {
        // Allocate a new value.
        auto v = new vtkInformationCharVectorValue();
        v->InitializeObjectBase();
        v->Value.insert(v->Value.begin(), value, value + length);
        this->SetAsObjectBase(info, v);
        v->Delete();
    }
}

char * vtkInformationCharVectorKey::Get(vtkInformation * info)
{
    const auto v = static_cast<vtkInformationCharVectorValue *>(this->GetAsObjectBase(info));
    return (v && !v->Value.empty()) ? (&v->Value[0]) : 0;
}

char vtkInformationCharVectorKey::Get(vtkInformation * info, int idx)
{
    if (idx >= this->Length(info))
    {
        vtkErrorWithObjectMacro(info,
            "Information does not contain " << idx
            << " elements. Cannot return information value.");
        return 0;
    }
    return this->Get(info)[idx];
}


void vtkInformationCharVectorKey::Get(vtkInformation * info, char * value)
{
    const auto v = static_cast<vtkInformationCharVectorValue *>(this->GetAsObjectBase(info));
    if (v && value)
    {
        for (std::vector<char>::size_type i = 0; i < v->Value.size(); ++i)
        {
            value[i] = v->Value[i];
        }
    }
}

int vtkInformationCharVectorKey::Length(vtkInformation * info)
{
    const auto v = static_cast<vtkInformationCharVectorValue *>(this->GetAsObjectBase(info));
    return v ? static_cast<int>(v->Value.size()) : 0;
}

void vtkInformationCharVectorKey::ShallowCopy(vtkInformation * from, vtkInformation * to)
{
    this->Set(to, this->Get(from), this->Length(from));
}

void vtkInformationCharVectorKey::Print(ostream & os, vtkInformation * info)
{
    if (!this->Has(info))
    {
        return;
    }
    const auto value = this->Get(info);
    const int length = this->Length(info);
    const char * sep = "";
    for (int i = 0; i < length; ++i)
    {
        os << sep << value[i];
        sep = " ";
    }
}

char * vtkInformationCharVectorKey::GetWatchAddress(vtkInformation* info)
{
    const auto v = static_cast<vtkInformationCharVectorValue*>(this->GetAsObjectBase(info));
    return (v && !v->Value.empty()) ? (&v->Value[0]) : 0;
}
