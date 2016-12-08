#pragma once

#include <vector>

#include <QObject>


class AbstractVisualizedData;
class RenderedData3D;


class GlyphColorMappingGlyphListener : public QObject
{
    Q_OBJECT

public:
    explicit GlyphColorMappingGlyphListener(QObject * parent = nullptr);
    ~GlyphColorMappingGlyphListener() override;

    void setData(const std::vector<AbstractVisualizedData *> & visualizedData);

signals:
    void glyphMappingChanged();

private:
    std::vector<RenderedData3D *> m_data;
    std::vector<QMetaObject::Connection> m_connects;

private:
    Q_DISABLE_COPY(GlyphColorMappingGlyphListener)
};
