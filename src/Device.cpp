#include "Device.h"
#include <QRegularExpression>
#include <random>

#ifndef NDEBUG // If in debug mode
#define DEBUG_PRINT(x) qDebug() << x
#else // If in release mode
#define DEBUG_PRINT(x) ((void)0) // No debug output
#endif


Device::Device(QObject* parent) : QObject(parent)
{
	settings = new DeviceSettings();
	serial = new QSerialPort();
	data.resize(10000);
	connect(serial, &QSerialPort::readyRead, this, &Device::onReadyRead);
	clearCommand = "CLEAR_BUFFER";
	pos = 0;
}


Device::Device(QMutex *read_mutex, QObject* parent) : QObject(parent),
	serial_mutex(read_mutex)
{
	settings = new DeviceSettings();
	serial = new QSerialPort();
	data.resize(10000);
	connect(serial, &QSerialPort::readyRead, this, &Device::onReadyRead);
	clearCommand = "CLEAR_BUFFER";
	pos = 0;
}

Device::~Device()
{
	delete settings;
	serial->close();
	delete serial;
}

void Device::openSerialPort()
{

	if (serial->isOpen()) {
		qDebug() << "Serial port is already open";
		emit serialPortStatusUpdate("Open");
	}

	const DeviceSettings::Settings p = settings->settings();
	serial->setPortName(p.name);
	serial->setBaudRate(p.baudRate);
	serial->setDataBits(p.dataBits);
	serial->setParity(p.parity);
	serial->setStopBits(p.stopBits);
	serial->setFlowControl(p.flowControl);
	if (serial->open(QIODevice::ReadWrite)) {
		//print to the console "Connected"
		DEBUG_PRINT("Serial Port is Connected");
		serial->clear();
		//serial->readAll();
		emit serialPortStatusUpdate("Open");
	}
	else {
		//print to the console "Not Connected"
		DEBUG_PRINT("Not Connected");
		qDebug() << "Error:" << serial->errorString();
		emit serialPortStatusUpdate("Not Connected");
	}

	if (serial->open(QIODevice::ReadWrite))
	{
		clearSerialBuffer();
	}

	this->prepareAcquisition();


}


void Device::closeSerialPort()
{
	
	if (serial->isOpen())
	{ 
		serial->close();
		emit serialPortStatusUpdate("Closed");
	}
	else
	{
		qDebug() << "Serial port is already closed";
		emit serialPortStatusUpdate("Closed");
	}
}



void Device::handleError(QSerialPort::SerialPortError error)
{
	if (error == QSerialPort::ResourceError) {
		//QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
		closeSerialPort();
	}
}


void Device::onReadyRead()
{
    // lock the mutex
	serial_mutex->lock();
	while (serial->canReadLine()) {
		lineData = serial->readLine();
		line = QString::fromUtf8(lineData).trimmed();

		
		value = line.toInt(&ok);

		if (ok) {
			ADC_Reading_vec.push_back(value);
			emit newDataReceived(ADC_Reading_vec);
		}
		else {
			qDebug() << "Invalid data received" << line;
		}
	}

	//emit newDataReceived(ADC_Reading_vec);
	serial_mutex->unlock();
}



void Device::updateDeviceSettings()
{
	//this->settings->apply();
}


void Device::clearSerialBuffer()
{
	serial->clear(QSerialPort::Input);
}

void Device::sendClearBufferCommand()
{
	serial->write(clearCommand);
}

void Device::clearADCVec()
{
	ADC_Reading_vec.clear();
}

void Device::prepareAcquisition()
{
	clearADCVec();
	clearSerialBuffer();
}