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

#include "vtkpipelinehelper.h"

#include <iostream>

#include <vtkAlgorithm.h>


namespace vtkpipelinehelper
{

void printPipeline(vtkAlgorithm * pipelineEnd)
{
    std::cout << print(pipelineEnd);
}

PrintHelper print(vtkAlgorithm * pipelineEnd)
{
    return PrintHelper{ pipelineEnd };
}

}

std::ostream & operator<<(std::ostream & os, vtkpipelinehelper::PrintHelper pipelinePrintHelper)
{
    auto upstream = pipelinePrintHelper.algorithm;

    unsigned n = 0;
    while (upstream)
    {
        ++n;

        if (upstream->GetNumberOfInputPorts() == 0)
        {
            break;
        }

        upstream = upstream->GetInputAlgorithm();
    }

    upstream = pipelinePrintHelper.algorithm;

    int i = n;
    while (upstream)
    {
        os << '[' << i << '/' << n << "] ";
        --i;

        upstream->Print(os);
        const auto ports = upstream->GetNumberOfInputPorts();
        if (ports == 0)
        {
            break;
        }
        if (ports > 1)
        {
            os << "Warning: Multiple input ports (" << ports <<  "), only following the first one." << std::endl;
        }
        upstream = upstream->GetInputAlgorithm();
    }

    return os;
}
