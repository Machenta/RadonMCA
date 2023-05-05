#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/qmessagebox.h>
#include <QTimer> 
#include <QThread>
#include <QMutex>
#include <QRegularExpression>
#include <QByteArray>
#include "ui_MainWindow.h"
#include "DeviceSettings.h"
#include "Metrics.h"
#include "PausableTimer.h"
#include "acquisitionsettingswindow.h"
#include <vector>
#include <map>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QElapsedTimer>
#include <QAreaSeries>
#include <QGraphicsRectItem>
#include <limits.h>


class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        MainWindow(QThread* metrics_thread, QMutex* savefile_mutex, QMutex* settings_mutex, QWidget *parent = nullptr);
        ~MainWindow();
    
    private:
        Ui::MainWindowClass *ui;
        AcquisitionSettingsWindow* acquisitionSettingsWindow;
        DeviceSettings* settings;
        Metrics* metrics;
        QMutex* savefile_mutex;
        QMutex* settings_mutex;
        QMutex* data_mutex;
        PausableTimer data_timer;
        QElapsedTimer data_timer_elapsed;
        QVector<int> ADC_Reading_vec; //values from ADC which are to be used for the plot
        QByteArray data; //data received from the Arduino
        QByteArray sourceData;
        std::map <int, int> ADC_Reading_map; //values from ADC which are to be used for the plot
        QVector<int> plot_vals; //values from ADC which are to be used for the plot
        QElapsedTimer timer;
        QChart* chart;
 
        QList<QAreaSeries*> coloredRegions1;
        QAreaSeries* areaSeries1;

        QList<QAreaSeries*> coloredRegions2;
        QAreaSeries* areaSeries2;

        QLineSeries* series;
        QLineSeries* cursorLine;
        QLinearGradient gradient;
        QChartView *chartView;
        QVBoxLayout *layout;
        QWidget *PlotWidget;
        QThread* metrics_thread;
        QVector<QPointF> blankPlotPoints;
        QValueAxis* axisX;
        QValueAxis* axisY;
        int plotMin;
        int plotMax;
        QString plotMinStr;
        QString plotMaxStr;
        QString cursorStr;
        int cursorChannel;
        int cursorYval;
        int threshold;
        int modifiedYValue;
        int refresh_ms;


        bool acquisitionPaused;
        QString serialPortStatus;
        QString acquisitionStatus;
        bool peak1_lower_OK;
        bool peak1_upper_OK;
        bool peak2_lower_OK;
        bool peak2_upper_OK;
        bool deviceSetup;
        qint64 lowerValue1;
        qint64 upperValue1;
        qint64 lowerValue2;
        qint64 upperValue2;
        QColor region1Color;
        QColor region2Color;





    private slots:
    
        //button presses and other user actions
        void launch_DeviceSetup();
        void launchAcquisitionSetup();
        void toggleLinLog();

        //other functions
        void refreshPlot();
        void setupPlot();
        void updateMetricsTable();
        void updatePlotMetrics();
        void getCopySettings();
        void getDefaultAcquisitionSettings();
        void updateColoredRegion(qreal a, qreal b, QColor color, QList<QAreaSeries*>& regionsList, QAreaSeries*& areaSeriesToUpdate);
        void getRegionBounds();
        void removeColoredRegion(QList<QAreaSeries*>& coloredRegions, QAreaSeries*& areaSeries);
        double countsWithBackgroundRemoval(int xLower, int xUpper);

        

        
        
    public slots:
        //button presses and other user actions
        void startButtonPressed();
        void pauseButtonPressed();
        void resumeButtonPressed();
        void stopButtonPressed();
        void saveFileUserRequest();

        void updateSerialConnectionStatus(QString status);
        void DataTimerTimeout();
        void onAcquisitionTimeEnd();
        void handleNewDataRefreshLoop(QVector<int> data_vec, Metrics metrics);
        void requestSaveFile();
        void clearPlot();
        void updatePlotBounds();
        void updateCursorPosition();
        void stopThread();

    signals:
        void settingsChanged(DeviceSettings::Settings settings);
        void startButtonPressedSignal();
        void stopButtonPressedSignal();
        void dataTimerTimeoutSignal(qint64 time_elapsed_ms);
        void saveFileRequested();
        void clearMetricsObj();
        void acquisitionSettingsAccepted(qint64 tAcq, qint64 nAcq, QString default_filename, QString save_directory);
        void saveFileSettingsChanged(QString filename, QString save_dir);
        void resumeAcquisitionSignal();
        void stopAcquisitionSignal();
        void clearSignal();
};
