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
#include <memory>

#include <core/io/types.h>


class DataObject;


/** Simple annotated text file format intended for quickly testing new data sets during development. */
class MetaTextFileReader
{
public:
    static std::unique_ptr<DataObject> read(const QString & fileName);

private:
    struct InputFileInfo
    {
        InputFileInfo(const QString & name, const io::ModelType type);

        const QString name;
        const io::ModelType type;

        InputFileInfo(InputFileInfo && other);
        void operator=(const InputFileInfo &) = delete;
    };

    struct DataSetDef
    {
        io::DataSetType type;
        size_t nbLines;
        size_t nbColumns;
        QString attributeName;
        vtkSmartPointer<vtkDataObject> vtkMetaData;
    };

    /// read the file header and leave the input stream at a position directly behind the header end
    /// @return a unique pointer to an InputFileInfo object, if the file contains a valid header
    static std::unique_ptr<InputFileInfo> readHeader(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);

    static bool readHeader_triangles(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);
    static bool readHeader_DEM(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);
    static bool readHeader_grid2D(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);
    static bool readHeader_vectorGrid3D(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);

    static io::DataSetType checkDataSetType(const std::string & nameString);

    static std::unique_ptr<InputFileInfo> readData(
        const QString & fileName,
        std::vector<io::ReadDataSet> & readDataSets);

private:
    MetaTextFileReader() = delete;
    ~MetaTextFileReader() = delete;
};
