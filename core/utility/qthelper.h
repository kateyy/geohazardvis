#pragma once

#include <type_traits>
#include <vector>

#include <QMetaObject>

#include <core/core_api.h>


class QColor;
class QDebug;
class QEvent;
class QTableWidget;
class QVariant;

class DataObject;


/** https://stackoverflow.com/questions/22535469/how-to-get-human-readable-event-type-from-qevent
  * Gives human-readable event type information. */
CORE_API QDebug operator<<(QDebug str, const QEvent * ev);

CORE_API QColor vtkColorToQColor(double colorF[4]);

/** Disconnect all connections and clear the list */
CORE_API void disconnectAll(std::vector<QMetaObject::Connection> & connections);
CORE_API void disconnectAll(std::vector<QMetaObject::Connection> && connections);

CORE_API QVariant dataObjectPtrToVariant(DataObject * dataObject);
CORE_API DataObject * variantToDataObjectPtr(const QVariant & variant);


/*
 * QString::number and QByteArray::number do not provide overloads for long and unsigned long.
 * QtNumberType dispatches to the matching int type: (unsigned) long long or (unsigned) int.
 */
template<typename NumberType>
struct QtNumberType
{
    static_assert(std::is_integral<NumberType>::value
        || std::is_floating_point<NumberType>::value, "");
    using type = NumberType;
};
template<typename NumberType> using QtNumberType_t = typename QtNumberType<NumberType>::type;
template<>
struct QtNumberType<long>
{
    using type = std::conditional_t<sizeof(long) == sizeof(int), int, long long>;
};
template<>
struct QtNumberType<unsigned long>
{
    using type = std::make_unsigned_t<QtNumberType<long>::type>;
};


/**
 * RAII-style helper class to fill a QTableWidget row by row.
 *
 * Call addRow for each row of the new contents. Upon destruction, previous contents will be
 * replaced by the new set of rows. Row and column count will be set automatically, whereas the 
 * column count is the maximum number of columns specified for any row.
 */
class CORE_API QTableWidgetSetRowsWorker
{
public:
    explicit QTableWidgetSetRowsWorker(QTableWidget & table);
    ~QTableWidgetSetRowsWorker();

    void addRow(const QString & title, const QString & text);
    void addRow(const std::vector<QString> & items);
    void addRow(std::vector<QString> && items);

    QTableWidgetSetRowsWorker & operator()(const QString & title, const QString & text);
    QTableWidgetSetRowsWorker & operator()(const std::vector<QString> & items);
    QTableWidgetSetRowsWorker & operator()(std::vector<QString> && items);

    const std::vector<std::vector<QString>> & rows() const;

private:
    QTableWidget & m_table;
    std::vector<std::vector<QString>> m_rows;
    int m_columnCount;
};
