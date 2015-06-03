#pragma once

#include <iosfwd>
#include <memory>

#include <core/io/types.h>



struct InputFileInfo
{
    InputFileInfo(const std::string & name, const io::ModelType type);

    const std::string name;
    const io::ModelType type;
};


class TextFileReader
{
public:
    static std::shared_ptr<InputFileInfo> read(const std::string & filename, std::vector<io::ReadDataSet> & readDatasets);

private:
    struct DataSetDef {
        io::DataSetType type;
        size_t nbLines;
        size_t nbColumns;
        std::string attributeName;
        vtkSmartPointer<vtkDataObject> vtkMetaData;
    };

    /// read the file header and leave the input stream at a position directly behind the header end
    /// @return a shared pointer to an InputFileInfo object, if the file contains a valid header
    static std::shared_ptr<InputFileInfo> readHeader(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);

    static bool readHeader_triangles(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);
    static bool readHeader_DEM(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);
    static bool readHeader_grid2D(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);
    static bool readHeader_vectorGrid3D(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);

    static io::DataSetType checkDataSetType(const std::string & nameString);
};
