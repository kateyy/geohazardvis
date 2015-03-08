/*
 * file_parser.h
 *
 *  Created on: Aug 13, 2013
 *      Author: Fahad Khalid
 */

#pragma once

#include <core/core_api.h>
#include <core/io/types.h>


namespace FileParser
{

bool CORE_API populateIOVectors(const std::string inputFileName,
    std::vector<std::vector<io::t_FP> > &ioVectors);
bool CORE_API parseIOFile(const std::string inputFileName, std::vector<io::t_FP> &parsedData, size_t & nbColumns);
bool CORE_API parseIOStream(std::ifstream & inputStream, std::vector<io::t_FP> &parsedData, size_t numValues);
void populateVectorsFromData(const std::vector<io::t_FP> &parsedData,
    std::vector<std::vector<io::t_FP> > &vectorizedData);
bool populateIOVectors(std::ifstream & inputStream,
    std::vector<std::vector<io::t_FP> > &ioVectors,
    size_t numTuples,
    size_t componentsPerTuple);
};
