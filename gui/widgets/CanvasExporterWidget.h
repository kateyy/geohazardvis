#pragma once

#include <map>
#include <memory>

#include <QDialog>

#include <gui/gui_api.h>


class QString;

class AbstractRenderView;
class CanvasExporter;
class Ui_CanvasExporterWidget;


class GUI_API CanvasExporterWidget : public QDialog
{
public:
    CanvasExporterWidget(QWidget * parent = nullptr, Qt::WindowFlags f = 0);
    ~CanvasExporterWidget() override;

public:
    void setRenderView(AbstractRenderView * renderView);

    /** capture screenshot with current settings to quick output folder */
    void captureScreenshot();
    /** capture screenshot with current settings and ask user for the target file name */
    void captureScreenshotTo();

private:
    void updateUiForFormat(const QString & format);

private:
    CanvasExporter * currentExporter();
    QString currentExportFolder() const;
    QString fileNameWithTimeStamp() const;
    void saveScreenshotTo(CanvasExporter * exporter,  const QString & fileName) const;

private:
    Ui_CanvasExporterWidget * m_ui;

    AbstractRenderView * m_renderView;

    std::map<QString, std::unique_ptr<CanvasExporter>> m_exporters;
};
