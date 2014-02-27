#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <memory>

#include "common/ebem3d_common.h"

enum class ContentType {
    vertices,   // index + vec3
    triangles,  // three indices referring to a vertex list
    grid2d
};

class Input;

struct ReadData {
    ContentType type;
    std::vector<std::vector<t_FP>> data;
    std::shared_ptr<Input> input;
};

class TextFileReader
{
public:
    static void read(const std::string & filename, std::string & dataSetName, std::vector<ReadData> & readDataSets);

protected:
    struct InputDefinition {
        ContentType type;
        unsigned long nbLines;
        unsigned long nbColumns;
        std::shared_ptr<Input> input;
    };

    /// read the file header and leave the input stream at a position directly behind the header end
    /// @return true, only if the input has a valid format
    static bool readHeader(std::ifstream & inputStream, std::vector<InputDefinition>& inputDefs, std::string & name);

    static void readContent(std::ifstream & inputStream, const InputDefinition & inputdef);
};
