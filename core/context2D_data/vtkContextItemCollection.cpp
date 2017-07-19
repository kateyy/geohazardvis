/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

    return static_cast<vtkAbstractContextItem *>(this->Bottom->Item);
}
