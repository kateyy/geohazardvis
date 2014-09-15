#pragma once

#include <QDialog>
#include <QList>

#include <ui_ScalarRearrangeObjects.h>


class ScalarsForColorMapping;

class ScalarRearrangeObjects : public QDialog
{
    Q_OBJECT

public:
    static void rearrange(QWidget * parent, ScalarsForColorMapping * scalars);

private:
    explicit ScalarRearrangeObjects(QWidget * parent = nullptr, Qt::WindowFlags f = 0);

    Ui_ScalarRearrangeObjects m_ui;
};
