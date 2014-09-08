#include "TextFileReader.h"

#include <fstream>
#include <sstream>
#include <cassert>
#include <list>
#include <map>

#include "common/file_parser.h"


using namespace std;

namespace 
{
const map<string, DatasetType> datasetNamesTypes = {
        { "vertices", DatasetType::vertices },
        { "indices", DatasetType::indices },
        { "grid2d", DatasetType::grid2D },
        { "centroid", DatasetType::centroid },
        { "vectors", DatasetType::vectors } };
const map<string, ModelType> modelNamesType = {
        { "triangles", ModelType::triangles },
        { "grid2d", ModelType::grid2D } };
}

InputFileInfo::InputFileInfo(const std::string & name, ModelType type)
    : name(name)
    , type(type)
{
}

shared_ptr<InputFileInfo> TextFileReader::read(const string & filename, vector<ReadDataset> & readDataSets)
{
    ifstream inputStream(filename);
    assert(inputStream.good());

    vector<DatasetDef> datasetDefs;
    shared_ptr<InputFileInfo> input = readHeader(inputStream, datasetDefs);

    if (!input) {
        cerr << "could not read input text file: \"" << filename << "\"" << endl;
        return nullptr;
    }
    assert(input);

    for (const DatasetDef & datasetDef : datasetDefs) {
        ReadDataset readData{datasetDef.type, {}, datasetDef.attributeName};

        if (populateIOVectors(inputStream, readData.data,
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

std::shared_ptr<InputFileInfo> TextFileReader::readHeader(ifstream & inputStream, vector<DatasetDef> & inputDefs)
{
    assert(inputStream.good());

    bool validFile = false;
    bool started = false;
    shared_ptr<InputFileInfo> input;

    string line;


    while (!inputStream.eof())
    {
        getline(inputStream, line);

        // ignore empty lines and comments
        if (line.empty() || line[0] == '#')
            continue;

        if (line == "$begin") {
            started = true;
            continue;
        }

        if (!started) {
            //cerr << "invalid input file (does not begin with the $begin tag)." << endl;
            return make_shared<InputFileInfo>("", ModelType::raw);
        }

        // define the current 3d/2d model
        if (line.substr(0, 8) == "$ model ") {
            if (input) {
                cerr << "multiple models per file not supported." << endl;
                break;
            }
            assert(!input);
            stringstream linestream(line.substr(8, string::npos));
            string type, name;
            getline(linestream, type, ' ');
            getline(linestream, name);

            assert(modelNamesType.find(type) != modelNamesType.end());
            input = make_shared<InputFileInfo>(name, modelNamesType.at(type));

            switch (input->type)
            {
            case ModelType::triangles:
                validFile = readHeader_triangles(inputStream, inputDefs);
                break;
            case ModelType::grid2D:
                validFile = readHeader_grid2D(inputStream, inputDefs);
                break;
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

bool TextFileReader::readHeader_triangles(ifstream & inputStream, vector<DatasetDef> & inputDefs)
{
    string line;
    DatasetType currentDataType(DatasetType::unknown);
    t_UInt numCells = 0;

    while (!inputStream.eof())
    {
        getline(inputStream, line);

        // ignore empty lines and comments
        if (line.empty() || line[0] == '#')
            continue;

        // this is the end if the header section, required for valid input files
        if (line == "$end")
            return true;

        // expecting only some types of datasets from now on
        if (line.substr(0, 2) != "$ ")
        {
            cerr << "Invalid line in input file: \n\t" << line << endl;
            return false;
        }
        
        stringstream linestream(line.substr(2, string::npos));
        string datasetType, parameter;
        getline(linestream, datasetType, ' ');
        getline(linestream, parameter);

        string attributeName;

        assert(datasetNamesTypes.find(datasetType) != datasetNamesTypes.end());
        currentDataType = datasetNamesTypes.at(datasetType);

        t_UInt tupleSize = 0;
        t_UInt numTuples = 0;
        switch (currentDataType)
        {
        case DatasetType::indices:
            tupleSize = 3;
            numTuples = stoul(parameter);
            numCells = numTuples;
            break;
        case DatasetType::vertices:
            tupleSize = 4;
            numTuples = stoul(parameter);
            break;
        case DatasetType::centroid:
            tupleSize = 3;
            numTuples = numCells;
            break;
        case DatasetType::vectors:
        {
            size_t tupleSizePos = parameter.find(" ", 0);
            tupleSize = stoul(parameter.substr(0, tupleSizePos));
            numTuples = numCells;
            attributeName = parameter.substr(tupleSizePos + 1);
            break;
        }
        }
        assert(tupleSize);
        assert(numTuples);
        inputDefs.push_back({ currentDataType, numTuples, tupleSize, attributeName });
    }

    return false;
}

bool TextFileReader::readHeader_grid2D(ifstream & inputStream, vector<DatasetDef> & inputDefs)
{
    string line;
    DatasetType currentDataType(DatasetType::unknown);

    while (!inputStream.eof())
    {
        getline(inputStream, line);

        if (line.empty() || line[0] == '#')
            continue;

        if (line == "$end")
            return true;
        
        if (!(line.substr(0, 2) == "$ "))
        {
            cerr << "Invalid line in input file: \n\t" << line << endl;
            return false;
        }

        stringstream linestream(line.substr(2, string::npos));
        string datasetType, parameter;
        getline(linestream, datasetType, ' ');
        getline(linestream, parameter);


        assert(datasetNamesTypes.find(datasetType) != datasetNamesTypes.end());
        currentDataType = datasetNamesTypes.at(datasetType);

        size_t seperator = parameter.find_first_of(":");
        unsigned long columns = stol(parameter.substr(0, seperator));
        unsigned long rows = stol(parameter.substr(seperator + 1, string::npos));
        assert(columns > 0 && rows > 0);
        inputDefs.push_back({ currentDataType, rows, columns });

        continue;
    }

    return false;
}

