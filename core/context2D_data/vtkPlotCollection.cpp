#include "vtkPlotCollection.h"

#include <vtkObjectFactory.h>
#include <vtkPlot.h>


vtkStandardNewMacro(vtkPlotCollection);


vtkPlotCollection::vtkPlotCollection()
    : vtkContextItemCollection()
{
}

vtkPlotCollection::~vtkPlotCollection() = default;

vtkPlot * vtkPlotCollection::GetNextPlot(vtkCollectionSimpleIterator &cookie)
{
    return static_cast<vtkPlot *>(this->GetNextItemAsObject(cookie));
};

void vtkPlotCollection::AddItem(vtkPlot *a)
{
    this->vtkContextItemCollection::AddItem(a);
}

vtkPlot *vtkPlotCollection::GetNextPlot()
{
    return static_cast<vtkPlot *>(this->GetNextItemAsObject());
}

vtkPlot *vtkPlotCollection::GetLastPlot()
{
    if (!this->Bottom)
    {
        return nullptr;
    }

    return static_cast<vtkPlot *>(this->Bottom->Item);
}
