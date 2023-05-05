#include "PausableTimer.h"

#ifndef NDEBUG // If in debug mode
#define DEBUG_PRINT(x) qDebug() << x
#else // If in release mode
#define DEBUG_PRINT(x) ((void)0) // No debug output
#endif

PausableTimer::PausableTimer(QObject* parent)
    : QTimer(parent), runningTime(0), isPaused(false)
{
}

void PausableTimer::start(int interval)
{
    if (isPaused)
    {
		resume();
	}
    else
    {
		runningTime = 0;
		elapsedTimer.start();
		QTimer::start(interval);
	}
}


void PausableTimer::pause()
{
    if (!isPaused)
    {
        isPaused = true;
        runningTime += elapsedTimer.elapsed();
        QTimer::stop();
    }
}

void PausableTimer::resume()
{
    if (isPaused)
    {
        isPaused = false;
        elapsedTimer.restart();
        QTimer::start();
    }
}

qint64 PausableTimer::elapsedTime() const
{
    if (isPaused)
    {
        return runningTime;
    }
    else
    {
        return (runningTime + elapsedTimer.elapsed());
    }
}

void PausableTimer::reset()
{
	runningTime = 0;
	//elapsedTimer.restart();
}