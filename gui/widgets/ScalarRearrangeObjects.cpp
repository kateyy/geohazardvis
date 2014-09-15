#include "ScalarRearrangeObjects.h"

#include <core/data_objects/DataObject.h>
#include <core/scalar_mapping/ScalarsForColorMapping.h>


ScalarRearrangeObjects::ScalarRearrangeObjects(QWidget * parent, Qt::WindowFlags f)
    : QDialog(parent, f)
{
    m_ui.setupUi(this);
}

void ScalarRearrangeObjects::rearrange(QWidget * parent, ScalarsForColorMapping * scalars)
{
    ScalarRearrangeObjects w(parent);

    vtkIdType oldStartingIndex = scalars->startingIndex();
    
    auto slider = w.m_ui.firstIndexSlider;
    slider->setMaximum(scalars->maximumStartingIndex());
    connect(slider, &QAbstractSlider::valueChanged, scalars, &ScalarsForColorMapping::setStartingIndex);

    for (const DataObject * dataObject : scalars->dataObjects())
    {
        QListWidgetItem * item = new QListWidgetItem(dataObject->name());
        item->setData(Qt::UserRole, reinterpret_cast<unsigned long long>(dataObject));
        w.m_ui.objectList->addItem(item);
    }

    if (!w.exec())
    {
        scalars->setStartingIndex(oldStartingIndex);
        return;
    }

    QList<DataObject *> newList;
    for (int i = 0; i < w.m_ui.objectList->count(); ++i)
        newList << reinterpret_cast<DataObject *>(w.m_ui.objectList->item(i)->data(Qt::UserRole).toULongLong());

    scalars->rearrangeDataObjets(newList);
}
