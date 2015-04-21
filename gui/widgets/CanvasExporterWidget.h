#pragma once

#include <QDialog>
#include <QString>
#include <QMap>

#include <gui/gui_api.h>


class AbstractRenderView;
class CanvasExporter;
class Ui_CanvasExporterWidget;


class GUI_API CanvasExporterWidget : public QDialog
{
    Q_OBJECT

public:
    CanvasExporterWidget(QWidget * parent = nullptr, Qt::WindowFlags f = 0);
    ~CanvasExporterWidget() override;

public slots:
    void setRenderView(AbstractRenderView * renderView);

    /** capture screenshot with current settings to quick output folder */
    void captureScreenshot();
    /** capture screenshot with current settings and ask user for the target file name */
    void captureScreenshotTo();

private slots:
    void updateUiForFormat(const QString & format);

private:
    CanvasExporter * currentExporter();
    QString currentExportFolder() const;
    QString fileNameWithTimeStamp() const;
    void saveScreenshotTo(CanvasExporter * exporter,  const QString & fileName) const;

private:
    Ui_CanvasExporterWidget * m_ui;

    AbstractRenderView * m_renderView;

    QMap<QString, CanvasExporter *> m_exporters;
};
