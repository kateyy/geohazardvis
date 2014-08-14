#pragma once

#include <QTableView>


class TableView : public QTableView
{
    Q_OBJECT

signals:
    void mouseDoubleClicked(int column, int row);

protected:
    void mouseDoubleClickEvent(QMouseEvent * event) override;
};