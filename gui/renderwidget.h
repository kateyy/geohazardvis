#pragma once

#include <QVTKWidget.h>

class RenderWidget : public QVTKWidget
{
    Q_OBJECT
        
public:
    RenderWidget(QWidget* parent = nullptr, Qt::WindowFlags f = 0);
    virtual ~RenderWidget() override;

signals:
    void onInputFileDropped(QString filename);

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;
};
