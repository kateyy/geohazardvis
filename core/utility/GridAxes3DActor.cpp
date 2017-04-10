#include "GridAxes3DActor.h"

#include <vtkObjectFactory.h>


vtkStandardNewMacro(GridAxes3DActor);


GridAxes3DActor::GridAxes3DActor()
    : Superclass()
    , LabelsVisible{ true }
    , RequestedLabelMask{ 0u }
{
    this->RequestedLabelMask = this->GetLabelMask();
}

GridAxes3DActor::~GridAxes3DActor() = default;

void GridAxes3DActor::SetLabelsVisible(const bool visible)
{
    if (visible == this->LabelsVisible)
    {
        return;
    }

    this->LabelsVisible = visible;

    this->Superclass::SetLabelMask(this->LabelsVisible
        ? this->RequestedLabelMask
        : 0u);

    this->Modified();
}

void GridAxes3DActor::SetLabelMask(const unsigned int mask)
{
    this->RequestedLabelMask = mask;

    if (this->LabelsVisible)
    {
        this->Superclass::SetLabelMask(this->RequestedLabelMask);
    }
}
