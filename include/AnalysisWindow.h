#ifndef ANALYSISWINDOW_H
#define ANALYSISWINDOW_H


#include <QMainWindow>
#include <QFileDialog>
#include <QDirIterator>
#include <QChartView>
#include <QLineSeries>
#include <QAreaSeries>
#include <QValueAxis>
#include <QRegularExpression>
#include "ui_AnalysisWindow.h"
#include "Metrics.h"

namespace Ui {
    class AnalysisWindow;
}

class AnalysisWindow : public QMainWindow
{
    Q_OBJECT

    private:
    
    Metrics* metrics;
    QFileInfoList spectrumFiles;
    QStringList filters;
    //QVector<QVector<double>> spectrumData;
    QChartView* chartView;
    QChartView* AccumulatedChartView;
    QVector<QLineSeries*> spectrumData;
    QVector<QVector<int> > SpectrumVec;
    QVector<QVector<QString> > InformationVec;
    QLinearGradient gradient;
    QLayout *layout;
    QLayout *AccumulatedLayout;
    QVector<int> accumulatedSpectrum;


    //constants for colored regions
    qint64 lowerValue1;
    qint64 upperValue1;
    qint64 lowerValue2;
    qint64 upperValue2;

    qint64 lowerValue1Accumulated;
    qint64 upperValue1Accumulated;
    qint64 lowerValue2Accumulated;
    qint64 upperValue2Accumulated;

    QList<QAreaSeries*> coloredRegions1;
    QAreaSeries* areaSeries1;

    QList<QAreaSeries*> coloredRegions2;
    QAreaSeries* areaSeries2;

    QList<QAreaSeries*> coloredRegions1Accmulated;
    QAreaSeries* areaSeries1Accmulated;

    QList<QAreaSeries*> coloredRegions2Accmulated;
    QAreaSeries* areaSeries2Accmulated;
    

    public:
        explicit AnalysisWindow(Metrics* metrics, QWidget* parent = nullptr);
        ~AnalysisWindow();
        Ui::MainWindow* ui;
        QString spectrumDirectory;

    private:
        void onSelectDirectoryButtonClicked();
        void loadSpectrums();
        void displaySpectrum(int pos);
        void displayAccumulatedSpectrum();
        void accumulateSpectrums();
        void createSpectrumLineSeries();
        void displayCurrentSpectrumNumber(int value);
        void updatePlotRangeSingle();
        void updatePlotRangeAccumulated();
        void removeColoredRegion(QList<QAreaSeries*>& coloredRegions, QAreaSeries*& areaSeries);
        void removeColoredRegionAccumulated(QList<QAreaSeries*>& coloredRegions, QAreaSeries*& areaSeries);
        void updateColoredRegion(qreal a, qreal b, QColor color, QList<QAreaSeries*>& regionsList, QAreaSeries*& areaSeriesToUpdate);
        void updateColoredRegionAccumulated(qreal a, qreal b, QColor color, QList<QAreaSeries*>& regionsList, QAreaSeries*& areaSeriesToUpdate);
        void getRegionBoundsSingle();
        double countsWithBackgroundRemoval(int xLower, int xUpper, QLineSeries* series);
        QLineSeries* findLineSeries(const QList<QAbstractSeries*>& seriesList);
        void getRegionBoundsAccumulated();

};      

#endif // ANALYSISWINDOW_H