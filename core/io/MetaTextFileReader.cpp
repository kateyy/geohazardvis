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

#include "MetaTextFileReader.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <fstream>
#include <functional>
#include <list>
#include <limits>
#include <locale>
#include <map>
#include <sstream>

#include <vtkImageData.h>

#include <core/data_objects/DataObject.h>
#include <core/io/TextFileReader.h>
#include <core/io/MatricesToVtk.h>


using namespace io;
using std::ifstream;
using std::map;
using std::stringstream;
using std::string;
using std::vector;


namespace
{
const map<string, DataSetType> datasetNamesTypes = {
        { "vertices", DataSetType::vertices },
        { "indices", DataSetType::indices },
        { "grid2d", DataSetType::grid2D },
        { "vectors", DataSetType::vectors },
        { "vectorGrid3D", DataSetType::vectorGrid3D } };
const map<string, ModelType> modelNamesType = {
        { "triangles", ModelType::triangles },
        { "DEM", ModelType::DEM },
        { "grid2d", ModelType::grid2D },
        { "vectorGrid3D", ModelType::vectorGrid3D } };


bool ignoreLine(const string & line)
{
    return line.empty() || line[0] == '#';
}

string toLower(const string & s)
{
    string result(s.size(), 0);
    std::transform(s.begin(), s.end(), result.begin(), ::tolower);
    return result;
}

// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
string rtrim(const string & s)
{
    return string(s.begin(),
        std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base());
}

}

MetaTextFileReader::InputFileInfo::InputFileInfo(const QString & name, ModelType type)
    : name{ name }
    , type{ type }
{
}

MetaTextFileReader::InputFileInfo::InputFileInfo(InputFileInfo && other)
    : InputFileInfo(other.name, other.type)
{
}

std::unique_ptr<DataObject> MetaTextFileReader::read(const QString & fileName)
{
    std::vector<ReadDataSet> readDatasets;
    auto inputInfo = MetaTextFileReader::readData(fileName, readDatasets);
    if (!inputInfo)
    {
        return nullptr;
    }

    switch (inputInfo->type)
    {
    case ModelType::triangles:
        return MatricesToVtk::loadIndexedTriangles(inputInfo->name, readDatasets);
    case ModelType::DEM:
    case ModelType::grid2D:
        return MatricesToVtk::loadGrid2D(inputInfo->name, readDatasets);
    case ModelType::vectorGrid3D:
        return MatricesToVtk::loadGrid3D(inputInfo->name, readDatasets);
    case ModelType::raw:
        return MatricesToVtk::readRawFile(fileName);
    default:
        cerr << "Warning: model type unsupported by the loader: " << int(inputInfo->type) << endl;
        return nullptr;
    }
}

auto MetaTextFileReader::readData(
    const QString & fileName,
    vector<ReadDataSet> & readDataSets) -> std::unique_ptr<InputFileInfo>
{
    ifstream inputStream(fileName.toStdString());

    if (!inputStream.good())
    {
        cerr << R"(Cannot access file: ")" << fileName.toStdString() << '\"' << endl;
        return nullptr;
    }

    vector<DataSetDef> datasetDefs;
    auto input = readHeader(inputStream, datasetDefs);

    if (!input)
    {
        cerr << R"(could not read input text file: ")" << fileName.toStdString() << '\"' << endl;
        return nullptr;
    }
    assert(input);

    auto reader = TextFileReader(fileName);
    reader.seekTo(static_cast<uint64_t>(inputStream.tellg()));
    if (!reader.stateFlags().testFlag(TextFileReader::successful))
    {
        cerr << "could not read input data set in " << fileName.toStdString() << endl;
        return input;
    }

    for (const auto & dataSetDef : datasetDefs)
    {
        ReadDataSet readData{ dataSetDef.type, {}, dataSetDef.attributeName, dataSetDef.vtkMetaData };

        reader.read(readData.data, dataSetDef.nbLines);

        if (reader.stateFlags().testFlag(TextFileReader::successful)
            && readData.data.size() == dataSetDef.nbColumns
            && readData.data.front().size() == dataSetDef.nbLines)
        {
            readDataSets.push_back(std::move(readData));
        }
        else
        {
            cerr << "could not read input data set in " << fileName.toStdString() << endl;
        }
    }
    return input;
}

auto MetaTextFileReader::readHeader(ifstream & inputStream, vector<DataSetDef> & inputDefs)
    -> std::unique_ptr<InputFileInfo>
{
    assert(inputStream.good());

    bool validFile = false;
    bool started = false;
    std::unique_ptr<InputFileInfo> input;

    string line;


    while (!inputStream.eof())
    {
        std::getline(inputStream, line);
        line = rtrim(line);

        if (ignoreLine(line))
        {
            continue;
        }

        if (line == "$begin")
        {
            started = true;
            continue;
        }

        if (!started) {
            //cerr << "invalid input file (does not begin with the $begin tag)." << endl;
            return std::make_unique<InputFileInfo>("", ModelType::raw);
        }

        // define the current 3d/2d model
        if (line.substr(0, 8) == "$ model ") {
            if (input) {
                cerr << "multiple models per file not supported." << endl;
                return nullptr;
            }
            assert(!input);
            stringstream linestream(line.substr(8, string::npos));
            string type, name;
            std::getline(linestream, type, ' ');
            std::getline(linestream, name);

            auto it = modelNamesType.find(type);
            if (it == modelNamesType.end())
            {
                cerr << R"(Invalid model type ")" << type << '\"' << endl;
                return nullptr;
            }
            input = std::make_unique<InputFileInfo>(QString::fromStdString(name), it->second);

            switch (input->type)
            {
            case ModelType::triangles:
                validFile = readHeader_triangles(inputStream, inputDefs);
                break;
            case ModelType::DEM:
                validFile = readHeader_DEM(inputStream, inputDefs);
                break;
            case ModelType::grid2D:
                validFile = readHeader_grid2D(inputStream, inputDefs);
                break;
            case ModelType::vectorGrid3D:
                validFile = readHeader_vectorGrid3D(inputStream, inputDefs);
                break;
            default:
                cerr << R"(Unexpected model type ")" << type << '\"' << endl;
                return nullptr;
            }

            if (!validFile)
            {
                return nullptr;
            }

            break;
        }

        cerr << "Invalid line in input file: \n\t" << line << endl;
    }

    assert(validFile && !inputDefs.empty());

    return input;
}

bool MetaTextFileReader::readHeader_triangles(ifstream & inputStream, vector<DataSetDef> & inputDefs)
{
    string line;
    DataSetType currentDataType(DataSetType::unknown);
    size_t numCells = 0;

    while (!inputStream.eof())
    {
        std::getline(inputStream, line);
        line = rtrim(line);

        if (ignoreLine(line))
        {
            continue;
        }

        // this is the end if the header section, required for valid input files
        if (line == "$end")
        {
            return true;
        }

        // expecting only some types of datasets from now on
        if (line.substr(0, 2) != "$ ")
        {
            cerr << "Invalid line in input file: \n\t" << line << endl;
            return false;
        }

        stringstream linestream(line.substr(2, string::npos));
        string datasetType, parameter;
        std::getline(linestream, datasetType, ' ');
        std::getline(linestream, parameter);

        string attributeName;

        currentDataType = checkDataSetType(datasetType);

        size_t tupleSize = 0;
        size_t numTuples = 0;
        switch (currentDataType)
        {
        case DataSetType::indices:
            tupleSize = 3;
            numTuples = stoul(parameter);
            numCells = numTuples;
            break;
        case DataSetType::vertices:
            tupleSize = 4;
            numTuples = stoul(parameter);
            break;
        case DataSetType::vectors:
        {
            const size_t tupleSizePos = parameter.find(' ', 0);
            tupleSize = stoul(parameter.substr(0, tupleSizePos));
            numTuples = numCells;
            attributeName = parameter.substr(tupleSizePos + 1);
            break;
        }
        case DataSetType::unknown:
            return false;
        default:
            cerr << R"(Data set type ")" + datasetType + R"(" not supported with model type "triangles")" << endl;
            return false;
        }
        assert(tupleSize);
        assert(numTuples);
        inputDefs.push_back({ currentDataType, numTuples, tupleSize, QString::fromStdString(attributeName), nullptr });
    }

    return false;
}

bool MetaTextFileReader::readHeader_DEM(ifstream & inputStream, vector<DataSetDef>& inputDefs)
{
    string line;
    int columns = -1, rows = -1;
    double xMin, xMax, yMin, yMax, cellSize;
    xMin = xMax = yMin = yMax = cellSize = nan("");

    bool atEnd = false;

    while (!inputStream.eof())
    {
        std::getline(inputStream, line);
        line = rtrim(line);

        if (ignoreLine(line))
        {
            continue;
        }

        if (line == "$end")
        {
            atEnd = true;
            break;
        }

        if (!(line.substr(0, 2) == "$ "))
        {
            cerr << "Invalid line in input file: \n\t" << line << endl;
            return false;
        }

        stringstream linestream(line.substr(2, string::npos));
        string parameter, value;
        std::getline(linestream, parameter, ' ');
        std::getline(linestream, value);
        parameter = toLower(parameter);
        value = value.substr(value.find_first_not_of(' '), string::npos);
        replace(value.begin(), value.end(), ',', '.');

        if (parameter == "ncols")
        {
            columns = stoi(value);
            continue;
        }
        if (parameter == "nrows")
        {
            rows = stoi(value);
            continue;
        }
        if (parameter == "xllcorner" || parameter == "xmin")
        {
            xMin = stod(value);
            continue;
        }
        if (parameter == "yllcorner" || parameter == "ymin")
        {
            yMin = stod(value);
            continue;
        }
        if (parameter == "xmax")
        {
            xMax = stod(value);
            continue;
        }
        if (parameter == "ymax")
        {
            yMax = stod(value);
            continue;
        }
        if (parameter == "cellsize")
        {
            cellSize = stod(value);
            continue;
        }
    }

    if (!atEnd)
    {
        return false;
    }

    if (columns <= 0 || rows <= 0 || std::isnan(xMin) || std::isnan(yMin))
    {
        return false;
    }

    auto image = vtkSmartPointer<vtkImageData>::New();
    image->SetExtent(0, columns - 1, 0, rows - 1, 0, 0);

    // either define xmax and ymax or cellSize
    if (std::isnan(cellSize))
    {
        if (std::isnan(xMax) || std::isnan(yMax))
        {
            return false;
        }

        image->SetOrigin(xMin, yMin, 0);
        image->SetSpacing(
            columns > 1 ? (xMax - xMin) / (columns - 1) : 1,
            rows > 1 ? (yMax - yMin) / (rows - 1) : 1,
            1);
    }
    else
    {
        if (!std::isnan(xMax) || !std::isnan(yMax))
        {
            return false;
        }

        image->SetOrigin(xMin, yMin, 0);
        image->SetSpacing(cellSize, cellSize, 1);
    }

    DataSetDef def;
    def.type = DataSetType::grid2D;
    def.nbColumns = columns;
    def.nbLines = rows;
    def.vtkMetaData = image;

    inputDefs.push_back(def);

    return true;
}

bool MetaTextFileReader::readHeader_grid2D(ifstream & inputStream, vector<DataSetDef> & inputDefs)
{
    string line;
    auto image = vtkSmartPointer<vtkImageData>::New();
    DataSetDef dataSetDef{ DataSetType::unknown, 0, 0, "", image };
    double extent[4] = { 1, -1, 1, -1 };  // optional extent defines, combined with the nbRows/lines, the origin and spacing

    bool atEnd = false;

    while (!inputStream.eof())
    {
        std::getline(inputStream, line);
        line = rtrim(line);

        if (ignoreLine(line))
        {
            continue;
        }

        if (line == "$end")
        {
            atEnd = true;
            break;
        }

        stringstream linestream(line);
        string indicator, paramName, paramValue;
        std::getline(linestream, indicator, ' ');
        std::getline(linestream, paramName, ' ');
        std::getline(linestream, paramValue);
        paramName = toLower(paramName);

        if (indicator == "$")
        {
            if (dataSetDef.type != DataSetType::unknown)
            {
                cerr << "Multiple data sets per grid2d file currently not supported." << endl;
                return false;
            }

            dataSetDef.type = checkDataSetType(paramName);

            if (dataSetDef.type != DataSetType::grid2D)
            {
                cerr << "Data set type " << paramName << " not supported in model type grid2d" << endl;
                return false;
            }

            const size_t seperator = paramValue.find_first_of(':');
            const size_t columns = stoul(paramValue.substr(0, seperator));
            const size_t rows = stoul(paramValue.substr(seperator + 1, string::npos));

            if (!(columns > 0 && rows > 0))
            {
                cerr << R"(missing "columns:rows" specification for grid2d data set)" << endl;
                return false;
            }

            dataSetDef.nbLines = rows;
            dataSetDef.nbColumns = columns;

            continue;
        }

        if (indicator != "$$")
        {
            cerr << "Invalid line in input file: \n\t" << line << endl;
            return false;
        }

        const size_t seperator = paramValue.find_first_of(':');
        double val1 = stod(paramValue.substr(0, seperator));
        double val2 = stod(paramValue.substr(seperator + 1, string::npos));

        if (paramName == "xextent")
        {
            extent[0] = val1;
            extent[1] = val2;
        }
        else if (paramName == "yextent")
        {
            extent[2] = val1;
            extent[3] = val2;
        }
        else
        {
            cerr << "Invalid parameter in input file: " << paramName << endl;
            return false;
        }
    }

    if (!atEnd)
    {
        return false;
    }

    image->SetExtent(0, static_cast<int>(dataSetDef.nbColumns) - 1, 0, static_cast<int>(dataSetDef.nbLines) - 1, 0, 0);

    double spacing[3] = { 1, 1, 1 };

    if (dataSetDef.nbColumns > 1 && extent[0] <= extent[1])
    {
        spacing[0] = (extent[1] - extent[0]) / double(dataSetDef.nbColumns - 1);
    }
    if (dataSetDef.nbLines > 1 && extent[2] <= extent[3])
    {
        spacing[1] = (extent[3] - extent[2]) / double(dataSetDef.nbLines - 1);
    }

    image->SetSpacing(spacing);

    double origin[3] = { 0, 0, 0 };
    if (extent[0] <= extent[1])
    {
        origin[0] = extent[0];
    }
    if (extent[2] <= extent[3])
    {
        origin[1] = extent[2];
    }
    image->SetOrigin(origin);

    inputDefs.push_back(dataSetDef);

    return true;
}

bool MetaTextFileReader::readHeader_vectorGrid3D(ifstream & inputStream, vector<DataSetDef>& inputDefs)
{
    string line;
    DataSetType currentDataType(DataSetType::unknown);

    while (!inputStream.eof())
    {
        std::getline(inputStream, line);
        line = rtrim(line);

        if (ignoreLine(line))
        {
            continue;
        }

        if (line == "$end")
        {
            return true;
        }

        if (!(line.substr(0, 2) == "$ "))
        {
            cerr << "Invalid line in input file: \n\t" << line << endl;
            return false;
        }

        stringstream linestream(line.substr(2, string::npos));
        string datasetType, parameter, parameter2;
        std::getline(linestream, datasetType, ' ');
        std::getline(linestream, parameter, ' ');
        std::getline(linestream, parameter2);

        currentDataType = checkDataSetType(datasetType);

        if (currentDataType != DataSetType::vectorGrid3D)
        {
            cerr << "Data set type " << datasetType << " not supported in model type vectorGrid3D" << endl;
            return false;
        }

        size_t rows = stoul(parameter);
        size_t columns = parameter2.empty() ? 6u : 3u + stoul(parameter2);
        inputDefs.push_back({ currentDataType, rows, columns, "", nullptr});
    }

    return false;
}

DataSetType MetaTextFileReader::checkDataSetType(const string & nameString)
{
    auto it = datasetNamesTypes.find(nameString);
    if (it == datasetNamesTypes.end())
    {
        cerr << "Invalid data set type: " << nameString << endl;
        return DataSetType::unknown;
    }
    return it->second;
}
