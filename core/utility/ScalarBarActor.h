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

#include <core/ThirdParty/ParaView/vtkPVScalarBarActor.h>


class CORE_API OrientedScalarBarActor : public vtkPVScalarBarActor
{
public:
    vtkTypeMacro(OrientedScalarBarActor, vtkPVScalarBarActor);
    void PrintSelf(ostream &os, vtkIndent indent) override;
    static OrientedScalarBarActor * New();

    vtkGetMacro(TitleAlignedWithColorBar, bool);
    vtkSetMacro(TitleAlignedWithColorBar, bool)

protected:
    OrientedScalarBarActor();
    ~OrientedScalarBarActor();

    void LayoutTitle() override;
    void ConfigureTitle() override;

protected:
    bool TitleAlignedWithColorBar;

private:
    OrientedScalarBarActor(const OrientedScalarBarActor &) = delete;
    void operator=(const OrientedScalarBarActor &) = delete;
};
