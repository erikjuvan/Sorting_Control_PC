#pragma once

#include "Window.hpp"
#include "Application.hpp"
#include <mygui/Button.hpp>
#include <mygui/Label.hpp>

struct SortingAnalysis {
	struct Channel {
		uint8_t min, max, avg, last;
		int cnt;
		int sum;
	};

	Channel channel[Application::N_CHANNELS];

	void Add(uint16_t* data, int size);
};

class AnalysisWindow {
private:
	Window	*m_window;
	Application	*m_application;
	SortingAnalysis *m_analysis;

public:
	// Methods
	//////////

	AnalysisWindow(int w, int h, const char* title, Application* application);
	~AnalysisWindow();

	inline void Run() { m_window->Run(); }
	bool IsOpen();
	void Show();
	void Hide();
	void Update(uint16_t* data, int size);

	static void button_clear_all_Clicked(void*);

	// Members
	//////////

	mygui::Button	*button_clear_all;

	mygui::Label	*label_info_win_to_det_min;
	mygui::Label	*label_info_win_to_det_max;
	mygui::Label	*label_info_win_to_det_avg;
	mygui::Label	*label_info_win_to_det_last;

	struct InfoLabel {
		mygui::Label* channel_number;
		mygui::Label* label_min;
		mygui::Label* label_max;
		mygui::Label* label_avg;
		mygui::Label* label_last;
	};

	InfoLabel labels[Application::N_CHANNELS];
};