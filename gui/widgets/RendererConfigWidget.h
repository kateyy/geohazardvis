#pragma once

#include <QDockWidget>


namespace reflectionzeug
{
    class PropertyGroup;
}
class Ui_RendererConfigWidget;
class RenderView;


class RendererConfigWidget : public QDockWidget
{
    Q_OBJECT

public:
    RendererConfigWidget(QWidget * parent = nullptr);
    ~RendererConfigWidget() override;

    void clear();

public slots:
    void setRenderViews(const QList<RenderView *> & renderViews);
    void setCurrentRenderView(RenderView * renderView);

private slots:
    void setCurrentRenderView(int index);

private:
    reflectionzeug::PropertyGroup * createPropertyGroup(RenderView * renderView);

private:
    Ui_RendererConfigWidget * m_ui;

    reflectionzeug::PropertyGroup * m_propertyRoot;
};
