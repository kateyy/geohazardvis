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
