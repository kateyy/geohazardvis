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

CORE_API void printPipeline(vtkAlgorithm * pipelineEnd);
CORE_API PrintHelper print(vtkAlgorithm * pipelineEnd);


}

CORE_API std::ostream & operator<<(std::ostream & os, vtkpipelinehelper::PrintHelper pipelinePrintHelper);
