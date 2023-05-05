#pragma once

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSerialPort>

QT_BEGIN_NAMESPACE

namespace Ui {
    class DeviceSettings;
}

class QIntValidator;

QT_END_NAMESPACE

class DeviceSettings : public QDialog
{
    Q_OBJECT

public:
    struct Settings {
        QString name;
        qint32 baudRate;
        QString stringBaudRate;
        QSerialPort::DataBits dataBits;
        QString stringDataBits;
        QSerialPort::Parity parity;
        QString stringParity;
        QSerialPort::StopBits stopBits;
        QString stringStopBits;
        QSerialPort::FlowControl flowControl;
        QString stringFlowControl;
        bool localEchoEnabled;
    };

    explicit DeviceSettings(QWidget* parent = nullptr);
    ~DeviceSettings();

    Settings settings() ;


public slots:
    void showPortInfo(int idx);
    void apply();
    void checkCustomBaudRatePolicy(int idx);
    void checkCustomDevicePathPolicy(int idx);

private:
    void fillPortsParameters();
    void fillPortsInfo();
    void updateSettings();


public:
    Ui::DeviceSettings* m_ui = nullptr;
    Settings m_currentSettings;
    QIntValidator* m_intValidator = nullptr;
    QPushButton* getApplyButton();

    void printSettings();
    
};

#endif // SETTINGSDIALOG_H
