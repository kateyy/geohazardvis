#pragma once

#include <memory>

#include <QMainWindow>

class Ui_InputViewer;

class SelectionHandler;
class Input;
class RenderView;
class RenderConfigWidget;
class TableWidget;
class DataChooser;

class InputViewer : public QMainWindow
{
    Q_OBJECT

public:
    explicit InputViewer(QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~InputViewer() override;

    bool isEmpty() const;

signals:
    void dockingRequested();

public slots:
    void openFile(QString filename);

protected:
    void showEvent(QShowEvent * event) override;
    void dragEnterEvent(QDragEnterEvent * event) override;
    void dropEvent(QDropEvent * event) override;

protected:
    Ui_InputViewer * m_ui;
    RenderView * m_renderView;
    TableWidget * m_tableWidget;
    DataChooser * m_dataChooser;
    RenderConfigWidget * m_renderConfigWidget;
    std::shared_ptr<SelectionHandler> m_selectionHandler;

    std::list<std::shared_ptr<Input>> m_inputs;
};
