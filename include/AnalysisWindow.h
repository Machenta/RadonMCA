#ifndef ANALYSISWINDOW_H
#define ANALYSISWINDOW_H


#include <QMainWindow>
#include <QFileDialog>
#include <QDirIterator>
#include <QChartView>
#include <QLineSeries>
#include "ui_AnalysisWindow.h"
#include "Metrics.h"

namespace Ui {
    class AnalysisWindow;
}

class AnalysisWindow : public QMainWindow
{
    Q_OBJECT

    private:
    Ui::MainWindow* ui;
    Metrics* metrics;
    QFileInfoList spectrumFiles;
    QString spectrumDirectory;
    QStringList filters;
    //QVector<QVector<double>> spectrumData;
    QChartView* chartView;
    QChartView* AccumulatedChartView;
    QVector<QLineSeries*> spectrumData;
    QVector<QVector<int> > SpectrumVec;
    QLinearGradient gradient;
    QLayout *layout;
    QLayout *AccumulatedLayout;

    QVector<int> accumulatedSpectrum;
    

    public:
        explicit AnalysisWindow(Metrics* metrics, QWidget* parent = nullptr);
        ~AnalysisWindow();

    private:
        void onSelectDirectoryButtonClicked();
        void loadSpectrums();
        void displaySpectrum(int pos);
        void displayAccumulatedSpectrum();
        void accumulateSpectrums();
        void createSpectrumLineSeries();
        void displayCurrentSpectrumNumber(int value);
    
};

#endif // ANALYSISWINDOW_H