#pragma once

#include <QObject>
#include "DeviceSettings.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QByteArray>
#include <QVector>
#include <QMutex>

class Device : public QObject
{
	Q_OBJECT

		public:
			Device(QObject* parent = nullptr);
			Device( QMutex *read_mutex, QObject* parent = nullptr);
			~Device();
			QSerialPort* serial;
			DeviceSettings* settings;
			QByteArray data;
			QByteArray lineData;
			QString line;
			QVector<int> ADC_Reading_vec;
			QString string;
			//QRegularExpression regex("\\d+");
			QByteArray clearCommand;
			QStringList list;
			int pos;
			bool ok;
			int value;
			QMutex* serial_mutex;
			

		public:
			void openSerialPort();
			void clearSerialBuffer();
			void launch_DeviceSetup();
			//void on_pushButton_clicked();
			QVector<int> readData();
			void handleError(QSerialPort::SerialPortError error);
			void closeSerialPort();
			void updateDeviceSettings();	
			void sendClearBufferCommand();
			void clearADCVec();
	
	public slots:
		void onReadyRead();
		void prepareAcquisition();
	
	signals: 
		//void newDataReceived(const QByteArray& data);
		void newDataReceived(const QVector<int> data_vec);
		void serialPortStatusUpdate(QString status);

};

