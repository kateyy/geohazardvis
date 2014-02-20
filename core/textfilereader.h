#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <list>

#include "common/ebem3d_common.h"

enum class ContentType {
    vertices,
    indices
};

struct ReadData {
    ContentType type;
    std::vector<std::vector<t_FP>> * data;
};

class TextFileReader
{
public:
    static void read(const std::string & filename);

protected:
    struct InputDefinition {
        ContentType type;
        unsigned long numTuples;
    };

    /// read the file header and leave the input stream at a position directly behind the header end
    /// @return empty list if reading the header was not successful
    static std::list<InputDefinition> readHeader(std::ifstream & inputStream);

    static void readContent(std::ifstream & inputStream, const InputDefinition & inputdef);
};
