#include "Model.h"
#include "MainWindow.h"

#ifndef NDEBUG // If in debug mode
#define DEBUG_PRINT(x) qDebug() << x
#else // If in release mode
#define DEBUG_PRINT(x) ((void)0) // No debug output
#endif

Model::Model(QMutex* savefile_mutex, QMutex* settings_mutex , QObject *parent)
	: savefile_mutex(savefile_mutex),
	settings_mutex(settings_mutex),
	metrics(new Metrics()),
	settings(new DeviceSettings())
	,device (new Device())
{
	connect(device, &Device::newDataReceived, this, &Model::onDataReceivedFromSerial);
	connect(device, &Device::serialPortStatusUpdate, this, &Model::onSerialConnectionUpdateStatus);
	current_acq = 0;
	data_vec.resize(1024);
	data_vec.fill(0);
	createSaveDirectory();
	acquisitionRunning = false;
	// initialize serial_mutex
	this->serial_mutex = new QMutex();
	this->device->serial_mutex = serial_mutex;
	//device = new Device(serial_mutex);
	this->metrics->acq_status = "Not Running";
}

Model::~Model()
{
	delete metrics;
	delete settings;
	delete device;
	
}

void Model::handleSettingsChanged(DeviceSettings::Settings settings)
{
	this->settings->m_currentSettings = settings;
	this->device->settings->m_currentSettings = settings;
	this->device->updateDeviceSettings();


	//this->settings->printSettings();

}

void Model::updateDeviceSettings()
{
	this->device->settings->m_currentSettings = this->settings->m_currentSettings;
}

void Model::onStartButtonPressed()
{

	//this only need to be called once
	if (this->metrics->current_acq == 1) {
		if (this->metrics->current_acq < this->metrics->n_acq){
			DEBUG_PRINT("Acquisition Number: " + QString::number(metrics->current_acq));
			if (metrics->current_acq < metrics->n_acq && metrics->start_button == false)
			{
				QDateTime currentDateTime = QDateTime::currentDateTime();
				QString dateTimeStr = currentDateTime.toString("yyyy-MM-dd \n hh:mm:ss");
				this->metrics->start_time = dateTimeStr;
				this->device->openSerialPort();
				this->device->prepareAcquisition();
				this->metrics->acq_status = "Running";
			}
		}
	}
}

void Model::onPauseButtonPressed()
{
	//this->device->pauseAcquisition();
}

void Model::onResumeButtonPressed()
{
	//this->device->resumeAcquisition();
}

void Model::onStopButtonPressed()
{
	// Perform actions when the stop button is pressed
}


void Model::handleDataTimerTimeout(qint64 time_elapsed_ms)
{
	//settings_mutex->lock();
	this->metrics->updateTimingMetrics(time_elapsed_ms);
	//metrics->time_elapsed_ms = time_elapsed_ms;
	//metrics->elapsed_seconds = time_elapsed_ms / 1000;
	//metrics->live_time = metrics->elapsed_seconds;
	//metrics->acq_rate = metrics->total_counts / metrics->elapsed_seconds;
	flag = false;

	//if the time elapsed is greater than the preset time, 
	//then save the data_vec and reset the metrics
	if (metrics->time_elapsed_ms > metrics->preset_time_ms && !flag)
	{
		DEBUG_PRINT("metrics->time_elapsed_ms" << metrics->time_elapsed_ms);
		this->current_acq += 1;
		DEBUG_PRINT("END OF ACQUISITION");
		//DEBUG_PRINT("Time elapsed (ms): " + QString::number(metrics->time_elapsed_ms));
		onAcquisitionTimeEnd();
		//this->metrics->reset(3000,2, this->current_acq);
		this->metrics->time_elapsed_ms = 0;
		this->metrics->elapsed_seconds = 0;
		this->metrics->current_acq += 1;
		this->metrics->total_counts = 0;
		this->metrics->acq_rate = 0;
		//clear the data_vec and reset the metrics
		this->saveFile();
		data_vec.fill(0);
		this->metrics->acq_status = "Not Running";
		flag = true;

		//this->device->readData();
		//updateDataVec();
		//this->metrics->update(data_vec);

		//return;
	}

	else {
		//this->device->readData();
		//updateDataVec();
		//this->metrics->update(data_vec);
	}

	emit sendDataToViewer(data_vec, *metrics);
	//DEBUG_PRINT("Time elapsed (ms): " + QString::number(metrics->time_elapsed_ms));
	//DEBUG_PRINT("Real time elapsed (ms): " + QString::number(metrics->time_elapsed_ms))
	//settings_mutex->unlock();
}

void Model::onAcquisitionTimeEnd()
{
	emit sendAcquisitionEndSignal();
}

void Model::updateDataVec(const QVector<int> serial_data)
{

	for (int i = 0; i < serial_data.size(); i++) {
		if (serial_data.at(i) < 1024 && serial_data.at(i) > 0 ) {
			data_vec[serial_data.at(i)] += 1;
			//DEBUG_PRINT("data_vec[" + QString::number(serial_data.at(i)) + "] = " + QString::number(data_vec[serial_data.at(i)]));
		}
	}
}

void Model::createSaveDirectory()
{
	//set the working directory as the current directory
	this->metrics->save_dir.setCurrent(QDir::currentPath());
	if (!this->metrics->save_dir.exists(metrics->save_folder_name))
	{
		//if it doesn't exist, create it
		this->metrics->save_dir.mkdir(metrics->save_folder_name);
	}
	this->metrics->save_dir.setCurrent(metrics->save_folder_name);
}

void Model::saveData()
{

	QString file_name = this->metrics->save_file_name + QString::number((metrics->current_acq)-1) + ".csv";
	QFile file(file_name);
	this->createHeader();
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	//create a text stream
	QTextStream out(&file);
	out << this->header << "\n";
	for (int i = 0; i < data_vec.size(); i++) {
		out << data_vec[i] << "\n";
	}
	file.close();
}

void Model::saveSettings()
{
	//save the settings
	QString file_name = this->metrics->save_file_name + QString::number(metrics->current_acq) + "_settings.csv";
	QFile file(file_name);
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	//create a text stream
	QTextStream out(&file);
	out << "Preset time (ms): " << metrics->preset_time_ms << "\n";
	out << "Number of acquisitions: " << metrics->n_acq << "\n";
	out << "Number of channels: " << metrics->n_channels << "\n";
}

void Model::createHeader() 
{
	//create the header for each acquisition file based on the metrics of the acquisition
	header  = "Preset time (ms): " + QString::number(metrics->preset_time_ms) + "\n";
	header += "Number of acquisitions: " + QString::number(metrics->n_acq) + "\n";
	header += "Number of channels: " + QString::number(metrics->n_channels) + "\n";
	header += "Total counts of samples: " + QString::number(metrics->total_counts) + "\n";
	header += "Start time: " + metrics->start_time + "\n";
}

void Model::saveFile()
{
	
	savefile_mutex->lock();
	this->saveData();
	savefile_mutex->unlock();

}

void Model::onAcquisitionSettingsAccepted(qint64 tAcq, qint64 nAcq, QString default_filename, QString save_directory)
{
	DEBUG_PRINT("Acquisition settings accepted in Model");
	this->metrics->preset_time_ms = tAcq*1000;
	this->metrics->preset_time = tAcq;
	this->metrics->n_acq = nAcq;
	this->metrics->save_file_name = default_filename;
	this->metrics->save_dir.setCurrent(save_directory);
}

void Model::onDataReceivedFromSerial(const QVector<int> serial_vec)
{
	if (this->metrics->acq_status == "Running") {
		serial_mutex->lock();
		updateDataVec(serial_vec);
		this->metrics->update(serial_vec);
		device->clearADCVec();
		serial_mutex->unlock();
	}
	else {
		serial_mutex->lock();
		device->clearADCVec();
		serial_mutex->unlock();
	}
}

void Model::onSaveFileSettingsChanged(QString filename, QString save_dir)
{
	savefile_mutex->lock();
	this->metrics->save_dir.setCurrent(save_dir);
	//DEBUG_PRINT("Save file settings changed in Model");

	QFile file(filename);
	this->createHeader();
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	//create a text stream
	QTextStream out(&file);
	out << this->header << "\n";
	for (int i = 0; i < data_vec.size(); i++) {
		out << data_vec[i] << "\n";
	}

	file.close();
	savefile_mutex->unlock();
}

void Model::onSerialConnectionUpdateStatus(QString status)
{
	emit serialPortConnectionStatusUpdate(status);
}

void Model::handleResumeAcquisition()
{
	this->device->clearADCVec();
	this->device->clearSerialBuffer();
	this->metrics->acq_status = "Running";
}

void Model::handleStopAcquisition()
{
	this->metrics->acq_status = "Paused";
}

void Model::handleClear() {
	this->metrics->time_elapsed_ms = 0;
	this->metrics->elapsed_seconds = 0;
	this->metrics->total_counts = 0;
	this->metrics->acq_rate = 0;
	//clear the data_vec and reset the metrics
	//this->saveFile();
	data_vec.fill(0);
	QDateTime currentDateTime = QDateTime::currentDateTime();
	QString dateTimeStr = currentDateTime.toString("yyyy-MM-dd \n hh:mm:ss");
	this->metrics->start_time = dateTimeStr;
}