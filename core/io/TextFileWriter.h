#pragma once

#include <core/core_api.h>

#include <memory>
#include <string>
#include <vector>

#include <QPointer>
#include <QString>

#include <vtkSmartPointer.h>


class QFile;
class QIODevice;
class vtkAbstractArray;

class ArraysToDelimitedText;


class CORE_API TextFileWriter
{
public:
    explicit TextFileWriter();
    virtual ~TextFileWriter();

    TextFileWriter(TextFileWriter && other);
    TextFileWriter & operator=(TextFileWriter && other);

    /**
     * Write output text directly to the file specified by fileName.
     * @param truncate Delete previous contents of the file.
     * @return true only if the file could be opened for writing, and, in case truncate is set,
     * if it was successfully truncated.
     */
    bool setOutputFileName(const QString & fileName, bool truncate = true);
    /**
     * @return The file name of the output file. This is only valid if a file name was passed to
     * setOutputFileName and setOutputTarget was not called meanwhile.
     */
    const QString & outputFileName() const;
    /**
     * Write output to a QIODevice (a file or a stream into a buffer/string).
     * To write array contents into a QString, pass in a QBuffer that maps to a QString instance.
     * @param truncate Delete previous contents of the file.
     * @return true only if the file could be opened for writing, and, in case truncate is set,
     * if it was successfully truncated.
     */
    bool setOutputTarget(QIODevice * qioDevice, bool truncate = true);
    /**
     * @return The output target device. This is only valid if a valid device was passed to
     * setOutputTarget and setOutputFileName was not called meanwhile.
     */
    QIODevice * outputTarget() const;

    const QString & delimiter() const;
    /** Set delimiter between values in a line. Defaults to a semicolon (";"). */
    void setDelimiter(const QString & delimiter);

    const QString & stringDelimiter() const;
    /** Set delimiter to be inserted before and after string values. Defaults to " */
    void setStringDelimiter(const QString & stringDelimiter);

    const QString & lineEnding() const;
    /** Set line ending character(s). Defaults to the current platform's default. */
    void setLineEnding(const QString & lineEnding);

    /**
     * Delete all current contents of the file. This will only work if the output was set via
     * setOutputFileName() or a QFile was passed to setOutptuTarget().
     */
    bool clearFile();

    /** Writer columns of the supplied array to the current position in the file. */
    bool write(vtkAbstractArray & array);
    /**
     * From the supplied arrays, write all columns of the selected rows in the order they are
     * supplied. If rows is empty, all tuples are written.
     */
    bool write(
        const std::vector<vtkSmartPointer<vtkAbstractArray>> & arrays,
        const std::vector<vtkIdType> & rows = {});

    friend void swap(TextFileWriter & lhs, TextFileWriter & rhs);

private:
    TextFileWriter(std::nullptr_t);

    void resetOutput();
    QIODevice * output();

private:
    QString m_outputFileName;
    std::unique_ptr<QFile> m_outputFile;
    QPointer<QIODevice> m_outputTarget;
    std::unique_ptr<ArraysToDelimitedText> m_writer;

private:
    TextFileWriter(const TextFileWriter &) = delete;
    void operator=(const TextFileWriter &) = delete;
};