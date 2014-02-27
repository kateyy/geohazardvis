#include "textfilereader.h"

#include <fstream>
#include <sstream>
#include <cassert>
#include <list>
#include <map>

#include "common/file_parser.h"

using namespace std;

namespace {
    const map<string, ContentType> contentNamesTypes = {
        pair<string, ContentType>("vertices", ContentType::vertices),
        pair<string, ContentType>("triangles", ContentType::triangles)};

    const map<ContentType, unsigned short> contentTupleSizes = {
        pair<ContentType, unsigned short>(ContentType::vertices, 4),
        pair<ContentType, unsigned short>(ContentType::triangles, 3)};
}

void TextFileReader::read(const string & filename, std::string & dataSetName, list<ReadData> & readDataSets)
{
    ifstream inputStream(filename);
    assert(inputStream.good());

    list<InputDefinition> inputDefs;
    bool validFile = readHeader(inputStream, inputDefs, dataSetName);
    assert(validFile);

    if (!validFile) {
        cerr << "could not read input text file: \"" << filename << "\"" << endl;
        return;
    }

    for (const InputDefinition & input : inputDefs) {
        ReadData readData({input.type});

        if (populateIOVectors(inputStream, readData.data,
                input.numTuples,
                contentTupleSizes.at(input.type))) {
            readDataSets.push_back(readData);
        }
        else {
            assert(false);
            cerr << "could not read input data set in " << filename << endl;
        }
    }
}

bool TextFileReader::readHeader(ifstream & inputStream, list<InputDefinition> & inputDefs, string & name)
{
    assert(inputStream.good());

    bool validFile = false;

    string line;
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
            inputDefs.push_back({
                contentNamesTypes.at(command),
                stol(parameter)});
        }
    }

    assert(validFile && !inputDefs.empty());

    return validFile;
}

void TextFileReader::readContent(std::ifstream & inputStream, const InputDefinition & inputdef)
{

}
