/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
    BinaryFile(BinaryFile && other);
    BinaryFile(const BinaryFile &) = delete;
    void operator=(const BinaryFile &) = delete;
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

    template<typename T>
    bool writeStruct(const T & plainDataStruct);
    
    bool write(const void * data, size_t bytes);

    /**
     * Reads numValues value of type T from the current position in the file and store them in
     * vector data.
     * @param data is resized to numValues or less and filled with read data.
     * @return true only if numValues are read from the file.
    */
    template<typename T>
    bool read(size_t numValues, std::vector<T> & data);

    /**
     * Reads numBytes bytes from the current position in the file.
     * @param dataPtr must point to memory of at least numBytes bytes. 
     * @return the number of bytes read from the file.
     */
    size_t read(size_t numBytes, void * dataPtr);

    template<typename T>
    bool readStruct(T & plainDataStruct);

private:
    std::unique_ptr<QFile> m_file;
};

inline BinaryFile::OpenMode operator|(BinaryFile::OpenMode lhs, BinaryFile::OpenMode rhs)
{
    return static_cast<BinaryFile::OpenMode>(
        static_cast<int>(lhs) | static_cast<int>(rhs));
}

#include "BinaryFile.hpp"
