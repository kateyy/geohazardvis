#include "textfilereader.h"

#include <fstream>
#include <sstream>
#include <cassert>
#include <list>
#include <map>

#include "common/file_parser.h"
#include "input.h"

using namespace std;

namespace {
    const map<string, ContentType> contentNamesTypes = {
        pair<string, ContentType>("vertices", ContentType::vertices),
        pair<string, ContentType>("triangles", ContentType::triangles),
        pair<string, ContentType>("grid2d", ContentType::grid2d)};

    const map<ContentType, unsigned short> contentTupleSizes = {
        pair<ContentType, unsigned short>(ContentType::vertices, 4),
        pair<ContentType, unsigned short>(ContentType::triangles, 3)};
}

void TextFileReader::read(const string & filename, std::string & dataSetName, vector<ReadData> & readDataSets)
{
    ifstream inputStream(filename);
    assert(inputStream.good());

    vector<InputDefinition> inputDefs;
    bool validFile = readHeader(inputStream, inputDefs, dataSetName);
    assert(validFile);

    if (!validFile) {
        cerr << "could not read input text file: \"" << filename << "\"" << endl;
        return;
    }

    for (const InputDefinition & inputDef : inputDefs) {
        ReadData readData({inputDef.type});

        if (populateIOVectors(inputStream, readData.data,
                inputDef.nbLines,
                inputDef.nbColumns)) {
            readData.input = inputDef.input;
            readDataSets.push_back(readData);
        }
        else {
            assert(false);
            cerr << "could not read input data set in " << filename << endl;
        }
    }
}

bool TextFileReader::readHeader(ifstream & inputStream, vector<InputDefinition> & inputDefs, string & name)
{
    assert(inputStream.good());

    bool validFile = false;

    string line;
    ContentType currentType;
    while (!inputStream.eof()) {
        getline(inputStream, line);

        // ignore empty lines and comments
        if (line.empty() || line[0] == '#')
            continue;

        // this is the end if the header section, required for valid input files
        if (line == "$end") {
            validFile = true;
            break;
        }

        // line defining an input data set
        if (line.substr(0, 2) == "$ ") {
            stringstream linestream(line.substr(2, string::npos));
            string command, parameter;
            getline(linestream, command, ' ');
            getline(linestream, parameter, ' ');

            if (command == "name") {
                name = parameter;
                continue;
            }

            // not a comment or file name, not at the end of the header, so expect data definitions
            assert(contentNamesTypes.find(command) != contentNamesTypes.end());
            currentType = contentNamesTypes.at(command);
            switch (currentType) {
            case ContentType::triangles:
            case ContentType::vertices:
                inputDefs.push_back({currentType, stol(parameter), contentTupleSizes.at(currentType)});
                break;
            case ContentType::grid2d:
                size_t seperator = parameter.find_first_of(":");
                unsigned long columns = stol(parameter.substr(0, seperator));
                unsigned long rows = stol(parameter.substr(seperator + 1, string::npos));
                assert(columns > 0 && rows > 0);
                inputDefs.push_back({currentType, rows, columns});
                break;
            }
            continue;
        }

        // additional parameters for the current data set
        if (line.substr(0, 3) == "$$ ") {
            if (inputDefs.empty()) {
                cerr << "reading dataset parameters before dataset definition. line: \n\t" << line << endl;
                continue;
            }
            stringstream linestream(line.substr(3, string::npos));
            string paramName;
            vector<string> s_values;
            switch (currentType) {
            case ContentType::grid2d: {
                getline(linestream, paramName, ' ');
                assert(paramName == "xRange" || paramName == "yRange");
                s_values.resize(2);
                getline(linestream, s_values.at(0), ' ');    // range min value
                getline(linestream, s_values.at(1));    // range max value
                shared_ptr<GridDataInput> input;
                if (inputDefs.back().input != nullptr) {
                    input = dynamic_pointer_cast<GridDataInput>(inputDefs.back().input);
                    assert(input);
                }
                else {
                    input = make_shared<GridDataInput>(name);
                    inputDefs.back().input = input;
                    assert(sizeof(input->bounds) / sizeof(input->bounds[0]) == 6);
                    std::fill(input->bounds, input->bounds + 6, 0);
                }
                if (paramName == "xRange") {
                    input->bounds[0] = stod(s_values.at(0));
                    input->bounds[1] = stod(s_values.at(1));
                }
                else if (paramName == "yRange") {
                    input->bounds[2] = stod(s_values.at(0));
                    input->bounds[3] = stod(s_values.at(1));
                }
                else {
                    cerr << "Invalid parameter in input file: \"" << paramName << "\"" << endl;
                }
                break;
            }
            default:
                cerr << "Unexpectedly detected parameters for input type " << int(currentType) << endl;
            }
            continue;
        }

        cerr << "Invalid line in input file: \n\t" << line << endl;
    }

    assert(validFile && !inputDefs.empty());

    return validFile;
}

void TextFileReader::readContent(std::ifstream & inputStream, const InputDefinition & inputdef)
{

}
