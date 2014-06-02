#pragma once

#include <memory>

#include <QObject>
#include <QList>
#include <QMap>

class MainWindow;
class InputRepresentation;
class TableWidget;
class RenderWidget;

class DataMapping : public QObject
{
    Q_OBJECT

public:
    DataMapping(MainWindow & mainWindow);

    void addInputRepresenation(std::shared_ptr<InputRepresentation> input);

    void openInTable(std::shared_ptr<InputRepresentation> representation);
    void openInRenderView(std::shared_ptr<InputRepresentation> representation);
    void addToRenderView(std::shared_ptr<InputRepresentation> representation, int renderView);

private slots:
    void tableClosed();

private:
    MainWindow & m_mainWindow;

    QList<std::shared_ptr<InputRepresentation>> m_inputRepresentations;
    int m_nextTableIndex;
    int m_nextRenderWidgetIndex;
    QMap<int, TableWidget*> m_tableWidgets;
    QMap<int, RenderWidget*> m_renderWidgets;
};
