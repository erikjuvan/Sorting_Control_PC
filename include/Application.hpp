#pragma once

#include "Communication.hpp"

#include <thread>

class MainWindow;
class AnalysisWindow;

class Application {
public:
	static constexpr int DATA_PER_CHANNEL = 100;
	static constexpr int N_CHANNELS{ 8 };	

	enum class Running { STOPPED, RUNNING };
	enum class Mode { LIVE, RECORD };
	enum class View { RAW, FILTERED };
	enum class Capture { ON, OFF };

	int m_n_samples;

	Running m_running;
	Mode	m_mode;
	View	m_view;
	Capture m_capture;

	Communication	*communication;
	MainWindow		*mainWindow;
	AnalysisWindow	*analysisWindow;

	std::thread thread_info;
	std::thread thread_get_data;

public:

	Application();
	~Application();

	void Run();
	void Information();
	void GetData();

private:

	void InitFromFile(const std::string& file_name);
};
