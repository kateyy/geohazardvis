/*
 * file_parser.cpp
 *
 *  Created on: Aug 13, 2013
 *      Author: Fahad Khalid
 */

#include "file_parser.h"

#include <fstream>
#include <limits>
#include <stdlib.h>

bool populateIOVectors(const string inputFileName,
						vector<vector<t_FP> > &ioVectors) {
	vector<t_FP> parsedData;

	if(!parseIOFile(inputFileName, parsedData)) {
		return false;
	}

	populateVectorsFromData(parsedData, ioVectors);

	return true;
}

bool parseIOFile(const string inputFileName, vector<t_FP> &parsedData) {
	string input;
	t_FP input_FP;

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

	for(i = 0; i < parsedData.size(); i+=numOfVectors) {
		for(j = 0; j < numOfVectors; j++) {
			ioVectors[j].push_back(parsedData[i+j]);
		}
	}
}
