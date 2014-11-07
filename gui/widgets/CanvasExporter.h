#pragma once

#include <QString>
#include <QWidget>

#include <vtkSmartPointer.h>

#include <gui/gui_api.h>


class RenderView;
class Ui_CanvasExporter;

class GUI_API CanvasExporter : public QWidget
{
    Q_OBJECT

public:
    CanvasExporter(QWidget * parent = nullptr, Qt::WindowFlags f = 0);
    ~CanvasExporter() override;

    Q_PROPERTY(QString exportFolder MEMBER m_exportFolder);

public slots:
    void setRenderView(RenderView * renderView);

    void captureScreenshot();

private:
    Ui_CanvasExporter * m_ui;

    RenderView * m_renderView;

    QString m_exportFolder;
};
