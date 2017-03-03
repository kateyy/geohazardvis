#include "DoubleSpinBox.h"

#include <cmath>

#include <QDoubleValidator>
#include <QLineEdit>


namespace
{

char notationToFormatLetter(const DoubleSpinBox::Notation notation)
{
    switch (notation)
    {
    case DoubleSpinBox::Notation::Standard:
        return 'e';
    case DoubleSpinBox::Notation::Scientific:
        return 'f';
    case DoubleSpinBox::Notation::MostConcise:
        return 'g';
    default:
        return 0;
    }
}

template<typename T>
T clamp(const T value, const T min, const T max)
{
    return std::min(std::max(min, value), max);
}

}


class DoubleSpinBoxPrivate
{
public:
    DoubleSpinBox & q_ptr;
    std::unique_ptr<QDoubleValidator> validator;
    DoubleSpinBox::Notation notation;

    explicit DoubleSpinBoxPrivate(DoubleSpinBox & q_ptr)
        : q_ptr{ q_ptr }
        , validator{ std::make_unique<QDoubleValidator>() }
        , notation{ DoubleSpinBox::Notation::MostConcise }
    {
    }

    QString stripped(const QString & t) const
    {
        auto text = t;

        if (q_ptr.specialValueText().isEmpty() || text != q_ptr.specialValueText())
        {
            int from = 0;
            int size = text.size();
            bool changed = false;
            if (!q_ptr.prefix().isEmpty() && text.startsWith(q_ptr.prefix()))
            {
                from += q_ptr.prefix().size();
                size -= from;
                changed = true;
            }
            if (!q_ptr.suffix().isEmpty() && text.endsWith(q_ptr.suffix()))
            {
                size -= q_ptr.suffix().size();
                changed = true;
            }
            if (changed)
            {
                text = text.mid(from, size);
            }
        }

        return text.trimmed();
    }

    double validate(QString & text, int & pos, QValidator::State & state) const
    {
        auto copy = stripped(text);
        state = validator->validate(copy, pos);
        text = q_ptr.prefix() + copy + q_ptr.suffix();

        switch (state)
        {
        case QValidator::State::Acceptable:
            return copy.toDouble();
        case QValidator::State::Intermediate:
            return clamp(copy.toDouble(), q_ptr.minimum(), q_ptr.maximum());
        //case QValidator::State::Invalid:
        default:
            return q_ptr.minimum();
        }
    }
};


DoubleSpinBox::DoubleSpinBox(QWidget * parent)
    : QDoubleSpinBox(parent)
    , d_ptr{ std::make_unique<DoubleSpinBoxPrivate>(*this) }
{
    setDecimals(1000);
    setButtonSymbols(QAbstractSpinBox::NoButtons);

    lineEdit()->setValidator(d_ptr->validator.get());
}

DoubleSpinBox::~DoubleSpinBox() = default;

void DoubleSpinBox::setNotation(Notation notation)
{
    if (d_ptr->notation == notation)
    {
        return;
    }

    d_ptr->notation = notation;
    lineEdit()->update();
}

DoubleSpinBox::Notation DoubleSpinBox::notation() const
{
    return d_ptr->notation;
}

QValidator::State DoubleSpinBox::validate(QString & input, int & pos) const
{
    QValidator::State state;
    d_ptr->validate(input, pos, state);
    return state;
}

double DoubleSpinBox::valueFromText(const QString & text) const
{
    auto copy = text;
    int pos = 0;
    QValidator::State state;
    return d_ptr->validate(copy, pos, state);
}

QString DoubleSpinBox::textFromValue(double val) const
{
    auto str = QString::number(val,
        notationToFormatLetter(d_ptr->notation),
        decimals());
    fixup(str);
    return str;
}
