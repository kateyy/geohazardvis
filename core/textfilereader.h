#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <list>

#include "common/ebem3d_common.h"

enum class ContentType {
    vertices,   // index + vec3
    triangles   // three indices referring to a vertex list
};

struct ReadData {
    ContentType type;
    std::vector<std::vector<t_FP>> data;
};

class TextFileReader
{
public:
    static void read(const std::string & filename, std::string & dataSetName, std::list<ReadData> & readDataSets);

protected:
    struct InputDefinition {
        ContentType type;
        unsigned long numTuples;
    };

    /// read the file header and leave the input stream at a position directly behind the header end
    /// @return true, only if the input has a valid format
    static bool readHeader(std::ifstream & inputStream, std::list<InputDefinition>& inputDefs, std::string & name);

    static void readContent(std::ifstream & inputStream, const InputDefinition & inputdef);
};
