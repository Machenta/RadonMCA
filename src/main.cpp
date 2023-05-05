#include "MainWindow.h"
#include "Metrics.h"
#include "Model.h"
#include <QtWidgets/QApplication>
#include <QThread>
//#include <QIcon>
#include <QtGui/QIcon>
#ifndef NDEBUG // If in debug mode
#define DEBUG_PRINT(x) qDebug() << x
#else // If in release mode
#define DEBUG_PRINT(x) ((void)0) // No debug output
#endif


int main(int argc, char *argv[])
{
   
    QApplication a(argc, argv);
    QIcon icon("Icon.png");
    QThread metrics_thread;
    QMutex* metrics_mutex = new QMutex();
    QMutex* settings_mutex = new QMutex();
    qRegisterMetaType<DeviceSettings::Settings>("DeviceSettings::Settings");

    Model model(metrics_mutex, settings_mutex);

    //move the metrics class to a new thread
    model.moveToThread(&metrics_thread);
    metrics_thread.start();
    
    MainWindow w(&metrics_thread, metrics_mutex, settings_mutex);
    w.setWindowIcon(icon);
    a.setWindowIcon(icon);
    w.setWindowTitle("RadonMCA");

    QObject::connect(&w, &MainWindow::settingsChanged, &model, &Model::handleSettingsChanged);
    QObject::connect(&a, &QApplication::aboutToQuit, &w, &MainWindow::stopThread);
    QObject::connect(&w, &MainWindow::dataTimerTimeoutSignal, &model, &Model::handleDataTimerTimeout);
    QObject::connect(&model, &Model::sendAcquisitionEndSignal, &w, &MainWindow::onAcquisitionTimeEnd);
    QObject::connect(&model, &Model::sendDataToViewer, &w, &MainWindow::handleNewDataRefreshLoop);
    QObject::connect(&w, &MainWindow::saveFileRequested, &model, &Model::saveFile);
    QObject::connect(&w, &MainWindow::acquisitionSettingsAccepted, &model, &Model::onAcquisitionSettingsAccepted);
    QObject::connect(&w, &MainWindow::startButtonPressedSignal, &model, &Model::onStartButtonPressed);
    QObject::connect(&w, &MainWindow::stopButtonPressedSignal, &model, &Model::onStopButtonPressed);
    QObject::connect(&w, &MainWindow::saveFileSettingsChanged, &model, &Model::onSaveFileSettingsChanged);
    QObject::connect(&model, &Model::serialPortConnectionStatusUpdate, &w, &MainWindow::updateSerialConnectionStatus, Qt::QueuedConnection);
    QObject::connect(&w, &MainWindow::resumeAcquisitionSignal, &model, &Model::handleResumeAcquisition);
    QObject::connect(&w, &MainWindow::stopAcquisitionSignal, &model, &Model::handleStopAcquisition);
    QObject::connect(&w, &MainWindow::clearSignal, &model, &Model::handleClear);


    a.setWindowIcon(icon);
    w.setWindowIcon(icon);
    w.show();
    return a.exec();
}
