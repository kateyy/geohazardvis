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

#include "RawVectorData.h"

#include <QDebug>

#include <vtkCommand.h>
#include <vtkFloatArray.h>

#include <core/types.h>
#include <core/table_model/QVtkTableModelRawVector.h>


RawVectorData::RawVectorData(const QString & name, vtkFloatArray & dataArray)
    : DataObject(name, nullptr)
    , m_dataArray{ &dataArray }
{
    dataArray.SetName(name.toUtf8().data());

    connectObserver("dataChanged", dataArray, vtkCommand::ModifiedEvent,
        *this, &RawVectorData::signal_dataChanged);
}

RawVectorData::~RawVectorData() = default;

std::unique_ptr<DataObject> RawVectorData::newInstance(const QString & name, vtkDataSet * /*dataSet*/) const
{
    qDebug() << "Call to unsupported newInstance function in RawVectorData, "
        "trying to create data object named " + name;
    return{};
}

bool RawVectorData::is3D() const
{
    return false;
}

IndexType RawVectorData::defaultAttributeLocation() const
{
    return IndexType::invalid;
}

const QString & RawVectorData::dataTypeName() const
{
    return dataTypeName_s();
}

const QString & RawVectorData::dataTypeName_s()
{
    static const QString name{ "Raw Data Vector" };
    return name;
}

vtkFloatArray * RawVectorData::dataArray()
{
    return m_dataArray;
}

std::unique_ptr<QVtkTableModel> RawVectorData::createTableModel()
{
    auto model = std::make_unique<QVtkTableModelRawVector>();
    model->setDataObject(this);

    return std::move(model);
}
