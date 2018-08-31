#pragma once

#include "Window.hpp"
#include "Application.hpp"
#include <mygui/Button.hpp>
#include <mygui/Label.hpp>

struct SortingAnalysis {
	struct Channel {
		int min, max, avg, last;
		int cnt;
		int sum;

		Channel();
	};

	Channel channel[N_CHANNELS];
	Channel channel_total;

	void ClearAll();
	void Add(uint32_t* data, int size);	
};

class AnalysisWindow : public Window {
private:	
	SortingAnalysis *m_analysis;

public:
	// Methods
	//////////

	AnalysisWindow(int w, int h, const char* title, sf::Uint32 style = sf::Style::Default);
	~AnalysisWindow();
	
	void NewData(uint32_t *data, int size);

	static void button_clear_all_Clicked();

	// Members
	//////////

	mygui::Button	*button_clear_all;

	mygui::Label	*label_info_win_to_det_min;
	mygui::Label	*label_info_win_to_det_max;
	mygui::Label	*label_info_win_to_det_avg;
	mygui::Label	*label_info_win_to_det_last;
	mygui::Label	*label_info_win_to_det_cnt;	

	struct InfoLabel {
		mygui::Label* channel_number;
		mygui::Label* label_min;
		mygui::Label* label_max;
		mygui::Label* label_avg;
		mygui::Label* label_last;
		mygui::Label* label_cnt;

		~InfoLabel();
	};

	InfoLabel labels[N_CHANNELS];
	InfoLabel label_all;
};