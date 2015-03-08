/*
 * file_parser.h
 *
 *  Created on: Aug 13, 2013
 *      Author: Fahad Khalid
 */

#pragma once

#include <vector>

#include <core/core_api.h>


namespace FileParser
{

using t_FP = double;

bool CORE_API populateIOVectors(const std::string inputFileName,
    std::vector<std::vector<t_FP> > &ioVectors);
bool CORE_API parseIOFile(const std::string inputFileName, std::vector<t_FP> &parsedData, size_t & nbColumns);
bool CORE_API parseIOStream(std::ifstream & inputStream, std::vector<t_FP> &parsedData, size_t numValues);
void populateVectorsFromData(const std::vector<t_FP> &parsedData,
    std::vector<std::vector<t_FP> > &vectorizedData);
bool populateIOVectors(std::ifstream & inputStream,
    std::vector<std::vector<t_FP> > &ioVectors,
    size_t numTuples,
    size_t componentsPerTuple);
};
