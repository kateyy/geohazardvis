#include "vtkContextItemCollection.h"

#include <vtkObjectFactory.h>
#include <vtkAbstractContextItem.h>


vtkStandardNewMacro(vtkContextItemCollection);


vtkContextItemCollection::vtkContextItemCollection()
    : vtkCollection()
{
}

vtkContextItemCollection::~vtkContextItemCollection() = default;

vtkAbstractContextItem * vtkContextItemCollection::GetNextContextItem(vtkCollectionSimpleIterator &cookie)
{
    return static_cast<vtkAbstractContextItem *>(this->GetNextItemAsObject(cookie));
};

void vtkContextItemCollection::AddItem(vtkAbstractContextItem *a)
{
    this->vtkCollection::AddItem(a);
}

vtkAbstractContextItem *vtkContextItemCollection::GetNextContextItem()
{
    return static_cast<vtkAbstractContextItem *>(this->GetNextItemAsObject());
}

vtkAbstractContextItem *vtkContextItemCollection::GetLastContextItem()
{
    if (!this->Bottom)
    {
        return nullptr;
    }
    else
    {
        return static_cast<vtkAbstractContextItem *>(this->Bottom->Item);
    }
}
