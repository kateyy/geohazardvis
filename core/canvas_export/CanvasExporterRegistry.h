#pragma once

#include <functional>
#include <memory>

#include <QMap>
#include <QString>

#include <core/core_api.h>


class QStringList;
class CanvasExporter;


class CORE_API CanvasExporterRegistry
{
public:
    static const QStringList & supportedFormatNames();
    static std::unique_ptr<CanvasExporter> createExporter(const QString & formatName);

    using ExporterConstructor = std::function<std::unique_ptr<CanvasExporter>()>;
    static bool registerImplementation(const ExporterConstructor & ctor);

private:
    CanvasExporterRegistry();
    ~CanvasExporterRegistry();
    static CanvasExporterRegistry & instance();

private:
    QMap<QString, ExporterConstructor> m_formatToExporter;
    QStringList m_formatNames;
};
