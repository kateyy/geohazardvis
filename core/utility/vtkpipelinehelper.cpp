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
#include <vector>

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
    std::vector<vtkAlgorithm *> algorithms;

    for (auto upstream = pipelinePrintHelper.algorithm;
        upstream != nullptr;)
    {
        algorithms.push_back(upstream);

        if (upstream->GetNumberOfInputPorts() == 0)
        {
            break;
        }

        upstream = upstream->GetInputAlgorithm();
    }

    for (size_t i = 0; i < algorithms.size(); ++i)
    {
        os << '[' << i + 1 << '/' << algorithms.size() << "] ";

        auto upstream = algorithms[algorithms.size() - 1u - i];

        upstream->Print(os);

        const auto ports = upstream->GetNumberOfInputPorts();
        if (ports > 1)
        {
            os << "Warning: Multiple input ports (" << ports <<  "), only following the first one." << std::endl;
        }
    }

    return os;
}
