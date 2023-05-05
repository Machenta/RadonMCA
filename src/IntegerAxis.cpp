#include "IntegerAxis.h"

IntegerAxis::IntegerAxis(QObject* parent)
    : QValueAxis(parent)
{
}

QString IntegerAxis::formatLabel(qreal value) const
{
    int intValue = static_cast<int>(value);
    return QString::number(intValue);
}


