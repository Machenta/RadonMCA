#pragma once
#include <QObject>
#include <QTimer>
#include "Metrics.h"
#include "DeviceSettings.h"
#include <QMutex>
#include <QMutexLocker>
#include "Device.h"
#include <QDir>



class Model : public QObject
{
    Q_OBJECT
    public:
		Model(QMutex* savefile_mutex, QMutex* settings_mutex, QObject* parent = nullptr);
		~Model();

		Metrics* metrics; 
		DeviceSettings* settings;
		Device* device;
		//this is the vector that stores the counts for each channel
		QVector<int> data_vec;
		QString header;
		int current_acq;
		bool acquisitionRunning;
		bool flag;
		QMutex* serial_mutex;
		

	private:
		QMutex* savefile_mutex;
		QMutex* settings_mutex;

	public:
		void setData();
		void updateDeviceSettings();
		void startAquisition();
		void updateDataVec(const QVector<int> serial_data);
		void createSaveDirectory();
		void saveData();
		void createHeader();
		void saveSettings();
		

	public slots:
		void handleSettingsChanged(DeviceSettings::Settings settings);
		void onStartButtonPressed();
		void onStopButtonPressed();
		void onAcquisitionTimeEnd();
		void handleDataTimerTimeout(qint64 time_elapsed_ms);
		void saveFile();
		void onAcquisitionSettingsAccepted(qint64 tAcq, qint64 nAcq, QString default_filename, QString save_directory);
		void onDataReceivedFromSerial(const QVector<int> data_vec);
		void onPauseButtonPressed();
		void onResumeButtonPressed();
		void onSaveFileSettingsChanged(QString filename, QString save_dir);
		void onSerialConnectionUpdateStatus(QString status);
		void handleResumeAcquisition();
		void handleStopAcquisition();
		void handleClear();

	signals:
		void sendAcquisitionEndSignal();
		void sendDataToViewer(const QVector<int> data_vec, const Metrics metrics);
		void serialPortConnectionStatusUpdate(QString status);
		
};

