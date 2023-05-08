
#include "Metrics.h"

#ifndef NDEBUG // If in debug mode
#define DEBUG_PRINT(x) qDebug() << x
#else // If in release mode
#define DEBUG_PRINT(x) ((void)0) // No debug output
#endif

Metrics::Metrics()
{
	acq_status = "Not Acquiring";

	//get current time: 
	start_time = "--:--:--";
	preset_time = 3600;
	n_channels = 1024;
	n_acq = 2;
	current_acq = 1;
	elapsed_seconds = 0;
	time_elapsed_ms = 0;
	preset_time_ms = 30000;
	live_time = 0;
	total_counts = 0;
	acq_rate = 10;

	save_folder_name = "Data";
	save_file_name = "Data";
	start_button = false;
	stop_button = false;
	clear_button = false;
	lin_log = false;

	plotMax = n_channels;
	plotMin = 0;

	threshold = 0;
	cursor_xpos = 0;

	peak1_xmin = 0;
	peak1_xmax = 0;

	peak2_xmin = 0;
	peak2_xmax = 0;

	peak1_counts = 0;
	peak2_counts = 0;


}

Metrics::Metrics(const Metrics& newMetrics) {
	acq_status = newMetrics.acq_status;
	start_time = newMetrics.start_time;
	preset_time = newMetrics.preset_time;
	elapsed_seconds = newMetrics.elapsed_seconds;
	time_elapsed_ms = newMetrics.time_elapsed_ms;
	preset_time_ms = newMetrics.preset_time_ms;
	live_time = newMetrics.live_time;
	acq_rate = newMetrics.acq_rate;
	n_channels = newMetrics.n_channels;
	n_acq = newMetrics.n_acq;
	current_acq = newMetrics.current_acq;
	total_counts = newMetrics.total_counts;

	start_button = newMetrics.start_button;
	stop_button = newMetrics.stop_button;
	clear_button = newMetrics.clear_button;
	lin_log = newMetrics.lin_log;

	plotMax = newMetrics.plotMax;
	plotMin = newMetrics.plotMin;
	threshold = newMetrics.threshold;
	cursor_xpos = newMetrics.cursor_xpos;

	peak1_xmin = newMetrics.peak1_xmin;
	peak1_xmax = newMetrics.peak1_xmax;
	peak2_xmin = newMetrics.peak2_xmin;
	peak2_xmax = newMetrics.peak2_xmax;

	peak1_counts = newMetrics.peak1_counts;
	peak2_counts = newMetrics.peak2_counts;
}


Metrics::~Metrics()
{}

void Metrics::print() const
{
	qDebug() << " ######  Metrics: ###### ";
	qDebug() << "acq_status = " << acq_status;
	//qDebug() << "start_time = " << start_time;
	qDebug() << "preset_time = " << preset_time;
	//qDebug() << "n_channels = " << n_channels;
	qDebug() << "n_acq = " << n_acq;
	qDebug() << "current_acq = " << current_acq;
	//qDebug() << "elapsed_seconds = " << elapsed_seconds;
	qDebug() << "time_elapsed_ms = " << time_elapsed_ms;
	//qDebug() << "live_time = " << live_time;
	qDebug() << "total_counts = " << total_counts;
	qDebug() << "acq_rate = " << acq_rate;
	qDebug() << " ";

}

void Metrics::startAcquisition()
{
	acq_status = "Running";
	acq_running = true;
	start_time = QDateTime::currentDateTime().toString("hh:mm:ss");
	current_acq +=1;

}

void Metrics::pauseAcquisition()
{
	acq_status = "Paused";
	acq_running = false;
}

void Metrics::resumeAcquisition()
{
	acq_status = "Running";
	acq_running = true;
}


void Metrics::stopAcquisition()
{
	acq_status = "Not Acquiring";
	acq_running = false;
}
void Metrics::updateData()
{
	QString string = "test signal";
	elapsed_seconds = time_elapsed_ms / 1000;

	emit dataUpdated(string);
}



void Metrics::update(QVector<int> plot_data)
{
	//update the elapsed time
	elapsed_seconds = time_elapsed_ms / qreal(1000.0);
	int sum = 0;
	for (int value : plot_data) {
		sum += 1;
	}
	total_counts += sum;
	acq_rate = (elapsed_seconds == 0) ? 0 : (total_counts / elapsed_seconds);
	live_time = elapsed_seconds;

	
}

void Metrics::replace(const Metrics& newMetrics)
{
	
	acq_status = newMetrics.acq_status;
	start_time = newMetrics.start_time;
	preset_time = newMetrics.preset_time;
	elapsed_seconds = newMetrics.elapsed_seconds;
	time_elapsed_ms = newMetrics.time_elapsed_ms;
	preset_time_ms = newMetrics.preset_time_ms;
	live_time = newMetrics.live_time;
	acq_rate = newMetrics.acq_rate;
	n_channels = newMetrics.n_channels;
	n_acq = newMetrics.n_acq;
	current_acq = newMetrics.current_acq;
	total_counts = newMetrics.total_counts;
	
	start_button = newMetrics.start_button;
	stop_button = newMetrics.stop_button;
	clear_button = newMetrics.clear_button;
	lin_log = newMetrics.lin_log;
	
	plotMax = newMetrics.plotMax;
	plotMin = newMetrics.plotMin;
	threshold = newMetrics.threshold;
	cursor_xpos = newMetrics.cursor_xpos;
	
	peak1_xmin = newMetrics.peak1_xmin;
	peak1_xmax = newMetrics.peak1_xmax;
	peak2_xmin = newMetrics.peak2_xmin;
	peak2_xmax = newMetrics.peak2_xmax;
	
	peak1_counts = newMetrics.peak1_counts;
	peak2_counts = newMetrics.peak2_counts;

	
}

void Metrics::clear()
{
	acq_status = "Not Acquiring";
	start_time = "--:--:--";
	preset_time = 3600;
	elapsed_seconds = 0;
	time_elapsed_ms = 0;
	preset_time_ms = 3000;
	live_time = 0;
	total_counts = 0;
	acq_rate = 0;
	n_channels = 1024;
	n_acq = 2;
	current_acq = 0;
	start_button = false;
	stop_button = false;
	clear_button = false;
	lin_log = false;
	plotMax = n_channels;
	plotMin = 0;
	threshold = 0;
	cursor_xpos = 0;
	peak1_xmin = 0;
	peak1_xmax = 0;
	peak2_xmin = 0;
	peak2_xmax = 0;
	peak1_counts = 0;
	peak2_counts = 0;
}

void Metrics::reset(int acq_time_ms, int n_acqs, int curr_acq)
{
	acq_status = "Not Acquiring";
	start_time = "--:--:--";
	preset_time = 3600;
	elapsed_seconds = 0;
	time_elapsed_ms = 0;
	preset_time_ms = acq_time_ms;
	live_time = 0;
	total_counts = 0;
	acq_rate = 0;
	n_channels = 1024;
	n_acq = n_acqs;
	current_acq = curr_acq;
	start_button = false;
	stop_button = false;
	clear_button = false;
	lin_log = false;
	plotMax = n_channels;
	plotMin = 0;
	threshold = 0;
	cursor_xpos = 0;
	peak1_xmin = 0;
	peak1_xmax = 0;
	peak2_xmin = 0;
	peak2_xmax = 0;
	peak1_counts = 0;
	peak2_counts = 0;
}

void Metrics::updateTimingMetrics(float elapsed_ms)
{
	time_elapsed_ms = elapsed_ms;
	elapsed_seconds = elapsed_ms / 1000;
	live_time = elapsed_seconds;
	acq_rate = round((elapsed_seconds == 0) ? 0 : (total_counts / elapsed_seconds)*100)/100;
}