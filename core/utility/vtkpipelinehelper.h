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

#include <iosfwd>

#include <core/core_api.h>


class vtkAlgorithm;


namespace vtkpipelinehelper
{

struct PrintHelper
{
    vtkAlgorithm * algorithm;
};

/**
 * Print the upstream pipeline of the supplied algorithm.
 * 
 * This will traverse upstream algorithms until reaching an algorithm that has no input. From there
 * on up to down, Print(std::cout) is called on all algorithms up to the one passed as parameter.
 * This traverses only the input at port 0 of each algorithm. A warning is printed if more inputs
 * exists, but they are not traversed.
 * For example:
 * using namespace vtkpipelinehelper;
 * printPipeline(myAlgorihtm);
 */
CORE_API void printPipeline(vtkAlgorithm * pipelineEnd);
/**
 * Construct a dummy object that allows to print pipeline information to an arbitrary output stream.
 * For example:
 * using namespace vtkpipelinehelper;
 * std::cout << print(myAlgorihtm) << std::endl;
 */
CORE_API PrintHelper print(vtkAlgorithm * pipelineEnd);


}

/**
 * Utility function intended to be used in conjunction with vtkpipelinehelper::print;
 */
CORE_API std::ostream & operator<<(std::ostream & os, vtkpipelinehelper::PrintHelper pipelinePrintHelper);
