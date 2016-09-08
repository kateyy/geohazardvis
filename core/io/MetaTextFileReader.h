#pragma once

#include <iosfwd>
#include <memory>

#include <core/io/types.h>


class DataObject;


/** Simple annotated text file format intended for quickly testing new data sets during development. */
class MetaTextFileReader
{
public:
    static std::unique_ptr<DataObject> read(const QString & fileName);

private:
    struct InputFileInfo
    {
        InputFileInfo(const QString & name, const io::ModelType type);

        const QString name;
        const io::ModelType type;

        InputFileInfo(InputFileInfo && other);
        void operator=(const InputFileInfo &) = delete;
    };

    struct DataSetDef
    {
        io::DataSetType type;
        size_t nbLines;
        size_t nbColumns;
        QString attributeName;
        vtkSmartPointer<vtkDataObject> vtkMetaData;
    };

    /// read the file header and leave the input stream at a position directly behind the header end
    /// @return a unique pointer to an InputFileInfo object, if the file contains a valid header
    static std::unique_ptr<InputFileInfo> readHeader(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);

    static bool readHeader_triangles(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);
    static bool readHeader_DEM(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);
    static bool readHeader_grid2D(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);
    static bool readHeader_vectorGrid3D(std::ifstream & inputStream, std::vector<DataSetDef>& inputDefs);

    static io::DataSetType checkDataSetType(const std::string & nameString);

    static std::unique_ptr<InputFileInfo> readData(
        const QString & fileName,
        std::vector<io::ReadDataSet> & readDatasets);

private:
    MetaTextFileReader() = delete;
    ~MetaTextFileReader() = delete;
};
