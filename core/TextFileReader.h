#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <memory>

#include "common/ebem3d_common.h"


class Input;


enum class DatasetType {
    vertices,   // index + vec3
    indices,    // indices referring to a vertex list
    grid2d
};

struct ReadDataset {
    DatasetType type;
    std::vector<std::vector<t_FP>> data;
};

class TextFileReader
{
public:
    static std::shared_ptr<Input> read(const std::string & filename, std::vector<ReadDataset> & readDatasets);

protected:
    struct DatasetDef {
        DatasetType type;
        unsigned long nbLines;
        unsigned long nbColumns;
    };

    /// read the file header and leave the input stream at a position directly behind the header end
    /// @return a shared pointer to an Input object, if the file contains a valid header
    static std::shared_ptr<Input> readHeader(std::ifstream & inputStream, std::vector<DatasetDef>& inputDefs);
};
