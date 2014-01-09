/*
 * file_parser.h
 *
 *  Created on: Aug 13, 2013
 *      Author: Fahad Khalid
 */

#ifndef FILE_PARSER_H_
#define FILE_PARSER_H_

#include "ebem3d_common.h"

bool populateIOVectors(const string inputFileName,
					   vector<vector<t_FP> > &ioVectors);
bool parseIOFile(const string inputFileName, vector<t_FP> &parsedData);
void populateVectorsFromData(const vector<t_FP> &parsedData,
							 vector<vector<t_FP> > &vectorizedData);

#endif /* FILE_PARSER_H_ */
