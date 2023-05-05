#ifndef INTEGERAXIS_H
#define INTEGERAXIS_H

#include <QtCharts/QValueAxis>

class IntegerAxis : public QValueAxis
{
    Q_OBJECT

public:
    explicit IntegerAxis(QObject* parent = nullptr);

protected:
    QString formatLabel(qreal value) const;
};

#endif // INTEGERAXIS_H


