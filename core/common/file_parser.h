/*
 * file_parser.h
 *
 *  Created on: Aug 13, 2013
 *      Author: Fahad Khalid
 */

#ifndef FILE_PARSER_H_
#define FILE_PARSER_H_

#include "ebem3d_common.h"

#include <core/core_api.h>

bool CORE_API populateIOVectors(const std::string inputFileName,
                       std::vector<std::vector<t_FP> > &ioVectors);
bool parseIOFile(const std::string inputFileName, std::vector<t_FP> &parsedData, t_UInt & nbColumns);
bool parseIOStream(std::ifstream & inputStream, std::vector<t_FP> &parsedData, unsigned long numValues);
void populateVectorsFromData(const std::vector<t_FP> &parsedData,
                             std::vector<std::vector<t_FP> > &vectorizedData);
bool populateIOVectors(std::ifstream & inputStream,
    std::vector<std::vector<t_FP> > &ioVectors,
    unsigned long numTuples,
    unsigned long componentsPerTuple);


#endif /* FILE_PARSER_H_ */
