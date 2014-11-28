#pragma once

#include <vtkCollection.h>

#include <core/core_api.h>


class vtkAbstractContextItem;


class CORE_API vtkContextItemCollection : public vtkCollection
{
public:
    static vtkContextItemCollection *New();
    vtkTypeMacro(vtkContextItemCollection, vtkCollection);

    void AddItem(vtkAbstractContextItem *a);
    vtkAbstractContextItem *GetNextContextItem();
    vtkAbstractContextItem *GetLastContextItem();

    /** Reentrant safe way to get an object in a collection. Just pass the
        same cookie back and forth. */
    vtkAbstractContextItem *GetNextContextItem(vtkCollectionSimpleIterator &cookie);

protected:
    vtkContextItemCollection();
    ~vtkContextItemCollection() override;

private:
    // hide the standard AddItem from the user and the compiler.
    void AddItem(vtkObject *o) = delete;

private:
    vtkContextItemCollection(const vtkContextItemCollection&) = delete;
    void operator=(const vtkContextItemCollection&) = delete;
};
