#pragma once

#include <chrono>
#include <iostream>
#include <ctime>
#include <QString>
#include <QLabel>
#include <QDateTime>
#include <QDir>


class Metrics : public QObject 
{
	Q_OBJECT
	public:

		Metrics();
		Metrics(const Metrics& newMetrics);
		~Metrics();

		//metrics pertaining to the acquisition 
		QString acq_status;
		float duration_s;
		QString start_time;
		qreal preset_time;
		qreal elapsed_seconds;
		qint64 time_elapsed_ms;
		qint64 preset_time_ms;
		float live_time;
		float acq_rate;
		int n_channels;
		int n_acq;
		int current_acq;
		int total_counts;
		QDir save_dir;
		QString save_folder_name;
		QString save_file_name;


		//bool values pertaining to the buttons
		bool start_button;
		bool acq_running;
		bool stop_button;
		bool clear_button;
		bool lin_log;
		
		//values and metrics pertaining to the plot

		int plotMax;
		int plotMin;
		int threshold;
		int cursor_xpos;

		//values and metrics pertaining to the peak selection

		int peak1_xmin;
		int peak1_xmax;
		int peak2_xmin;
		int peak2_xmax;

		int peak1_counts;
		int peak2_counts;


	public:
		void print() const;
		void update(QVector<int> plot_data);
		void replace(const Metrics& newMetrics);
		void startAcquisition();
		void resumeAcquisition();
		void pauseAcquisition();
		void stopAcquisition();
		void clear();
		void reset(int preset_time_ms, int n_acq, int curr_acq);

	public slots:
		void updateData();

		
		 
	signals: 
		void dataUpdated(QString string);
};