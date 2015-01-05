#pragma once

#include <core/context2D_data/vtkContextItemCollection.h>

#include <core/core_api.h>


class vtkPlot;


class CORE_API vtkPlotCollection : public vtkContextItemCollection
{
public:
    static vtkPlotCollection *New();
    vtkTypeMacro(vtkPlotCollection, vtkCollection);

    void AddItem(vtkPlot *a);
    vtkPlot *GetNextPlot();
    vtkPlot *GetLastPlot();

    /** Reentrant safe way to get an object in a collection. Just pass the
        same cookie back and forth. */
    vtkPlot *GetNextPlot(vtkCollectionSimpleIterator &cookie);

protected:
    vtkPlotCollection();
    ~vtkPlotCollection() override;

private:
    // hide the standard AddItem from the user and the compiler.
    void AddItem(vtkObject *o) = delete;

private:
    vtkPlotCollection(const vtkPlotCollection&) = delete;
    void operator=(const vtkPlotCollection&) = delete;
};
