#include "qthelper.h"

#include <cassert>
#include <algorithm>
#include <limits>
#include <type_traits>

#include <QColor>
#include <QDebug>
#include <QEvent>
#include <QMetaEnum>
#include <QObject>
#include <QTableWidget>


QDebug operator<<(QDebug str, const QEvent * ev)
{
    str << "QEvent";
    if (ev)
    {
        static int eventEnumIndex = QEvent::staticMetaObject.indexOfEnumerator("Type");
        const QString name = QEvent::staticMetaObject.enumerator(eventEnumIndex).valueToKey(ev->type());
        if (!name.isEmpty())
        {
            str << name;
        }
        else
        {
            str << ev->type();
        }
    }
    else
    {
        str << (void*)ev;
    }
    return str.maybeSpace();
}

QColor vtkColorToQColor(double colorF[4])
{
    return QColor(int(colorF[0] * 0xFF),
                  int(colorF[1] * 0xFF),
                  int(colorF[2] * 0xFF),
                  int(colorF[3] * 0xFF));
}

void disconnectAll(std::vector<QMetaObject::Connection> & connections)
{
    disconnectAll(std::move(connections));
}

void disconnectAll(std::vector<QMetaObject::Connection> && connections)
{
    std::for_each(connections.begin(), connections.end(),
        static_cast<bool(*)(const QMetaObject::Connection &)>(&QObject::disconnect));

    connections.clear();
}

QVariant dataObjectPtrToVariant(DataObject * dataObject)
{
    static_assert(sizeof(qulonglong) >= sizeof(size_t), "Not implemented for current architecture's pointer size.");

    return static_cast<qulonglong>(reinterpret_cast<size_t>(dataObject));
}

DataObject * variantToDataObjectPtr(const QVariant & variant)
{
    static_assert(sizeof(qulonglong) >= sizeof(DataObject *), "Not implemented for current architecture's pointer size.");

    bool okay;
    auto ptr = reinterpret_cast<DataObject *>(variant.toULongLong(&okay));
    assert(variant.isNull() || okay);
    return ptr;
}

QTableWidgetSetRowsWorker::QTableWidgetSetRowsWorker(QTableWidget & table)
    : m_table{ table }
    , m_rows{}
    , m_columnCount{ 0 }
{
}

QTableWidgetSetRowsWorker::~QTableWidgetSetRowsWorker()
{
    const auto rowCount = static_cast<int>(std::min(
        static_cast<size_t>(std::numeric_limits<int>::max()), m_rows.size()));
    m_table.setRowCount(rowCount);
    m_table.setColumnCount(m_columnCount);

    const bool wasSorted = m_table.isSortingEnabled();
    m_table.setSortingEnabled(false);

    for (int r = 0; r < rowCount; ++r)
    {
        auto && row = m_rows[static_cast<size_t>(r)];
        const auto currentColumnCount = static_cast<int>(std::min(
            static_cast<size_t>(std::numeric_limits<int>::max()), row.size()));
        for (int c = 0; c < currentColumnCount; ++c)
        {
            m_table.setItem(r, c, new QTableWidgetItem(row[static_cast<size_t>(c)]));
        }
        for (int c = currentColumnCount; c < m_columnCount; ++c)
        {
            m_table.setItem(r, c, {});
        }
    }

    if (wasSorted)
    {
        m_table.setSortingEnabled(true);
    }
}

void QTableWidgetSetRowsWorker::addRow(const QString & title, const QString & text)
{
    addRow({ title, text });
}

void QTableWidgetSetRowsWorker::addRow(const std::vector<QString> & items)
{
    const auto cols = static_cast<int>(std::min(
        static_cast<size_t>(std::numeric_limits<int>::max()), items.size()));
    m_columnCount = std::max(m_columnCount, cols);
    m_rows.emplace_back(items);
}

void QTableWidgetSetRowsWorker::addRow(std::vector<QString> && items)
{
    const auto cols = static_cast<int>(std::min(
        static_cast<size_t>(std::numeric_limits<int>::max()), items.size()));
    m_columnCount = std::max(m_columnCount, cols);
    m_rows.emplace_back(std::forward<std::vector<QString>>(items));
}

QTableWidgetSetRowsWorker & QTableWidgetSetRowsWorker::operator()(const QString & title, const QString & text)
{
    addRow(title, text);
    return *this;
}

QTableWidgetSetRowsWorker & QTableWidgetSetRowsWorker::operator()(const std::vector<QString> & items)
{
    addRow(items);
    return *this;
}

QTableWidgetSetRowsWorker & QTableWidgetSetRowsWorker::operator()(std::vector<QString> && items)
{
    addRow(std::forward<std::vector<QString>>(items));
    return *this;
}

const std::vector<std::vector<QString>>& QTableWidgetSetRowsWorker::rows() const
{
    return m_rows;
}
