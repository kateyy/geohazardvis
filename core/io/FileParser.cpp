/*
 * file_parser.cpp
 *
 *  Created on: Aug 13, 2013
 *      Author: Fahad Khalid
 */

#include <core/io/FileParser.h>

#include <cassert>
#include <iostream>
#include <limits>
#include <fstream>
#include <sstream>

using namespace std;
using namespace io;

namespace FileParser
{

bool populateIOVectors(const string & inputFileName,
    vector<vector<t_FP> > &ioVectors)
{
    assert(ioVectors.empty());

    vector<t_FP> parsedData;

    size_t nbColumns;

    if (!parseIOFile(inputFileName, parsedData, nbColumns))
    {
        return false;
    }

    const auto columnSize = parsedData.size() / nbColumns;

    if (columnSize * nbColumns != parsedData.size())
    {
        return false;
    }

    ioVectors.resize(nbColumns);

    populateVectorsFromData(parsedData, ioVectors);

    return true;
}

bool populateIOVectors(ifstream & inputStream,
    io::InputVector &ioVectors,
    size_t numTuples,
    size_t componentsPerTuple)
{
    assert(ioVectors.empty());

    vector<t_FP> parsedData;

    if (!parseIOStream(inputStream, parsedData, numTuples * componentsPerTuple))
    {
        return false;
    }

    ioVectors.resize(componentsPerTuple);

    populateVectorsFromData(parsedData, ioVectors);

    return true;
}

bool parseIOStream(ifstream & inputStream, vector<t_FP> &parsedData, size_t numValues)
{
    string input;
    t_FP input_FP;

    assert(parsedData.empty());

    if (inputStream.fail())
    {
        return false;
    }

    size_t processedValue = 0;
    while (processedValue < numValues && !inputStream.eof())
    {
        inputStream >> input;

        if (input == "NaN")
        {
            input_FP = std::numeric_limits<double>::quiet_NaN();
        }
        else
        {
#if defined(_WIN32)
            input_FP = atof(input.c_str());
#else
            stringstream f_stream(input);
            f_stream >> input_FP;
#endif
        }

        parsedData.push_back(input_FP);

        ++processedValue;
    }

    if (processedValue < numValues)
        cerr << "\t" << "parseIOStream --- failed! (read less values than expected)" << endl;

    return (processedValue == numValues);
}

bool parseIOFile(const string & inputFileName, vector<t_FP> &parsedData, size_t & nbColumns)
{
    string inputValue;
    t_FP input_FP;

    assert(parsedData.empty());

    nbColumns = 0;
    {
        ifstream input;
        input.open(inputFileName.c_str(), ios::in);
        string line;
        getline(input, line);
        string columnElement;
        stringstream columnCounter(line);
        while (!columnCounter.eof())
        {
            getline(columnCounter, columnElement, ' ');
            if (columnElement != "")
                ++nbColumns;
        }
    }

    ifstream stream;
    stream.open(inputFileName.c_str(), ios::in);

    if (stream.fail())
    {
        return false;
    }

    while (stream >> inputValue)
    {
        if (inputValue == "NaN")
        {
            input_FP = std::numeric_limits<double>::quiet_NaN();
        }
        else
        {
#if defined(_WIN32)
            input_FP = atof(inputValue.c_str());
#else
            stringstream f_stream(inputValue);
            f_stream >> input_FP;
#endif
        }

        parsedData.push_back(input_FP);
    }
    stream.close();

    return true;
}

void populateVectorsFromData(const vector<t_FP> &parsedData,
    vector<vector<t_FP> > &ioVectors)
{
    size_t i, j, numOfVectors;
    numOfVectors = ioVectors.size();
    assert(numOfVectors > 0);

    for (i = 0; i < parsedData.size(); i += numOfVectors)
    {
        for (j = 0; j < numOfVectors; j++)
        {
            ioVectors[j].push_back(parsedData[i + j]);
        }
    }
}

}
