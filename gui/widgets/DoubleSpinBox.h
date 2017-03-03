#pragma once

#include <memory>

#include <QDoubleSpinBox>


#if COMPILE_QT_DESIGNER_PLUGIN
#define GUI_API
#else
#include <gui/gui_api.h>
#endif


class DoubleSpinBoxPrivate;


class GUI_API DoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
    Q_PROPERTY(Notation notation READ notation WRITE setNotation)

public:
    explicit DoubleSpinBox(QWidget * parent = nullptr);
    ~DoubleSpinBox() override;

    enum class Notation
    {
        Standard,
        Scientific,
        MostConcise
    };
    Q_ENUM(Notation)
    void setNotation(Notation notation);
    Notation notation() const;

signals:
    void notationChanged(Notation notation);

protected:
    QValidator::State validate(QString &input, int &pos) const override;
    double valueFromText(const QString &text) const override;
    QString textFromValue(double val) const override;

private:
    std::unique_ptr<DoubleSpinBoxPrivate> d_ptr;

private:
    Q_DISABLE_COPY(DoubleSpinBox)
};
