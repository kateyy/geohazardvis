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
