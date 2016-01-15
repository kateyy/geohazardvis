#include <core/io/BinaryFile.h>

#include <QFile>


BinaryFile::BinaryFile(const QString & fileName, OpenMode mode)
    : m_file(std::make_unique<QFile>(fileName))
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

BinaryFile::~BinaryFile() = default;

bool BinaryFile::isReadable() const
{
    if (!m_file->isOpen())
        return false;

    return m_file->isReadable();
}

bool BinaryFile::isWritable() const
{
    if (!m_file->isOpen())
        return false;

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
        return false;

    m_file->close();
    
    return m_file->open(mode);
}

bool BinaryFile::seek(size_t filePos)
{
    auto qpos = static_cast<qint64>(filePos);

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

    const auto writtenSize = m_file->write(
        reinterpret_cast<const char *>(data), static_cast<qint64>(bytes));

    return static_cast<size_t>(writtenSize) == bytes;
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
