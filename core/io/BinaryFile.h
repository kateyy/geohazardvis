#pragma once

#include <memory>
#include <vector>

#include <core/core_api.h>


class QFile;
class QString;


class CORE_API BinaryFile
{
public:
    enum OpenMode
    {
        Read = 1,
        Write = 2,
        ReadWrite = Read | Write,
        Append = 4,
        Truncate = 8
    };

    explicit BinaryFile(const QString & fileName, OpenMode mode);
    ~BinaryFile();

    bool isReadable() const;
    bool isWritable() const;

    bool remove();
    bool clear();

    bool seek(size_t filePos);
    size_t pos() const;

    size_t size() const;

    template<typename T>
    bool write(const std::vector<T> & data);
    
    bool write(const void * data, size_t bytes);

    /** Reads numValues value of type T from the current position in the file and store them in vector data
    * @param data is resized to numValues or less and filled with read data.
    * @return true only if numValues are read from the file */
    template<typename T>
    bool read(size_t numValues, std::vector<T> & data);

    /** Reads numBytes bytes from the current position in the file.
    * @param dataPtr must point to memory of at least numBytes bytes. 
    * @return the number of bytes read from the file */
    size_t read(size_t numBytes, void * dataPtr);

private:
    std::unique_ptr<QFile> m_file;
};

inline BinaryFile::OpenMode operator|(BinaryFile::OpenMode lhs, BinaryFile::OpenMode rhs)
{
    return static_cast<BinaryFile::OpenMode>(
        static_cast<int>(lhs) | static_cast<int>(rhs));
}

#include "BinaryFile.hpp"
