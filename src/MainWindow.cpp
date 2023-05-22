#include "MainWindow.h"
#include <QLogValueAxis>



#ifndef NDEBUG // If in debug mode
#define DEBUG_PRINT(x) qDebug() << x
#else // If in release mode
#define DEBUG_PRINT(x) ((void)0) // No debug output
#endif

MainWindow::MainWindow(QThread* metrics_thread, QMutex* savefile_mutex, QMutex* settings_mutex, QWidget *parent)
    : QMainWindow(parent), metrics_thread(metrics_thread), savefile_mutex(savefile_mutex), settings_mutex(settings_mutex),
    ui (new Ui::MainWindowClass),
    settings (new DeviceSettings(this))
	,chart (new QChart)
	,series (new QLineSeries)
	,chartView (new QChartView(chart))
	,PlotWidget (findChild<QWidget*>("PlotdrawWidget"))
	,layout (new QVBoxLayout())
	,metrics (new Metrics())
	,data_timer( new PausableTimer(this))
	,acquisitionSettingsWindow(new AcquisitionSettingsWindow(this))
	,data_mutex(new QMutex())
	


{
	acquisitionPaused = false;
    ui->setupUi(this);
	ui->acquisition_status->setAutoFillBackground(true);
	ui->acquisition_status_label->setAutoFillBackground(true);
	//ui->PlotdrawWidget = new QWidget(this);
	layout -> addWidget(chartView);
	plot_vals.resize(1024);
	for (int i = 0; i < 1024; i++) {
		series->append(i, i);
	}

	refresh_ms = 100;
	//"Not Started", "Running", "Paused", "Stopped"
	acquisitionStatus = "Not Started";
	data_timer.setInterval(refresh_ms);

    connect(ui->actionDevice_Setup, &QAction::triggered, this, &MainWindow::launch_DeviceSetup);
	connect(ui->actionAcquisition_Setup, &QAction::triggered, this, &MainWindow::launchAcquisitionSetup);
	connect(ui->start_button, &QPushButton::clicked, this, &MainWindow::startButtonPressed);
	connect(ui->stop_button, &QPushButton::clicked, this, &MainWindow::stopButtonPressed);
	connect(ui->plot_min_entry, &QLineEdit::textChanged, this, &MainWindow::updatePlotBounds);
	connect(ui->plot_max_entry, &QLineEdit::textChanged, this, &MainWindow::updatePlotBounds);
	connect(ui->channel_select_entry, &QLineEdit::textChanged, this, &MainWindow::updateCursorPosition);
	connect(&data_timer, &QTimer::timeout, this, &MainWindow::DataTimerTimeout);

	//connecting the user inputs 
	connect(ui->plot_min_entry, &QLineEdit::textChanged, this, &MainWindow::updatePlotMetrics);
	connect(ui->plot_max_entry, &QLineEdit::textChanged, this, &MainWindow::updatePlotMetrics);
	connect(ui->threshold_entry, &QLineEdit::textChanged, this, &MainWindow::refreshPlot);
	connect(ui->saveFileAction, &QAction::triggered, this, &MainWindow::saveFileUserRequest);
	connect(ui->clear_button, &QPushButton::clicked, this, &MainWindow::clearPlot);
	connect(ui->lin_log_button, &QPushButton::clicked, this, &MainWindow::toggleLinLog);

	connect(ui->lower_peak1, &QLineEdit::textChanged, this, &MainWindow::getRegionBounds);
	connect(ui->upper_peak1, &QLineEdit::textChanged, this, &MainWindow::getRegionBounds);
	connect(ui->lower_peak2, &QLineEdit::textChanged, this, &MainWindow::getRegionBounds);
	connect(ui->upper_peak2, &QLineEdit::textChanged, this, &MainWindow::getRegionBounds);


	//connect the settings dialog button "Apply" button to propagate the changes to the model class
	connect(settings->getApplyButton(), &QPushButton::clicked, this, &MainWindow::getCopySettings);

	//initializing the map with the proper number of ADC channels
	for (int i = 0; i < 1024; i++) {
		//ADC_Reading_map[i] = 1;
		plot_vals[i]= 0;
	}

	//initializing the series for the first time
	for (int i = 0; i < 1024; i++) {
		series->replace(i,i, plot_vals[i]);
	}
	setupPlot();
	getDefaultAcquisitionSettings();
	deviceSetup = false;
	updateMetricsTable();
	blankPlotPoints.resize(1024);
	for (int i = 0; i < 1024; i++) {
		blankPlotPoints[i] = QPointF(i, 0);
	}

	coloredRegions1 = QList<QAreaSeries*>();
	areaSeries1 = new QAreaSeries();
	coloredRegions2 = QList<QAreaSeries*>();
	areaSeries2 = new QAreaSeries();
	QPointer<QAreaSeries> coloredAreaSeries = new QAreaSeries();
	plotMin = 0;
	plotMax = 0;
	threshold = 0;
}

MainWindow::~MainWindow()
{

}

/*
@brief: Launches the acquisition device setup dialog 

The device must be configured before acquisition can begin. 
*/
void MainWindow::launch_DeviceSetup()
{

    //launch the settings dialog 
    int ret = settings->exec();
	DEBUG_PRINT("Device Setup Dialog Launched");
    if (ret==QDialog::Accepted) {
        // the user clicked the OK button in the dialog
        const DeviceSettings::Settings p = settings->settings();
        // do something with the settings, e.g. update the serial port configuration
		deviceSetup = true;
		DEBUG_PRINT("Device Setup Dialog Accepted");
    }
	return;
}

/* 
* @brief: Launches the acquisition settings dialog to setup basic acquisition parameters
* 
* The acquisition parameters are used to setup the necessary timers and conditions to run the 
* acquisition for the appropriate amount of time and number of times.
* 
* It retrieves: 
* -Acquisition Time - in seconds
* -Number of acquisitions 
* -Default file name
* -Save directory
* 
* @emits: acquisitionSettingsAccepted(qint64, qint64, QString, QString) - signal to the model class to setup the acquisition
*/
void MainWindow::launchAcquisitionSetup()
{
	int result = acquisitionSettingsWindow->exec();
	if (result == QDialog::Accepted)
	{
		qint64 acquisitionTime = acquisitionSettingsWindow->getAcq_t();
		qint64 numAcquisitions = acquisitionSettingsWindow->getAcq_n();
		QString default_filename = acquisitionSettingsWindow->getDefault_filename();
		refresh_ms = acquisitionSettingsWindow->getRefresh_ms();
		//QString save_directory = acquisitionSettingsWindow->onSelectDirectoryButtonClicked();
		QString save_directory = acquisitionSettingsWindow->save_dir;
		emit acquisitionSettingsAccepted(acquisitionTime, numAcquisitions, default_filename, save_directory);
		this->metrics->n_acq = numAcquisitions;
		this->metrics->preset_time_ms = acquisitionTime*1000;
		this->metrics->preset_time = acquisitionTime;
		this->metrics->save_file_name = default_filename;
		this->metrics->save_folder_name = save_directory;
		getDefaultAcquisitionSettings();
		updateMetricsTable();
		
	}
	else
	{
		DEBUG_PRINT("Acquisition Settings Rejected");
	}
	return;
}

/*
* @brief: Starts all necessary timers to begin the acquisition process
* 
* 
*/
void MainWindow::startButtonPressed()
{
	if (acquisitionStatus == "Paused" || acquisitionStatus == "Not Started") {

		if (!deviceSetup)
		{
			QMessageBox::warning(this, "Device not setup", "Please setup the device before starting the acquisition");
			return;
		}
		else
		{
			data_timer.start(refresh_ms);
			data_timer_elapsed.start();
			emit startButtonPressedSignal();
			DEBUG_PRINT("Timer Started");
			acquisitionPaused = true;
			ui->acquisition_status->setText("Running");
			ui->acquisition_status->setStyleSheet("QLabel { background-color: rgb(164, 227, 82); }");
			//ui->acquisition_status_label->setStyleSheet("QLabel { background-color: rgb(164, 227, 82); }");
			acquisitionStatus = "Running";
			
		}
	}
	else
	{
		resumeButtonPressed();
	}
}

/*
* @brief: pauses the acquisition process, but does not terminate it 
*/
void MainWindow::pauseButtonPressed()
{
	if (deviceSetup)
	{
		DEBUG_PRINT("Pause Button Pressed");
		acquisitionPaused = true;
		//DEBUG_PRINT("Timer Paused");
		//DEBUG_PRINT("Current elapsed time:"  +  QString::number(data_timer_elapsed.elapsed()) + " ms");
		//m_timer.resume();
		data_timer.pause();
		//data_timer.start(1000);
		//emit startButtonPressedSignal();
		//DEBUG_PRINT("Timer Started");
		ui->acquisition_status->setText("Paused");
		ui->acquisition_status->setStyleSheet("QLabel { background-color: rgb(227, 208, 82); }");
		//ui->acquisition_status_label->setStyleSheet("QLabel { background-color: rgb(227, 208, 82); }");
		acquisitionStatus = "Paused";
	}
	return;
}

/*
* @brief: resumes the acquisition process after it has been paused
*/
void MainWindow::resumeButtonPressed()
{
	if(deviceSetup)
	{
		DEBUG_PRINT("Resume Button Pressed");
		DEBUG_PRINT("Timer Resumed");
		DEBUG_PRINT("Current elapsed time:" + QString::number(data_timer.elapsedTime()) + " ms");
		//m_timer.resume();
		data_timer.resume();
		ui->acquisition_status->setText("Running");
		ui->acquisition_status->setStyleSheet("QLabel { background-color: rgb(164, 227, 82); }");
		//ui->acquisition_status_label->setStyleSheet("QLabel { background-color: rgb(164, 227, 82); }");
		//data_timer.start(1000);
		//emit startButtonPressedSignal();
		//DEBUG_PRINT("Timer Started");
		emit resumeAcquisitionSignal();
		acquisitionStatus = "Running";
	}
	return;
}

/*
* @brief: stops the acquisition process and terminates the acquisition
* 
* @emits: stopButtonPressedSignal() - signal to the model class to stop the acquisition
*/
void MainWindow::stopButtonPressed()
{
		if (deviceSetup)
		{
			if (acquisitionPaused)
			{
				//print to the console "Stop Button Pressed"
				DEBUG_PRINT("Stop Button Pressed");
				//data_timer.pause();
				data_timer.pause();
				ui->acquisition_status->setStyleSheet("QLabel { background-color: rgb(227, 97, 82); }");
				//ui->acquisition_status_label->setStyleSheet("QLabel { background-color: rgb(227, 97, 82); }");
				ui->acquisition_status->setText("Stopped");
				ui->start_button->setText("Resume");
				emit stopAcquisitionSignal();
				acquisitionStatus = "Stopped";
			}
			else
			{
				//this->closeSerialPort();
				emit stopButtonPressedSignal();
				DEBUG_PRINT("Timer Stopped");
				acquisitionPaused = true;
				ui->acquisition_status->setText("Acquisition Ended");
				ui->acquisition_status->setStyleSheet("QLabel { background-color: rgb(227, 97, 82); }");
				//ui->acquisition_status_label->setStyleSheet("QLabel { background-color: rgb(227, 97, 82); }");
				acquisitionStatus = "Stopped";
			}
		}
}

/*
* @brief:  refreshes the plot with the new data
* 
* This is run whenever the data timer times out. It updates the plot with the new data
* Replaces the current data with the new data. 
* Auto scales the y-axis to the max value of the data. 
*/
void MainWindow::refreshPlot()
{

	//for (int i = 0; i < 1024; i++) {
	//	series->replace(i, i, plot_vals[i]);
	//}
	QString thresholdStr = ui->threshold_entry->text();
	threshold = thresholdStr.isEmpty() ? 0 : thresholdStr.toInt();
	QVector<QPointF> newPoints;
	newPoints.reserve(1024);

	for (int i = 0; i < 1024; i++) {
		modifiedYValue = i < threshold ? 0 : plot_vals[i];
		newPoints.append(QPointF(i, modifiedYValue));
	}

	series->replace(newPoints);
	if (plotMin >= 0 && plotMax > plotMin) {

		if (threshold > plotMin && threshold < plotMax){
			DEBUG_PRINT("Threshold is greater than plotMin");
			auto max_iter = std::max_element(plot_vals.begin() + threshold, plot_vals.begin() + plotMax);
			int max_value = *max_iter;
			chart->axisY()->setRange(0, 1.1 * max_value + 20);
		}

		else {
			DEBUG_PRINT("Threshold is less than plotMin");
			auto max_iter = std::max_element(plot_vals.begin() + plotMin, plot_vals.begin() + plotMax);
			int max_value = *max_iter;
			chart->axisY()->setRange(0, 1.1 * max_value + 20);
		}
		
	}

	else
	{
		DEBUG_PRINT("Threshold is less than 0");
		auto max_iter = std::max_element(plot_vals.begin(), plot_vals.end());
		int max_value = *max_iter;
		chart->axisY()->setRange(0, 1.1 * max_value + 20);
	}
	
	//DEBUG_PRINT("Max Value is: " << max_value);

}


/*
* @brief: sets up the plot for the first time when the main window is created
* 
* 
*/
void MainWindow::setupPlot()
{
	
	//setting up the plot 
	gradient.setColorAt(0, QColor(255, 0, 0));
	gradient.setColorAt(1, QColor(0, 0, 255));
	series->setBrush(gradient);
	chart->addSeries(series);
	//Set the axis labels
	chart->createDefaultAxes();
	chart->axisX()->setTitleText("ADC Reading");
	chart->axisY()->setTitleText("Number of Occurences");

	//set the range of the axis
	chart->axisX()->setRange(0, 1024);
	auto max_iter = std::max_element(plot_vals.begin(), plot_vals.end());
	int max_value = *max_iter;
	chart->axisY()->setRange(0, 1.3 * max_value + 20);

	// Set label format for integer values
	auto valueAxisX = qobject_cast<QValueAxis*>(chart->axisX());
	auto valueAxisY = qobject_cast<QValueAxis*>(chart->axisY());
	if (valueAxisX) {
		valueAxisX->setLabelFormat("%i");
	}
	if (valueAxisY) {
		valueAxisY->setLabelFormat("%i");
	}

	//drawing the map on the plot for the first time
	ui->PlotdrawWidget->setLayout(layout);

	cursorLine = new QLineSeries();
	QPen pen(Qt::red, 1, Qt::DashLine);
	cursorLine->setPen(pen);
	chart->addSeries(cursorLine);
	cursorLine->attachAxis(chart->axisX());
	cursorLine->attachAxis(chart->axisY());
	chart->legend()->hide();
	//updateColoredRegion(100, 200);
}

//void MainWindow::setupPlot()
//{
//	//setting up the plot
//	gradient.setColorAt(0, QColor(255, 0, 0));
//	gradient.setColorAt(1, QColor(0, 0, 255));
//	series->setBrush(gradient);
//	chart->addSeries(series);
//	//Set the axis labels
//	chart->createDefaultAxes();
//	chart->axisX()->setTitleText("ADC Reading");
//	chart->axisY()->setTitleText("Number of Occurences");
//
//	//set the range of the axis
//	chart->axisX()->setRange(0, 1024);
//	auto max_iter = std::max_element(plot_vals.begin(), plot_vals.end());
//	int max_value = *max_iter;
//	chart->axisY()->setRange(0, 1.3 * max_value + 20);
//
//	// Set label format for integer values
//	auto valueAxisX = qobject_cast<QValueAxis*>(chart->axisX());
//	auto valueAxisY = qobject_cast<QValueAxis*>(chart->axisY());
//	if (valueAxisX) {
//		valueAxisX->setLabelFormat("%i");
//	}
//	if (valueAxisY) {
//		valueAxisY->setLabelFormat("%i");
//	}
//
//	// Create a QChartView and set the chart to it
//	QChartView* chartView = new QChartView(chart);
//	chartView->setRenderHint(QPainter::Antialiasing);
//
//	// Add the QChartView to the layout
//	layout->addWidget(chartView);
//
//	// Set the layout to the PlotdrawWidget
//	ui->PlotdrawWidget->setLayout(layout);
//
//	cursorLine = new QLineSeries();
//	QPen pen(Qt::red, 1, Qt::DashLine);
//	cursorLine->setPen(pen);
//	chart->addSeries(cursorLine);
//	cursorLine->attachAxis(chart->axisX());
//	cursorLine->attachAxis(chart->axisY());
//	chart->legend()->hide();
//	//updateColoredRegion(100, 200);
//}



/*
* @brief: update the metrics table with the new calculated metrics
* 
* This is run whenever the data timer times out. It updates the metrics table with the new metrics
* 
*/
void MainWindow::updateMetricsTable() 
{

	ui->acquisition_status ->setText(metrics->acq_status);
	ui->start_time ->setText(metrics->start_time);

	if (metrics->preset_time == 2147483647) {
		ui->preset_time->setText("Infinite");
	}
	else {
		ui->preset_time ->setText(QString::number(metrics->preset_time));
	}
	ui->n_channels ->setText(QString::number(metrics->n_channels));
	ui->n_acquisitions->setText(QString::number(metrics->n_acq));
	ui->current_acquisition->setText(QString::number(metrics->current_acq));
	ui->time_elapsed->setText(QString::number(static_cast<float>(metrics->time_elapsed_ms)/1000));
	ui->live_time ->setText(QString::number(static_cast<float>(metrics->time_elapsed_ms) / 1000));
	//ui->live_time ->setText(QString::number(metrics->live_time));
	ui->total_counts ->setText(QString::number(metrics->total_counts));
	ui->count_rate->setText(QString::number(metrics->acq_rate));
	return;
}

/*
* @brief: Update the plot with the user inputs for plot manipulation
* 
* This function is executed everytime the user alters any of the plot manipulation inputs,
* which are the plot min, plot max, cursor position, and threshold, as well as the peak select inputs.
* 
*/
void MainWindow::updatePlotMetrics()
{
	//metrics->plotMin = ui->plot_min_entry->text().toInt();
	try {
		//plot manipulation inputs
		metrics -> plotMin = ui -> plot_min_entry -> text().toInt();
		metrics -> plotMax = ui -> plot_max_entry -> text().toInt();
		metrics -> cursor_xpos = ui -> channel_select_entry -> text().toInt();
		metrics -> threshold = ui -> threshold_entry -> text().toInt();

		//peak select inputs
		metrics ->peak1_xmin = ui -> lower_peak1 -> text().toInt();
		metrics ->peak1_xmax = ui -> upper_peak1 -> text().toInt();
		metrics ->peak2_xmin = ui -> lower_peak2 -> text().toInt();
		metrics ->peak2_xmax = ui -> upper_peak2 -> text().toInt();

		// Do something with the input value
		DEBUG_PRINT("plotMin = " << metrics->plotMin);
		DEBUG_PRINT("plotMax = " << metrics->plotMax);
	}
	catch (const std::invalid_argument& e) {
		// Handle the invalid input (e.g. show an error message)
		DEBUG_PRINT("Invalid input: " << e.what());
	}
	catch (const std::out_of_range& e) {
		// Handle the out-of-range input (e.g. show an error message)
		DEBUG_PRINT("Input out of range: " << e.what());
	}
}


/*
* @brief: Retrieves a copy of the settings of the acquisition device
* 
* This is the function that is called when the user clicks the "Apply" button on the device settings window.
* It is used to get the settings and pass them to the acquisition device, through the signal/slot mechanism, 
* which passes through the Model class.
* 
* @emits: settingsChanged signal with the new settings to the handler in the Model class
*/
void MainWindow::getCopySettings()
{
	settings->apply();
	//settings->printSettings();
	const DeviceSettings::Settings p = settings->m_currentSettings;
	emit settingsChanged(p);
}

void MainWindow::stopThread()
{
	metrics_thread->quit();
	metrics_thread->wait();
}

/*
* @brief: Block that is run when the data timer times out for every refresh cycle
* 
* It updates the metrics table with the new metrics, and emits a signal to the Model class 
* to handle the timeout
*/
void MainWindow::DataTimerTimeout()
{
	qint64 elapsed_time = data_timer.elapsedTime();
	DEBUG_PRINT("Elapsed time (ms) is: " << elapsed_time);
	this->metrics ->time_elapsed_ms = elapsed_time;
	this->metrics->elapsed_seconds = elapsed_time / 1000;
	this->metrics->live_time = elapsed_time / 1000;
	updateMetricsTable();
	//qint64 elapsed_time1 = data_timer.totalElapsedTime;
	emit dataTimerTimeoutSignal(elapsed_time);
	
}

/*
* @brief: Runs when data is received from the acquisition device through the Model class
* 
* It replaces the current metrics with the new updated metrics.
* Refreshed the plot and updates the metrics table. 
* It is triggered by the data timer time out, as the handler for the data timer timeout emits this signal.
*/
void MainWindow::handleNewDataRefreshLoop(const QVector<int> data_vec, const Metrics metrics)
{
	//settings_mutex->lock();
	plot_vals = data_vec;
	this->metrics->replace(metrics);
	DEBUG_PRINT("New data received");
	refreshPlot();
	updateMetricsTable();
	//settings_mutex->unlock();
}

/*
* @brief: Requests a save file to the Model class
* 
* The Model class is responsible for handling the save file request, because it has
* all the relevant information such as metrics and acquisition data.
*/
void MainWindow::requestSaveFile()
{
	emit saveFileRequested();

}

/*
* @brief : Handles the acquisition time end event
* 
* Resets the timers and clears the plot and the metrics table for the next acquisition
* It also sends the signal to the Model class to clear the metrics object for the next acquisition
* and starts the next acquisition if there are more acquisitions to be done.
*/
void MainWindow::onAcquisitionTimeEnd()
{
	if (this->metrics->current_acq < this->metrics->n_acq) {
		//this->metrics->print();
		this->clearPlot();
		data_timer.stop();
		data_timer_elapsed.invalidate();
		DEBUG_PRINT("Acquisition time has ended");
		DEBUG_PRINT("Starting next acquisition");
		emit clearMetricsObj();
		emit startButtonPressedSignal();
		data_timer.start(refresh_ms);
		data_timer_elapsed.restart();
		this->metrics->current_acq += 1;
		startButtonPressed();
		updateMetricsTable();
	}
	else
	{
		DEBUG_PRINT("Acquisition time has ended");
		DEBUG_PRINT("All acquisitions have been completed");
		//this->metrics->print();
		data_timer.stop();
		data_timer_elapsed.invalidate();
		updateMetricsTable();
	}
}

void MainWindow::clearPlot()
{
	//reset the series that makes up the plot points
	series->clear();
	plot_vals.fill(0);
	series->replace(blankPlotPoints);
	this->metrics->time_elapsed_ms = 0;
	this->metrics->elapsed_seconds = 0;
	this->metrics->total_counts = 0;
	this->metrics->acq_rate = 0;
	this->metrics->live_time = 0;
	if (acquisitionStatus == "Stopped")
	{
		//data_timer.start(refresh_ms);
		data_timer.reset();
	}
	else
	{
		data_timer.reset();
		data_timer.start(refresh_ms);
	}
	//data_timer.start(refresh_ms);
	emit clearSignal();
}

/*
* @brief: Handles the acquisition settings window "Apply" button click
* 
* Propagates the accepted settings when configuring the acquisition device to the Model class
*/
void MainWindow::getDefaultAcquisitionSettings()
{
	qint64 acquisitionTime = acquisitionSettingsWindow->getAcq_t();
	qint64 numAcquisitions = acquisitionSettingsWindow->getAcq_n();
	QString default_filename = acquisitionSettingsWindow->getDefault_filename();
	//QString save_directory = acquisitionSettingsWindow->onSelectDirectoryButtonClicked();
	emit acquisitionSettingsAccepted(acquisitionTime,
										numAcquisitions, 
										default_filename, 
										this->acquisitionSettingsWindow->save_dir);
}




void MainWindow::updatePlotBounds()
{

	plotMinStr = ui->plot_min_entry->text();
	plotMaxStr = ui->plot_max_entry->text();

	plotMin = plotMinStr.isEmpty() ? 0 : plotMinStr.toInt();
	plotMax = plotMaxStr.isEmpty() ? 1024 : plotMaxStr.toInt();

	if (plotMin >= 0 && plotMax <= 1024 && plotMin < plotMax)
	{
		chart->axisX()->setRange(plotMin, plotMax);
	}




}

/*
* @brief: Updates the cursor position and the counts value on the cursor for the selected channel in the plot
*/
void MainWindow::updateCursorPosition()
{
	cursorStr = ui->channel_select_entry->text();
	cursorChannel = cursorStr.isEmpty() ? -1 : cursorStr.toInt();
	DEBUG_PRINT("cursorChannel = " << cursorChannel);
	if (cursorChannel >= 0 && cursorChannel < 1024)
	{
		cursorLine->clear();
		QValueAxis* valueAxisY = qobject_cast<QValueAxis*>(chart->axisY());
		cursorLine->append(cursorChannel, valueAxisY->min());
		cursorLine->append(cursorChannel, valueAxisY->max());
		cursorYval = plot_vals[cursorChannel];
		ui->channel_select_counts->setText(QString::number(cursorYval));
	}
	else
	{
		cursorLine->clear();
	}
}

void MainWindow::toggleLinLog()
{
	QAbstractAxis* currentYAxis = chart->axisY();
	QAbstractSeries* currentSeries = chart->series().first();


	if (qobject_cast<QValueAxis*>(currentYAxis)) {
		// If the current Y-axis is linear, replace it with a logarithmic axis
		chart->removeAxis(currentYAxis);
		QLogValueAxis* logYAxis = new QLogValueAxis();
		logYAxis->setTitleText("Log(Number of Occurrences)");
		chart->addAxis(logYAxis, Qt::AlignLeft);
		currentSeries->attachAxis(logYAxis);
		delete currentYAxis;

	}
	else if (qobject_cast<QLogValueAxis*>(currentYAxis)) {
		// If the current Y-axis is logarithmic, replace it with a linear axis
		chart->removeAxis(currentYAxis);
		QValueAxis* linYAxis = new QValueAxis();
		linYAxis->setTitleText("Number of Occurrences");
		chart->addAxis(linYAxis, Qt::AlignLeft);
		currentSeries->attachAxis(linYAxis);
		delete currentYAxis;
	}



	// Refresh the plot to apply the new axis settings
	refreshPlot();
}

/*
* @brief: Handles the "Save File" button click event for a manual save file request
* 
* Still sends the signal to the Model class to handle the save file request, but it also
* sends the updated directory and filename to the Model class.
*/
void MainWindow::saveFileUserRequest()
{
	QString fileName = QFileDialog::getSaveFileName(
		this,
		tr("Save File"),
		"",
		tr("Text Files (*.csv);;All Files (*)"));

	if (!fileName.isEmpty())
	{
		// Extract the directory
		QFileInfo fileInfo(fileName);
		QString dirPath = fileInfo.absoluteDir().absolutePath();

		// Emit the signal with the directory and the filename
		emit saveFileSettingsChanged(fileName, dirPath);
	}

}


/*
* @brief: Handles the coloring of the peak selection regions in the plot
* 
*/
void MainWindow::getRegionBounds() {
	bool ok1, ok2, ok3, ok4;
	lowerValue1 = ui->lower_peak1->text().toInt(&ok1);
	upperValue1 = ui->upper_peak1->text().toInt(&ok2);
	lowerValue2 = ui->lower_peak2->text().toInt(&ok3);
	upperValue2 = ui->upper_peak2->text().toInt(&ok4);

	double counts1 = countsWithBackgroundRemoval(lowerValue1, upperValue1);
	double counts2 = countsWithBackgroundRemoval(lowerValue2, upperValue2);

	region1Color = QColor(247, 213, 149, 100);
	region2Color = QColor(171, 219, 227, 100);

	if (ok1 && ok2) {
		updateColoredRegion(lowerValue1, upperValue1, region1Color, coloredRegions1, areaSeries1);
		ui->counts1->setText(QString::number(counts1));
	}
	else {
		removeColoredRegion(coloredRegions1, areaSeries1);
		ui->counts1->setText("---");
	}

	if (ok3 && ok4) {
		updateColoredRegion(lowerValue2, upperValue2, region2Color, coloredRegions2, areaSeries2);
		ui->counts2->setText(QString::number(counts2));
	}
	else {
		removeColoredRegion(coloredRegions2, areaSeries2);
		ui->counts2->setText("---");
	}
}

/*
* @brief: Remover for the colored regions in the plot (used for the peak selection regions)
* 
* Removes all previous colored regions and deletes them from the heap, to only present the most current colored region
*/
void MainWindow::removeColoredRegion(QList<QAreaSeries*>& coloredRegions, QAreaSeries*& areaSeries) {
	if (areaSeries != nullptr) {
		chart->removeSeries(areaSeries);
		coloredRegions.removeAll(areaSeries);
		delete areaSeries;
		areaSeries = nullptr;
	}
}


/*
* @brief: Draws the colored regions in the plot (used for the peak selection regions)
*/
void MainWindow::updateColoredRegion(qreal a, qreal b, QColor color, QList<QAreaSeries*>& regionsList, QAreaSeries*& areaSeriesToUpdate)
{
	// Remove and delete previous colored regions
	for (QAreaSeries* region : regionsList) {
		if (region != nullptr) {
			chart->removeSeries(region);
			delete region;
		}
	}
	regionsList.clear(); // Clear the list

	QValueAxis* valueAxisY = qobject_cast<QValueAxis*>(chart->axisY());
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
	chart->addSeries(areaSeriesToUpdate);
	areaSeriesToUpdate->attachAxis(chart->axisX());
	areaSeriesToUpdate->attachAxis(chart->axisY());

	// Add the new region to the list
	regionsList.append(areaSeriesToUpdate);
}

/*
* @brief: Removes the background counts for the peak selection regions using a linear background removal
*/
double MainWindow::countsWithBackgroundRemoval(int xLower, int xUpper)
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
		}
	}

	return totalCounts;
}


/*
* @brief: Updater for the serial connection status to prevent acquisitions with unopened connections
*/
void MainWindow::updateSerialConnectionStatus(QString status)
{
	if (status == "Open")
	{
		serialPortStatus = status;
		DEBUG_PRINT("Status Updated on Main Window");
	}
	else if (status == "Closed")
	{
		serialPortStatus = status;
	}
	else if (status == "Error")
	{
		serialPortStatus = status;
	}
	else if (status == "Not Connected")
	{
		serialPortStatus = "Unknown";
	}
}

