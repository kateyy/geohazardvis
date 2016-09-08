#include "CanvasExporterRegistry.h"

#include <QDebug>
#include <QStringList>

#include <core/canvas_export/CanvasExporter.h>


CanvasExporterRegistry::CanvasExporterRegistry()
{
}

CanvasExporterRegistry::~CanvasExporterRegistry() = default;

CanvasExporterRegistry & CanvasExporterRegistry::instance()
{
    static CanvasExporterRegistry reg;
    return reg;
}

QStringList CanvasExporterRegistry::supportedFormatNames()
{
    return instance().m_formatToExporter.keys();
}

std::unique_ptr<CanvasExporter> CanvasExporterRegistry::createExporter(const QString & formatName)
{
    const ExporterConstructor & ctor = instance().m_formatToExporter.value(formatName, nullptr);
    if (!ctor)
        return nullptr;

    auto exporter = ctor();
    exporter->setOutputFormat(formatName);

    return exporter;
}

bool CanvasExporterRegistry::registerImplementation(const ExporterConstructor & ctor)
{
    auto exporter(ctor());

    bool okay = true;

    for (const QString & format : exporter->fileFormats())
    {
        if (instance().m_formatToExporter.contains(format))
        {
            qDebug() << "File format name already used:" << format;
            okay = false;
            continue;
        }
        instance().m_formatToExporter.insert(format, ctor);
    }

    return okay;
}
