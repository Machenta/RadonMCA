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
    connect(ui->plot_min_entry_single, &QLineEdit::textChanged, this, &AnalysisWindow::updatePlotRangeSingle);
    connect(ui->plot_max_entry_single, &QLineEdit::textChanged, this, &AnalysisWindow::updatePlotRangeSingle);

    connect(ui->plot_min_entry_accumulated, &QLineEdit::textChanged, this, &AnalysisWindow::updatePlotRangeAccumulated);
    connect(ui->plot_max_entry_accumulated, &QLineEdit::textChanged, this, &AnalysisWindow::updatePlotRangeAccumulated);

    connect(ui->lower_peak1_single, &QLineEdit::textChanged, this, &AnalysisWindow::getRegionBoundsSingle);
    connect(ui->upper_peak1_single, &QLineEdit::textChanged, this, &AnalysisWindow::getRegionBoundsSingle);
    connect(ui->lower_peak2_single, &QLineEdit::textChanged, this, &AnalysisWindow::getRegionBoundsSingle);
    connect(ui->upper_peak2_single, &QLineEdit::textChanged, this, &AnalysisWindow::getRegionBoundsSingle);

    connect(ui->lower_peak1_accumulated, &QLineEdit::textChanged, this, &AnalysisWindow::getRegionBoundsAccumulated);
    connect(ui->upper_peak1_accumulated, &QLineEdit::textChanged, this, &AnalysisWindow::getRegionBoundsAccumulated);
    connect(ui->lower_peak2_accumulated, &QLineEdit::textChanged, this, &AnalysisWindow::getRegionBoundsAccumulated);
    connect(ui->upper_peak2_accumulated, &QLineEdit::textChanged, this, &AnalysisWindow::getRegionBoundsAccumulated);


    //fill accumulatedSpectrum with zeros
    accumulatedSpectrum.reserve(1024);
    for (int i = 0; i < 1024; i++) {
		accumulatedSpectrum.push_back(0);
	}

    

    ui->tabWidget->setTabText(0, "Individual Spectrums");
    ui->tabWidget->setTabText(1, "Accumulated Spectrum");
    //disable the slider
    ui->SpectrumSliderSelector->setEnabled(false);

    coloredRegions1 = QList<QAreaSeries*>();
    coloredRegions2 = QList<QAreaSeries*>();
    coloredRegions1Accmulated = QList<QAreaSeries*>();
    coloredRegions2Accmulated = QList<QAreaSeries*>();

    areaSeries1 = new QAreaSeries();
    areaSeries2 = new QAreaSeries();
    areaSeries1Accmulated = new QAreaSeries();
    areaSeries2Accmulated = new QAreaSeries();

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
        QRegularExpression regex("\\d+");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        //expand the InformationVec to hold the information for the new spectrum
        InformationVec.push_back(QVector<QString>(10));
        QTextStream in(&file);
        int i = 0;
        int j = 0;
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
            else
            {
                
                QRegularExpressionMatch match = regex.match(line);
                
                if (match.hasMatch()) {
                    QString numberString = match.captured(0);
                    int number = numberString.toInt();
                    InformationVec[fileIndex][j] = line;
                    j++;
                }
                else {
                    j++;
                }

                
            }

        }
        file.close();
        fileIndex++; // Increment file index for next QLineSeries
        SpectrumVec.push_back(pointVec);

    }
    //qDebug() << "SpectrumVec size: " << SpectrumVec.size();

    //resize the slider to match the number of spectrums
    ui->SpectrumSliderSelector->setRange(0, SpectrumVec.size() - 1);
    ui->SpectrumSliderSelector->setEnabled(true);
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

void AnalysisWindow::updatePlotRangeSingle()
{
    QString plotMinStr = ui->plot_min_entry_single->text();
    QString plotMaxStr = ui->plot_max_entry_single->text();
    int plotMin = plotMinStr.isEmpty() ? 0 : plotMinStr.toInt();
    int plotMax = plotMaxStr.isEmpty() ? 1024 : plotMaxStr.toInt();

    chartView->chart()->axisX()->setRange(plotMin, plotMax);
}

void AnalysisWindow::updatePlotRangeAccumulated()
{
	QString plotMinStr = ui->plot_min_entry_accumulated->text();
	QString plotMaxStr = ui->plot_max_entry_accumulated->text();
	int plotMin = plotMinStr.isEmpty() ? 0 : plotMinStr.toInt();
	int plotMax = plotMaxStr.isEmpty() ? 1024 : plotMaxStr.toInt();
	AccumulatedChartView->chart()->axisX()->setRange(plotMin, plotMax);
}

void AnalysisWindow::removeColoredRegion(QList<QAreaSeries*>& coloredRegions, QAreaSeries*& areaSeries) {
    if (areaSeries != nullptr) {
        chartView->chart()->removeSeries(areaSeries);
        coloredRegions.removeAll(areaSeries);
        delete areaSeries;
        areaSeries = nullptr;
    }
}

void AnalysisWindow::removeColoredRegionAccumulated(QList<QAreaSeries*>& coloredRegions, QAreaSeries*& areaSeries) {
    if (areaSeries != nullptr) {
        AccumulatedChartView->chart()->removeSeries(areaSeries);
        coloredRegions.removeAll(areaSeries);
        delete areaSeries;
        areaSeries = nullptr;
    }
}

void AnalysisWindow::updateColoredRegion(qreal a, qreal b, QColor color, QList<QAreaSeries*>& regionsList, QAreaSeries*& areaSeriesToUpdate)
{
    // Remove and delete previous colored regions
    for (QAreaSeries* region : regionsList) {
        if (region != nullptr) {
            chartView->chart()->removeSeries(region);
            delete region;
        }
    }
    regionsList.clear(); // Clear the list

    QValueAxis* valueAxisY = qobject_cast<QValueAxis*>(chartView->chart()->axisY());
    qreal minY = valueAxisY->min();
    qreal maxY = valueAxisY->max();

    QLineSeries* lowerSeries = new QLineSeries();
    QLineSeries* upperSeries = new QLineSeries();
    lowerSeries->append(a, minY);
    lowerSeries->append(b, minY);
    upperSeries->append(a, maxY);
    upperSeries->append(b, maxY);

    areaSeriesToUpdate = new QAreaSeries(lowerSeries, upperSeries);
    areaSeriesToUpdate->setBrush(color);
    areaSeriesToUpdate->setPen(Qt::NoPen);
    chartView->chart()->addSeries(areaSeriesToUpdate);
    areaSeriesToUpdate->attachAxis(chartView->chart()->axisX());
    areaSeriesToUpdate->attachAxis(chartView->chart()->axisY());

    // Add the new region to the list
    regionsList.append(areaSeriesToUpdate);
}

void AnalysisWindow::getRegionBoundsSingle() {

    QColor region1Color = QColor(247, 213, 149, 100);
    QColor region2Color = QColor(171, 219, 227, 100);

    bool ok1, ok2, ok3, ok4;
    lowerValue1 = ui->lower_peak1_single->text().toInt(&ok1);
    upperValue1 = ui->upper_peak1_single->text().toInt(&ok2);
    lowerValue2 = ui->lower_peak2_single->text().toInt(&ok3);
    upperValue2 = ui->upper_peak2_single->text().toInt(&ok4);

    double counts1 = countsWithBackgroundRemoval(lowerValue1, upperValue1, findLineSeries(chartView->chart()->series()));
    double counts2 = countsWithBackgroundRemoval(lowerValue2, upperValue2, findLineSeries(chartView->chart()->series()));

    region1Color = QColor(247, 213, 149, 100);
    region2Color = QColor(171, 219, 227, 100);

    if (ok1 && ok2) {
        updateColoredRegion(lowerValue1, upperValue1, region1Color, coloredRegions1, areaSeries1);
        ui->counts1_single->setText(QString::number(counts1));
    }
    else {
        removeColoredRegion(coloredRegions1, areaSeries1);
        ui->counts1_single->setText("---");
    }

    if (ok3 && ok4) {
        updateColoredRegion(lowerValue2, upperValue2, region2Color, coloredRegions2, areaSeries2);
        ui->counts2_single->setText(QString::number(counts2));
    }
    else {
        removeColoredRegion(coloredRegions2, areaSeries2);
        ui->counts2_single->setText("---");
    }
}

/*
* @brief: Removes the background counts for the peak selection regions using a linear background removal
*/
double AnalysisWindow::countsWithBackgroundRemoval(int xLower, int xUpper, QLineSeries* series)
{
    double totalCounts = 0.0;

    // Obtain data points from the series
    const QVector<QPointF>& dataPoints = series->pointsVector();

    // Find the points corresponding to xLower and xUpper
    QPointF lowerPoint, upperPoint;
    for (const QPointF& point : dataPoints)
    {
        int x = static_cast<int>(point.x());
        if (x == xLower) {
            lowerPoint = point;
        }
        else if (x == xUpper) {
            upperPoint = point;
        }
    }

    // Calculate the slope and intercept for linear background removal
    double deltaX = upperPoint.x() - lowerPoint.x();
    double deltaY = upperPoint.y() - lowerPoint.y();
    double slope = deltaY / deltaX;
    double intercept = lowerPoint.y() - slope * lowerPoint.x();

    // Iterate over the data points and accumulate the counts with background removal
    for (const QPointF& point : dataPoints)
    {
        int x = static_cast<int>(point.x());
        double y = point.y();

        if (x >= xLower && x <= xUpper)
        {
            double background = slope * x + intercept;
            double correctedY = y - background;
            totalCounts += correctedY;
            qDebug() << "x: " << x << " y: " << y << " background: " << background << " correctedY: " << correctedY;
            qDebug() << "slope: " << slope << " intercept: " << intercept;
            qDebug() << "totalCounts: " << totalCounts;
        }
    }

    return totalCounts;
}

QLineSeries* AnalysisWindow::findLineSeries(const QList<QAbstractSeries*>& seriesList)
{
    for (QAbstractSeries* series : seriesList) {
        if (QLineSeries* lineSeries = qobject_cast<QLineSeries*>(series)) {
            return lineSeries;
        }
    }
    return nullptr;
}

void AnalysisWindow::getRegionBoundsAccumulated() {

    QColor region1Color = QColor(247, 213, 149, 100);
    QColor region2Color = QColor(171, 219, 227, 100);

    bool ok1, ok2, ok3, ok4;
    lowerValue1Accumulated = ui->lower_peak1_accumulated->text().toInt(&ok1);
    upperValue1Accumulated = ui->upper_peak1_accumulated->text().toInt(&ok2);
    lowerValue2Accumulated = ui->lower_peak2_accumulated->text().toInt(&ok3);
    upperValue2Accumulated = ui->upper_peak2_accumulated->text().toInt(&ok4);

    double counts1 = countsWithBackgroundRemoval(lowerValue1Accumulated, upperValue1Accumulated, findLineSeries(AccumulatedChartView->chart()->series()));
    double counts2 = countsWithBackgroundRemoval(lowerValue2Accumulated, upperValue2Accumulated, findLineSeries(AccumulatedChartView->chart()->series()));

    region1Color = QColor(247, 213, 149, 100);
    region2Color = QColor(171, 219, 227, 100);

    if (ok1 && ok2) {
        updateColoredRegionAccumulated(lowerValue1Accumulated, upperValue1Accumulated, region1Color, coloredRegions1Accmulated, areaSeries1Accmulated);
        ui->counts1_accumulated->setText(QString::number(counts1));
    }
    else {
        removeColoredRegion(coloredRegions1Accmulated, areaSeries1Accmulated);
        ui->counts1_accumulated->setText("---");
    }

    if (ok3 && ok4) {
        updateColoredRegionAccumulated(lowerValue2Accumulated, upperValue2Accumulated, region2Color, coloredRegions2Accmulated, areaSeries2Accmulated);
        ui->counts2_accumulated->setText(QString::number(counts2));
    }
    else {
        removeColoredRegion(coloredRegions2Accmulated, areaSeries2Accmulated);
        ui->counts2_accumulated->setText("---");
    }
}

void AnalysisWindow::updateColoredRegionAccumulated(qreal a, qreal b, QColor color, QList<QAreaSeries*>& regionsList, QAreaSeries*& areaSeriesToUpdate)
{
    // Remove and delete previous colored regions
    for (QAreaSeries* region : regionsList) {
        if (region != nullptr) {
            AccumulatedChartView->chart()->removeSeries(region);
            delete region;
        }
    }
    regionsList.clear(); // Clear the list

    QValueAxis* valueAxisY = qobject_cast<QValueAxis*>(AccumulatedChartView->chart()->axisY());
    qreal minY = valueAxisY->min();
    qreal maxY = valueAxisY->max();

    QLineSeries* lowerSeries = new QLineSeries();
    QLineSeries* upperSeries = new QLineSeries();
    lowerSeries->append(a, minY);
    lowerSeries->append(b, minY);
    upperSeries->append(a, maxY);
    upperSeries->append(b, maxY);

    areaSeriesToUpdate = new QAreaSeries(lowerSeries, upperSeries);
    areaSeriesToUpdate->setBrush(color);
    areaSeriesToUpdate->setPen(Qt::NoPen);
    AccumulatedChartView->chart()->addSeries(areaSeriesToUpdate);
    areaSeriesToUpdate->attachAxis(AccumulatedChartView->chart()->axisX());
    areaSeriesToUpdate->attachAxis(AccumulatedChartView->chart()->axisY());

    // Add the new region to the list
    regionsList.append(areaSeriesToUpdate);
}