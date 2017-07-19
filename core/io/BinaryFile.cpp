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

#include <core/io/BinaryFile.h>

#include <QFile>


BinaryFile::BinaryFile(const QString & fileName, OpenMode mode)
    : m_file{ std::make_unique<QFile>(fileName) }
{
    QIODevice::OpenMode qmode = QIODevice::Unbuffered;
    if (mode & OpenMode::Read)
        qmode |= QIODevice::ReadOnly;
    if (mode & OpenMode::Write)
        qmode |= QIODevice::WriteOnly;
    if (mode & OpenMode::Append)
        qmode |= QIODevice::Append;
    if (mode & OpenMode::Truncate)
        qmode |= QIODevice::Truncate;

    m_file->open(qmode);
}

BinaryFile::BinaryFile(BinaryFile && other)
    : m_file{ std::move(other.m_file) }
{
}

BinaryFile::~BinaryFile() = default;

bool BinaryFile::isReadable() const
{
    if (!m_file->isOpen())
    {
        return false;
    }

    return m_file->isReadable();
}

bool BinaryFile::isWritable() const
{
    if (!m_file->isOpen())
    {
        return false;
    }

    return m_file->isWritable();
}

bool BinaryFile::remove()
{
    return m_file->remove();
}

bool BinaryFile::clear()
{
    if (!isWritable())
    {
        return false;
    }

    const auto mode = m_file->openMode();

    m_file->close();
    bool success = m_file->open(QIODevice::WriteOnly | QIODevice::Truncate);
    if (!success)
    {
        return false;
    }

    m_file->close();

    return m_file->open(mode);
}

bool BinaryFile::seek(size_t filePos)
{
    const auto qpos = static_cast<qint64>(filePos);

    if (static_cast<size_t>(qpos) != filePos)
    {
        return false;
    }

    return m_file->seek(qpos);
}

size_t BinaryFile::pos() const
{
    return static_cast<size_t>(m_file->pos());
}

size_t BinaryFile::size() const
{
    return static_cast<size_t>(m_file->size());
}

bool BinaryFile::write(const void * data, size_t bytes)
{
    if (!isWritable())
    {
        return false;
    }

    if (bytes == 0u)
    {
        return true;
    }

    qint64 bytesRemaining = bytes;
    auto currentPtr = reinterpret_cast<const char *>(data);

    while (bytesRemaining)
    {
        auto bytesWritten = m_file->write(currentPtr, static_cast<qint64>(bytesRemaining));
        if (bytesWritten == 0)
        {
            break;
        }
        currentPtr += bytesWritten;
        bytesRemaining -= bytesWritten;
    }

    return bytesRemaining == 0;
}

size_t BinaryFile::read(size_t numBytes, void * dataPtr)
{
    if (!isReadable())
    {
        return false;
    }

    if (numBytes == 0u)
    {
        return 0u;
    }

    return m_file->read(reinterpret_cast<char *>(dataPtr), numBytes);
}
