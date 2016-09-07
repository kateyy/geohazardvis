#pragma once

#include <vtkSmartPointer.h>
#include <vtkTrivialProducer.h>


class InformationSource : public vtkTrivialProducer
{
public:
    vtkTypeMacro(InformationSource, vtkTrivialProducer);

    static InformationSource * New();

    void SetOutInfo(vtkInformation * info);
    vtkInformation * GetOutInfo();

protected:
    InformationSource();
    ~InformationSource() override;

    int ProcessRequest(vtkInformation * request,
        vtkInformationVector ** inVector,
        vtkInformationVector * outVector) override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inVector,
        vtkInformationVector * outVector);

private:
    vtkSmartPointer<vtkInformation> OutInfo;

private:
    InformationSource(const InformationSource &) = delete;
    void operator=(const InformationSource &) = delete;
};


class InformationSink : public vtkAlgorithm
{
public:
    vtkTypeMacro(InformationSink, vtkAlgorithm);

    static InformationSink * New();

    vtkInformation * GetInInfo();

protected:
    InformationSink();
    ~InformationSink() override;

    int FillInputPortInformation(int port, vtkInformation * info) override;

    int ProcessRequest(vtkInformation * request,
        vtkInformationVector ** inVector,
        vtkInformationVector * outVector) override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inVector,
        vtkInformationVector * outVector);

private:
    vtkSmartPointer<vtkInformation> InInfo;

private:
    InformationSink(const InformationSink &) = delete;
    void operator=(const InformationSink &) = delete;
};
