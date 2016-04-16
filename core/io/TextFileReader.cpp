#include "TextFileReader.h"

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

#include <core/io/FileParser.h>


using namespace io;
using std::ifstream;
using std::map;
using std::shared_ptr;
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

InputFileInfo::InputFileInfo(const string & name, ModelType type)
    : name(name)
    , type(type)
{
}

shared_ptr<InputFileInfo> TextFileReader::read(const string & filename, vector<ReadDataSet> & readDataSets)
{
    ifstream inputStream(filename);

    if (!inputStream.good())
    {
        cerr << "Cannot access file: \"" << filename << "\"" << endl;
        return nullptr;
    }

    vector<DataSetDef> datasetDefs;
    shared_ptr<InputFileInfo> input = readHeader(inputStream, datasetDefs);

    if (!input)
    {
        cerr << "could not read input text file: \"" << filename << "\"" << endl;
        return nullptr;
    }
    assert(input);

    for (const DataSetDef & datasetDef : datasetDefs)
    {
        ReadDataSet readData{ datasetDef.type, {}, datasetDef.attributeName, datasetDef.vtkMetaData };

        if (FileParser::populateIOVectors(inputStream, readData.data,
            datasetDef.nbLines,
            datasetDef.nbColumns)) {
            readDataSets.push_back(readData);
        }
        else {
            assert(false);
            cerr << "could not read input data set in " << filename << endl;
        }
    }
    return input;
}

shared_ptr<InputFileInfo> TextFileReader::readHeader(ifstream & inputStream, vector<DataSetDef> & inputDefs)
{
    assert(inputStream.good());

    bool validFile = false;
    bool started = false;
    shared_ptr<InputFileInfo> input;

    string line;


    while (!inputStream.eof())
    {
        std::getline(inputStream, line);
        line = rtrim(line);

        if (ignoreLine(line))
            continue;

        if (line == "$begin") {
            started = true;
            continue;
        }

        if (!started) {
            //cerr << "invalid input file (does not begin with the $begin tag)." << endl;
            return std::make_shared<InputFileInfo>("", ModelType::raw);
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
                cerr << "Invalid model type \"" << type << "\"" << endl;
                return nullptr;
            }
            input = std::make_shared<InputFileInfo>(name, it->second);

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
                cerr << "Unexpected model type \"" << type << "\"" << endl;
                return nullptr;
            }

            if (!validFile)
                return nullptr;
            else
                break;

        }

        cerr << "Invalid line in input file: \n\t" << line << endl;
    }

    assert(validFile && !inputDefs.empty());

    return input;
}

bool TextFileReader::readHeader_triangles(ifstream & inputStream, vector<DataSetDef> & inputDefs)
{
    string line;
    DataSetType currentDataType(DataSetType::unknown);
    size_t numCells = 0;

    while (!inputStream.eof())
    {
        std::getline(inputStream, line);
        line = rtrim(line);

        if (ignoreLine(line))
            continue;

        // this is the end if the header section, required for valid input files
        if (rtrim(line) == "$end")
            return true;

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
            size_t tupleSizePos = parameter.find(" ", 0);
            tupleSize = stoul(parameter.substr(0, tupleSizePos));
            numTuples = numCells;
            attributeName = parameter.substr(tupleSizePos + 1);
            break;
        }
        case DataSetType::unknown:
            return false;
        default:
            cerr << "Data set type \"" + datasetType + "\" not supported with model type \"triangles\"" << endl;
            return false;
        }
        assert(tupleSize);
        assert(numTuples);
        inputDefs.push_back({ currentDataType, numTuples, tupleSize, attributeName, nullptr });
    }

    return false;
}

bool TextFileReader::readHeader_DEM(ifstream & inputStream, vector<DataSetDef>& inputDefs)
{
    string line;
    int columns = -1, rows = -1;
    double xMin, xMax, yMin, yMax, nanValue, cellSize;
    xMin = xMax = yMin = yMax = cellSize = nanValue = nan("");

    bool atEnd = false;

    while (!inputStream.eof())
    {
        std::getline(inputStream, line);
        line = rtrim(line);

        if (ignoreLine(line))
            continue;

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
        //if (parameter == "NODATA_value")
        //{
        //    nanValue = stod(value);
        //    continue;
        //}
    }

    if (!atEnd)
        return false;

    if (columns <= 0 || rows <= 0 || isnan(xMin) || isnan(xMax) || cellSize <= 0)
        return false;

    auto image = vtkSmartPointer<vtkImageData>::New();
    image->SetExtent(0, columns - 1, 0, rows - 1, 0, 0);

    // either define xmax and ymax or cellSize
    if (isnan(cellSize))
    {
        if (isnan(xMax) || isnan(yMax))
            return false;

        image->SetOrigin(xMin, yMin, 0);
        image->SetSpacing(
            columns > 1 ? (xMax - xMin) / (columns - 1) : 1,
            rows > 1 ? (yMax - yMin) / (rows - 1) : 1,
            1);
    }
    else
    {
        if (!isnan(xMax) || !isnan(yMax))
            return false;

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

bool TextFileReader::readHeader_grid2D(ifstream & inputStream, vector<DataSetDef> & inputDefs)
{
    string line;
    auto image = vtkSmartPointer<vtkImageData>::New();
    DataSetDef dataSetDef{ DataSetType::unknown, 0, 0, "", image };
    double extent[4] = { 1, -1, 1, -1 };  // optional extent defines, combined with the nbRows/lines, the origin and spacing

    bool atEnd = false;

    while (!inputStream.eof())
    {
        std::getline(inputStream, line);
        rtrim(line);

        if (ignoreLine(line))
            continue;

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

            size_t seperator = paramValue.find_first_of(":");
            size_t columns = stoul(paramValue.substr(0, seperator));
            size_t rows = stoul(paramValue.substr(seperator + 1, string::npos));

            if (!(columns > 0 && rows > 0))
            {
                cerr << "missing \"columns:rows\" specification for grid2d data set" << endl;
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

        size_t seperator = paramValue.find_first_of(":");
        double val1 = stod(paramValue.substr(0, seperator));
        double val2 = stod(paramValue.substr(seperator + 1, string::npos));
        
        if (paramName == "xExtent")
        {
            extent[0] = val1;
            extent[1] = val2;
        }
        else if (paramName == "yExtent")
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
        return false;

    image->SetExtent(0, static_cast<int>(dataSetDef.nbColumns) - 1, 0, static_cast<int>(dataSetDef.nbLines) - 1, 0, 0);

    double spacing[3] = { 1, 1, 1 };

    if (dataSetDef.nbColumns > 1 && extent[0] <= extent[1])
        spacing[0] = (extent[1] - extent[0]) / double(dataSetDef.nbColumns - 1);
    if (dataSetDef.nbLines > 1 && extent[2] <= extent[3])
        spacing[1] = (extent[3] - extent[2]) / double(dataSetDef.nbLines - 1);

    image->SetSpacing(spacing);

    double origin[3] = { 0, 0, 0 };
    if (extent[0] <= extent[1])
        origin[0] = extent[0];
    if (extent[2] <= extent[3])
        origin[1] = extent[2];
    image->SetOrigin(origin);

    inputDefs.push_back(dataSetDef);

    return true;
}

bool TextFileReader::readHeader_vectorGrid3D(ifstream & inputStream, vector<DataSetDef>& inputDefs)
{
    string line;
    DataSetType currentDataType(DataSetType::unknown);

    while (!inputStream.eof())
    {
        std::getline(inputStream, line);
        line = rtrim(line);

        if (ignoreLine(line))
            continue;

        if (line == "$end")
            return true;

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

DataSetType TextFileReader::checkDataSetType(const string & nameString)
{
    auto it = datasetNamesTypes.find(nameString);
    if (it == datasetNamesTypes.end())
    {
        cerr << "Invalid data set type: " << nameString << endl;
        return DataSetType::unknown;
    }
    return it->second;
}
