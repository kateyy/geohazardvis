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
