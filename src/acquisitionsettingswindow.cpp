#include "acquisitionsettingswindow.h"


AcquisitionSettingsWindow::AcquisitionSettingsWindow(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::AcquisitionSettings)
{
    ui->setupUi(this);
	connect(ui->select_directory_button, &QPushButton::clicked, this, &AcquisitionSettingsWindow::onSelectDirectoryButtonClicked);
	connect(ui->single_run_button, SIGNAL(clicked()), this, SLOT(on_single_run_button_clicked()));

	//default acquisition settings
	ui->t_acquisition_entry->setText("3600");
	ui->n_acquisitions_entry->setText("10");
	ui->default_filename_entry->setText("Data");
	ui->directory_entry->setText(QDir::currentPath());
	ui->refresh_interval_entry->setText("50");
	save_dir = QDir::currentPath();

	acq_t = 3600;
	acq_n = 10;
	refresh_ms = 50;
	default_filename = "Data";
	directory = QDir::currentPath();
	infiniteAcquisition = false;

}

AcquisitionSettingsWindow::~AcquisitionSettingsWindow()
{
	delete ui;
}

qint64 AcquisitionSettingsWindow::getAcq_t()
{
	if (infiniteAcquisition) {
		acq_t = INT_MAX;
	}
	else{
		acq_t = ui->t_acquisition_entry->text().toInt();
	}
    return acq_t;
}

qint64 AcquisitionSettingsWindow::getAcq_n()
{
	if (infiniteAcquisition) {
		acq_n = 1;
	}
	else {
		acq_n = ui->n_acquisitions_entry->text().toInt();
	}
	return acq_n;
}

int AcquisitionSettingsWindow::getRefresh_ms()
{
	refresh_ms = ui->refresh_interval_entry->text().toInt();
	return refresh_ms;
}

QString AcquisitionSettingsWindow::getDefault_filename()
{
	
	default_filename = ui->default_filename_entry->text();
	return default_filename;
}

QString AcquisitionSettingsWindow::onSelectDirectoryButtonClicked()
{
	
	directory = QFileDialog::getExistingDirectory(this, tr("Select Directory"), QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	//QString directory = "";
	//if (!directory.isEmpty())
	//{
	//	
	//}

	ui->directory_entry->setText(directory);
	this->save_dir = directory;
	return directory;
}

void AcquisitionSettingsWindow::on_single_run_button_clicked()
{
	ui->n_acquisitions_entry->setEnabled(false);
	ui->t_acquisition_entry->setEnabled(false);
	acq_t = INT_MAX;
	acq_n = 1;

	ui->n_acquisitions_entry->setText("1");
	ui->t_acquisition_entry->setText("Infinite");
	infiniteAcquisition = true;
}


