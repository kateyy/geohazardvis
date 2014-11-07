#pragma once

#include <functional>

#include <QMap>
#include <QString>

#include <core/core_api.h>


class QStringList;
class CanvasExporter;


class CORE_API CanvasExporterRegistry
{
public:
    static QStringList supportedFormatNames();
    static CanvasExporter * createExporter(const QString & formatName);

    using ExporterConstructor = std::function<CanvasExporter*()>;
    static bool registerImplementation(const ExporterConstructor & ctor);

private:
    CanvasExporterRegistry();
    static CanvasExporterRegistry & instance();

private:
    QMap<QString, ExporterConstructor> m_formatToExporter;
};
