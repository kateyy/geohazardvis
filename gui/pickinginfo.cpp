#include "pickinginfo.h"

#include <QTextStream>
#include <QStringList>

#include <vtkPointPicker.h>
#include <vtkAbstractMapper3D.h>
#include <vtkInformation.h>
#include <vtkInformationStringKey.h>

#include <vtkDataSet.h>
#include <vtkCommand.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkIdTypeArray.h>

#include "core/input.h"

#define VTK_CREATE(type, name) \
    vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

void PickingInfo::sendPointInfo(vtkSmartPointer<vtkPointPicker> picker, bool mouseClick) const
{
    double* pos = picker->GetPickPosition();

    QString content;
    QTextStream stream;
    stream.setString(&content);

    stream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
    stream.setRealNumberPrecision(17);

    std::string inputname;

    vtkAbstractMapper3D * mapper = picker->GetMapper();

    if (!mapper) {
        emit infoSent(QStringList());
        return;
    }
    
    vtkInformation * inputInfo = mapper->GetInformation();

    if (inputInfo->Has(Input::NameKey()))
        inputname = Input::NameKey()->Get(inputInfo);

    stream
        << "input file: " << QString::fromStdString(inputname) << endl
        << "selected point: " << endl
        << pos[0] << endl
        << pos[1] << endl
        << pos[2] << endl
        << "id: " << picker->GetPointId();

    QStringList info;
    QString line;
    while ((line = stream.readLine()) != QString())
        info.push_back(line);

    emit infoSent(info);

    if (!mouseClick)
        return;

    vtkIdType ptIndex = picker->GetPointId();

    if (ptIndex == -1)
        return;

    VTK_CREATE(vtkSelection, selection);
    VTK_CREATE(vtkSelectionNode, node);
    VTK_CREATE(vtkIdTypeArray, indices);
    indices->InsertValue(0, ptIndex);
    node->SetContentType(vtkSelectionNode::INDICES);
    node->SetSelectionList(indices);
    selection->AddNode(node);

    emit selectionChanged(picker->GetDataSet(), vtkCommand::SelectionChangedEvent, nullptr, (void*)selection, nullptr);
}
