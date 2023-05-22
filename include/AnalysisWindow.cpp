#include "AnalysisWindow.h"


AnalysisWindow::AnalysisWindow(Metrics* metrics, QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    layout(new QVBoxLayout()),
    AccumulatedLayout(new QVBoxLayout())
{
    ui->setupUi(this);
    connect(ui->LoadSpectrumsDirectory, &QPushButton::clicked, this, &AnalysisWindow::onSelectDirectoryButtonClicked);
    this->metrics = metrics;
    ui->SpectrumDirectory->setText(this->metrics->save_folder_name);
    spectrumDirectory = this->metrics->save_folder_name;
    filters << "*.csv";

    ui->SpectrumSliderSelector->setRange(0, 100);
    ui->SpectrumSliderSelector->setTickInterval(1);
    ui->SpectrumSliderSelector->setSingleStep(1);
    ui->SpectrumSliderSelector->setTickPosition(QSlider::TicksBothSides);

    connect(ui->SpectrumSliderSelector, &QSlider::valueChanged, this, &AnalysisWindow::displayCurrentSpectrumNumber);

    //fill accumulatedSpectrum with zeros
    accumulatedSpectrum.reserve(1024);
    for (int i = 0; i < 1024; i++) {
		accumulatedSpectrum.push_back(0);
	}

    ui->tabWidget->setTabText(0, "Individual Spectrums");
    ui->tabWidget->setTabText(1, "Accumulated Spectrum");
}

AnalysisWindow::~AnalysisWindow()
{
    delete ui;
}

void AnalysisWindow::onSelectDirectoryButtonClicked()
{
    spectrumDirectory = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    ui->SpectrumDirectory->setText(spectrumDirectory);
    this->metrics->save_folder_name = spectrumDirectory;
    qDebug() << "Directory that contains the spectrums: " << this->metrics->save_folder_name;
    //change the current working directory to the directory that contains the spectrums
    QDir::setCurrent(spectrumDirectory);

    QDir dir(spectrumDirectory);
    filters << "*.csv";
    dir.setNameFilters(filters);
    spectrumFiles = dir.entryInfoList();

    ui->SpectrumList->clear();
    
    for (const QFileInfo& fileInfo : spectrumFiles)
        ui->SpectrumList->addItem(fileInfo.fileName());

    //get the number of spectrums
    int numSpectrums = spectrumFiles.size();
    spectrumData.reserve(numSpectrums);

    for (int i = 0; i < numSpectrums; ++i) {
        QLineSeries* series = new QLineSeries();
        for (int j = 0; j < 1024; ++j) {
            series->append(j, 0);
        }
        spectrumData.push_back(series);  // Add the series to the vector
    }

    loadSpectrums();

    displaySpectrum(0);

    accumulateSpectrums();

    displayAccumulatedSpectrum();
}


void AnalysisWindow::loadSpectrums()
{
    QDirIterator it(spectrumDirectory, filters, QDir::Files, QDirIterator::Subdirectories);
    int fileIndex = 0; // File index for corresponding QLineSeries
    QVector <int> pointVec;
    pointVec.reserve(1024);
    for (int i = 0; i < 1024; ++i) {
		pointVec.push_back(0);
	}


    while (it.hasNext())
    {
        QString filename = it.next();
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        QTextStream in(&file);
        int i = 0;
        while (!in.atEnd())
        {
            QString line = in.readLine();
            bool ok;
            double value = line.toDouble(&ok);
            if (ok)
            {
                // add the number to the spectrumData vector
                spectrumData[fileIndex]->append(QPointF(i, value));
                pointVec[i] = value;

                i++;
            }
        }
        file.close();
        fileIndex++; // Increment file index for next QLineSeries
        SpectrumVec.push_back(pointVec);
    }
    qDebug() << "SpectrumVec size: " << SpectrumVec.size();
}

void AnalysisWindow::displaySpectrum(int pos)
{
    // Create a new chart
    QChart* chart = new QChart();

    //setting up the plot 
    QGradient gradient;
    gradient.setColorAt(0, QColor(255, 0, 0));
    gradient.setColorAt(1, QColor(0, 0, 255));
    //Set the axis labels
    chart->createDefaultAxes();
    //chart->axisX()->setTitleText("ADC Reading");
    //chart->axisY()->setTitleText("Number of Occurences");

    //set the range of the axis
    //chart->axisX()->setRange(0, 1024);

    // Add the first series from spectrumData to the chart
    if (!spectrumData.isEmpty()) {
        chart->addSeries(spectrumData[pos]);
    }

    // Create default axes
    chart->createDefaultAxes();
    ui->SpectrumDisplay->layout()->removeWidget(chartView);
    // Create a new chart view with the chart
    chartView = new QChartView(chart, ui->SpectrumDisplay);

    // Set the chart view to fill the SpectrumDisplay widget
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Show the chart view
    //ui->SpectrumDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->SpectrumDisplay->layout()->addWidget(chartView);

    // Show the chart view
    chartView->show();
}

void AnalysisWindow::displayAccumulatedSpectrum()
{
    //sum all of the spectrums
    QLineSeries* AccumulatedSeries = new QLineSeries();

    //create a series from the accumulated spectrum
    for (int i = 0; i < 1024; ++i) {
        AccumulatedSeries->append(i, accumulatedSpectrum[i]);
    }

    // Create a new chart
    QChart* AccumulatedChart = new QChart();
    // Add the series to the chart
    AccumulatedChart->addSeries(AccumulatedSeries);
    // Create default axes
    AccumulatedChart->createDefaultAxes();

    // Create a new chart view with the chart
    AccumulatedChartView = new QChartView(AccumulatedChart, ui->AccumulatedDisplay);
    // Set the chart view to fill the SpectrumDisplay widget
    AccumulatedChartView->setRenderHint(QPainter::Antialiasing);
    AccumulatedChartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Ensure the layout is not null before adding the widget
    if (ui->AccumulatedDisplay->layout() == nullptr) {
        ui->AccumulatedDisplay->setLayout(new QVBoxLayout());
    }
    ui->AccumulatedDisplay->layout()->addWidget(AccumulatedChartView);

    // Show the chart view
    AccumulatedChartView->show();
}


void AnalysisWindow::accumulateSpectrums()
{

    for (int i = 0; i < spectrumData.size(); ++i) {
        for (int j = 0; j < 1024; ++j) {
            if (SpectrumVec[i][j]!=0) {
                //qDebug() << "Found non-zero y value at " << i << "," << j << ": " << spectrumData[i]->at(j).y();
                //accumulatedSpectrum[j] += spectrumData[i]->at(j).y();
            }
            //accumulatedSpectrum[j] += spectrumData[i]->at(j).y();
            accumulatedSpectrum[j] += SpectrumVec[i][j];
        }
    }
    qDebug() << "accumulated spectrum size: " << spectrumData.size();
}

void AnalysisWindow::createSpectrumLineSeries() 
{

}


void AnalysisWindow::displayCurrentSpectrumNumber(int value)
{
    ui->CurrentSpectrum->setText(QString::number(value));

    displaySpectrum(value);
}