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
