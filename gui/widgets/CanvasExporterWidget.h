#pragma once

#include <QDialog>
#include <QString>
#include <QList>
#include <QMap>

#include <gui/gui_api.h>


namespace propertyguizeug { class PropertyBrowser; }
class CanvasExporter;
class RenderView;
class Ui_CanvasExporterWidget;


class GUI_API CanvasExporterWidget : public QDialog
{
    Q_OBJECT

public:
    CanvasExporterWidget(QWidget * parent = nullptr, Qt::WindowFlags f = 0);
    ~CanvasExporterWidget() override;

public slots:
    void setRenderView(RenderView * renderView);

    void captureScreenshot();

private slots:
    void updateUiForFormat(const QString & format);

private:
    Ui_CanvasExporterWidget * m_ui;
    propertyguizeug::PropertyBrowser * m_exporterSettingsBrowser;

    RenderView * m_renderView;

    QMap<QString, CanvasExporter *> m_exporters;
};
