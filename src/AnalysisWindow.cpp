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

    connect(ui->background_activity, &QLineEdit::textChanged, this, &AnalysisWindow::getCalculationVariables);
    connect(ui->half_life, &QLineEdit::textChanged, this, &AnalysisWindow::getCalculationVariables);
    connect(ui->volume_entry, &QLineEdit::textChanged, this, &AnalysisWindow::getCalculationVariables);
    
    connect(ui->actionLoad_Spectrums, &QAction::triggered, this, &AnalysisWindow::onSelectDirectoryButtonClicked);

    //fill accumulatedSpectrum with zeros
    accumulatedSpectrum.reserve(1024);
    for (int i = 0; i <= 1024; i++) {
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

    backgroundActivity = 0;
    halfLife = 0;
    volume = 1;
    counts1 = 0;
    counts2 = 0;


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
    
    //for (const QFileInfo& fileInfo : spectrumFiles)
    //    ui->SpectrumList->addItem(fileInfo.fileName());

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
    QDir dir(spectrumDirectory);
    QFileInfoList fileInfoList = dir.entryInfoList(filters, QDir::Files, QDir::NoSort);

    // Custom sort comparator
    std::sort(fileInfoList.begin(), fileInfoList.end(), [](const QFileInfo& a, const QFileInfo& b) {
        // Extract the number part of the file name
        int numA = a.baseName().mid(4).toInt();  // Assuming file name always starts with "Data"
        int numB = b.baseName().mid(4).toInt();
        return numA < numB;
        });

    int fileIndex = 0; // File index for corresponding QLineSeries
    QVector<int> pointVec;
    pointVec.reserve(1024);
    for (int i = 0; i < 1024; ++i) {
        pointVec.push_back(0);
    }

    for (const QFileInfo& fileInfo : fileInfoList)
    {
        QString filename = fileInfo.absoluteFilePath();
        //get only the filename and not the whole path and add it to the list
        QFileInfo fileInfo(filename);
        ui->SpectrumList->addItem(fileInfo.fileName());
        fileNames.push_back(fileInfo.fileName());
        //print the filename to the console
        QFile file(filename);
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
                InformationVec[fileIndex][j] = line;
                j++;
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

    chartView->chart()->legend()->hide();
    chartView->chart()->setTitle("Spectrum for the file: " + fileNames[pos]);
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
    AccumulatedChartView->chart()->legend()->hide();
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
    loadAcquisitionInformation(InformationVec[value]);
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
    //qDebug() << "Limits chosen: " << xLower << " - " << xUpper;
    // Calculate the slope and intercept for linear background removal
    double deltaX = upperPoint.x() - lowerPoint.x();
    //qDebug() << "Delta X: " << deltaX;
    double deltaY = upperPoint.y() - lowerPoint.y();
    //qDebug() << "Delta Y: " << deltaY;
    double slope = deltaY / deltaX;
    double intercept = lowerPoint.y() - slope * lowerPoint.x();
    double background = (slope/2) * (upperPoint.x() * upperPoint.x() - lowerPoint.x() * lowerPoint.x()) + intercept * deltaX;
    //qDebug() << "Slope: " << slope;
    //qDebug() << "Intercept: " << intercept;
    //qDebug() << "Background: " << background;
    // Iterate over the data points and accumulate the counts 
    for (const QPointF& point : dataPoints)
    {
        int x = static_cast<int>(point.x());
        double y = point.y();

        if (x >= xLower && x <= xUpper)
        {
            totalCounts += y;
            qDebug() << "Counts: " << y;
            qDebug() << "Point: " << x << " - " << y;
        }
    }
    //total counts are the counts under the line minus the background
    totalCounts -= background;
    qDebug() << "Total counts: " << totalCounts;

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

    counts1 = countsWithBackgroundRemoval(lowerValue1Accumulated, upperValue1Accumulated, findLineSeries(AccumulatedChartView->chart()->series()));
    counts2 = countsWithBackgroundRemoval(lowerValue2Accumulated, upperValue2Accumulated, findLineSeries(AccumulatedChartView->chart()->series()));

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

    //check if the user has selected a region and it they have, calculate activity
    if (ok1 && ok2 || ok3 && ok4) {
		//calculate activity
		calculateActivity();
	}
    else {
	
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

void AnalysisWindow::loadAcquisitionInformation(QVector<QString> infoVec)
{

    // QVector to store the extracted information
    QVector<QString> extractedInfo;

    QRegularExpression regex(":\\s*(.*)");

    for (int i = 0; i < infoVec.size(); ++i) {
        QString line = infoVec.at(i); // Explicitly defining line as QString
        QRegularExpressionMatch match = regex.match(infoVec[i]);
        if (match.hasMatch()) {
            QString info = match.captured(1).trimmed();
            extractedInfo.push_back(info);
        }
    }

    // Print the extracted information
    for (const auto& info : extractedInfo) {
        qDebug() << "Extracted info: " << info;
    }

    ui->preset_time_label->setText(extractedInfo[0]);
    ui->elapsed_time_label->setText(extractedInfo[5]);
    ui->total_counts_label->setText(extractedInfo[2]);
    ui->start_time_label->setText(extractedInfo[3]);

}

void AnalysisWindow::getCalculationVariables()
{
    bool activityOk, halfLifeOk,volumeOk;
    //get the values from the text boxes that will be used for the calculations
    backgroundActivity = ui->background_activity->text().toDouble(&activityOk);
    halfLife = ui->half_life->text().toDouble(&halfLifeOk);
    volume = ui->volume_entry->text().toDouble(&volumeOk);

    if (activityOk && halfLifeOk && volumeOk) {
		qDebug() << "Activity: " << backgroundActivity;
		qDebug() << "Half Life: " << halfLife;
		qDebug() << "Volume: " << volume;
        if (volume != 0) {
            calculateActivity();
        }
            
	}
    else {
		qDebug() << "Error: Could not convert to double";
	}
    
}

void AnalysisWindow::calculateActivity()
{
	//calculate the activity while making sure that the values are not nan or it does not divide by zero

    double activity1 = (halfLife * counts1) / volume - backgroundActivity;
    qDebug() << "Activity 1: " << activity1;
    double activity2 = (halfLife * counts2) / volume - backgroundActivity;
    qDebug() << "Activity 2: " << activity2;

	//display the activity if it's not nan
    if (std::isnan(activity1)) {
		ui->peak1_activity->setText("0");
	}
    else {
        ui->peak1_activity->setText(QString::number(activity1));
    }


    if (std::isnan(activity2)) {
        ui->peak2_activity->setText("0");
    }
    else {
		ui->peak2_activity->setText(QString::number(activity1));
	}
	
}