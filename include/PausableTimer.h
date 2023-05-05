#ifndef PAUSABLETIMER_H
#define PAUSABLETIMER_H

#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>

class PausableTimer : public QTimer
{
    Q_OBJECT

public:
    explicit PausableTimer(QObject* parent = nullptr);
    void start(int interval);
    void pause();
    void resume();
    qint64 elapsedTime() const;
    void reset();

private:
    QElapsedTimer elapsedTimer;
    qint64 runningTime;
    bool isPaused;
};

#endif // PAUSABLETIMER_H

