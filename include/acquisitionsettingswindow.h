#ifndef ACQUISITIONSETTINGSWINDOW_H
#define ACQUISITIONSETTINGSWINDOW_H

#include <QDialog>
#include <QFileDialog>
#include <QPushButton>
#include "ui_acquisitionsettingswindow.h"   
#include <QDir>
#include <limits.h>

namespace Ui {
    class AcquisitionSettingsWindow;
}

class AcquisitionSettingsWindow : public QDialog
{
        Q_OBJECT
    
    public:
        explicit AcquisitionSettingsWindow(QWidget* parent = nullptr);
        ~AcquisitionSettingsWindow();
    
        qint64 getAcq_t();
        qint64 getAcq_n();
        int getRefresh_ms();
        QString getDefault_filename();
        QString save_dir;

        qint64 acq_t;
        qint64 acq_n;
        int refresh_ms;
        QString default_filename;
        QString directory;
        bool infiniteAcquisition;
    
    public slots:
        QString onSelectDirectoryButtonClicked();
        void on_single_run_button_clicked();
    
    private:
        Ui::AcquisitionSettings* ui;
};

#endif // ACQUISITIONSETTINGSWINDOW_H

