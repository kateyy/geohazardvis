/*
 * file_parser.cpp
 *
 *  Created on: Aug 13, 2013
 *      Author: Fahad Khalid
 */

#include "file_parser.h"

#include <cassert>
#include <fstream>
#include <sstream>
#include <limits>
#include <stdlib.h>

using namespace std;

bool populateIOVectors(const string inputFileName,
						vector<vector<t_FP> > &ioVectors) {
    assert(ioVectors.empty());

	vector<t_FP> parsedData;

    t_UInt nbColumns;

	if(!parseIOFile(inputFileName, parsedData, nbColumns)) {
		return false;
	}

    ioVectors.resize(nbColumns);

	populateVectorsFromData(parsedData, ioVectors);

	return true;
}

bool parseIOFile(const string inputFileName, vector<t_FP> &parsedData, t_UInt & nbColumns) {
	string input;
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
        while (!columnCounter.eof()) {
            getline(columnCounter, columnElement, ' ');
            if (columnElement != "")
                ++nbColumns;
        }
    }

	ifstream stream;
	stream.open(inputFileName.c_str(), ios::in);

	if(stream.fail()) {
		cout << endl << "\t" << "parseIOFile --- failed!" << endl;
		return false;
	}

	while(stream >> input) {
		if(input == "NaN") {
			input_FP = std::numeric_limits<double>::quiet_NaN();
		}
		else {
			input_FP = atof(input.c_str());
		}

		parsedData.push_back(input_FP);
	}
	stream.close();

	return true;
}

void populateVectorsFromData(const vector<t_FP> &parsedData,
							 vector<vector<t_FP> > &ioVectors) {
	size_t i, j, numOfVectors;
	numOfVectors = ioVectors.size();
    assert(numOfVectors > 0);

	for(i = 0; i < parsedData.size(); i+=numOfVectors) {
		for(j = 0; j < numOfVectors; j++) {
			ioVectors[j].push_back(parsedData[i+j]);
		}
	}
}
