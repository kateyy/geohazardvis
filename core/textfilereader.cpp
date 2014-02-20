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
        pair<string, ContentType>("indices", ContentType::indices)};

    const map<ContentType, unsigned short> contentTupleSizes = {
        pair<ContentType, unsigned short>(ContentType::vertices, 4),
        pair<ContentType, unsigned short>(ContentType::indices, 3)};
}

void TextFileReader::read(const string & filename)
{
    ifstream inputStream(filename);
    assert(inputStream.good());

    list<InputDefinition> dataSets = readHeader(inputStream);

    list<ReadData> readDataSets;

    for (const InputDefinition & input : dataSets) {
        std::vector<std::vector<t_FP>> * inputData = new std::vector<std::vector<t_FP>>();
        if ( populateIOVectors(inputStream, *inputData,
                input.numTuples,
                contentTupleSizes.at(input.type))) {
            readDataSets.push_back({input.type, inputData});
        }
        else {
            assert(false);
            delete inputData;
        }
    }

}

list<TextFileReader::InputDefinition> TextFileReader::readHeader(ifstream & inputStream)
{
    assert(inputStream.good());

    string line;

    list<InputDefinition> fileContents;   // content + number of tuples
    bool validFile = false;

    while (!inputStream.eof()) {
        getline(inputStream, line);

        // ignore empty lines and comments
        if (line.empty() || line[0] == '#')
            continue;

        // line defining an input data set
        if (line.substr(0, 2) == "$ ") {
            stringstream linestream(line.substr(0, 2));
            string dataSetType, numTuples;
            getline(linestream, dataSetType, ' ');
            getline(linestream, numTuples, ' ');
            assert(contentNamesTypes.find(dataSetType) != contentNamesTypes.end());
            fileContents.push_back({
                contentNamesTypes.at(dataSetType),
                stol(numTuples)});            
        }

        // this is the end if the header section, required for valid input files
        if (line == "$end") {
            validFile = true;
            break;
        }
    }

    assert(validFile && !fileContents.empty());

    if (validFile)
        return fileContents;
    else return list<InputDefinition>();
}

void TextFileReader::readContent(std::ifstream & inputStream, const InputDefinition & inputdef)
{

}
