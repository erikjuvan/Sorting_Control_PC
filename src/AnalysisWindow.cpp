#include "AnalysisWindow.hpp"

extern AnalysisWindow	*g_analysisWindow;

SortingAnalysis::Channel::Channel() : min(1000), max(0), avg(0), last(0), cnt(0), sum(0) {
}

void SortingAnalysis::ClearAll() {
	for (auto& c : channel) {
		c.min = 1000;
		c.max = c.avg = c.last = c.cnt = c.sum = 0;
	}
}

void SortingAnalysis::Add(uint32_t* data, int size) {
	bool isChange = false;
	for (int i = 0; i < size; ++i) {
		int val = data[i] & 0x00FFFFFF;
		if (val > 0) {
			isChange = true;
			int ch = ((data[i] >> 24) & 0xFF);
			channel[ch].last = val;
			channel[ch].sum += val;
			channel[ch].cnt++;
			if (channel[ch].min > val) channel[ch].min = val;
			if (channel[ch].max < val) channel[ch].max = val;
		}		
	}
	for (int i = 0; i < N_CHANNELS; ++i) {
		if (channel[i].cnt > 0)
			channel[i].avg = channel[i].sum / channel[i].cnt;
	}

	if (isChange) {
		channel_total.min = 1000;
		channel_total.max = channel_total.avg = channel_total.cnt = channel_total.sum = 0;
		for (int i = 0; i < N_CHANNELS; ++i) {
			channel_total.sum += channel[i].sum;
			channel_total.cnt += channel[i].cnt;
			if (channel_total.min > channel[i].min) channel_total.min = channel[i].min;
			if (channel_total.max < channel[i].max) channel_total.max = channel[i].max;
		}
		if (channel_total.cnt > 0)
			channel_total.avg = channel_total.sum / channel_total.cnt;
	}	
}

// Callback
///////////
void AnalysisWindow::button_clear_all_Clicked() {
	g_analysisWindow->m_analysis->ClearAll();
}
///////////


void AnalysisWindow::NewData(uint32_t* data, int size) {
	m_analysis->Add(data, size);

	for (int i = 0; i < N_CHANNELS; ++i) {		
		labels[i].label_min->SetText(std::to_string(m_analysis->channel[i].min));
		labels[i].label_max->SetText(std::to_string(m_analysis->channel[i].max));
		labels[i].label_avg->SetText(std::to_string(m_analysis->channel[i].avg));
		labels[i].label_last->SetText(std::to_string(m_analysis->channel[i].last));
		labels[i].label_cnt->SetText(std::to_string(m_analysis->channel[i].cnt));		
	}	
	label_all.label_min->SetText(std::to_string(m_analysis->channel_total.min));
	label_all.label_max->SetText(std::to_string(m_analysis->channel_total.max));
	label_all.label_avg->SetText(std::to_string(m_analysis->channel_total.avg));
	//label_all.label_last->SetText(std::to_string(m_analysis->channel_total.last)); // can't get last value
	label_all.label_cnt->SetText(std::to_string(m_analysis->channel_total.cnt));
}

AnalysisWindow::AnalysisWindow(int w, int h, const char* title, sf::Uint32 style) : Window(w, h, title, style) {
	m_analysis = new SortingAnalysis;
	
	button_clear_all = new mygui::Button(140, 10, "Clear", 90, 25);
	button_clear_all->OnClick(&AnalysisWindow::button_clear_all_Clicked);
	Add(button_clear_all);

	label_info_win_to_det_min = new mygui::Label(80, 50, "Min");
	Add(label_info_win_to_det_min);
	label_info_win_to_det_max = new mygui::Label(140, 50, "Max:");
	Add(label_info_win_to_det_max);
	label_info_win_to_det_avg = new mygui::Label(200, 50, "Avg:");
	Add(label_info_win_to_det_avg);
	label_info_win_to_det_last = new mygui::Label(260, 50, "Last:");
	Add(label_info_win_to_det_last);
	label_info_win_to_det_cnt = new mygui::Label(320, 50, "N: ");
	Add(label_info_win_to_det_cnt);

	for (int i = 0; i < N_CHANNELS; ++i) {
		labels[i].channel_number = new mygui::Label(20, 80 + i * 30, ("Ch " + std::to_string(i+1)).c_str());
		Add(labels[i].channel_number);
		labels[i].label_min = new mygui::Label(80, 80 + i * 30, ("N/A"));
		Add(labels[i].label_min);
		labels[i].label_max = new mygui::Label(140, 80 + i * 30, ("N/A"));
		Add(labels[i].label_max);
		labels[i].label_avg = new mygui::Label(200, 80 + i * 30, ("N/A"));
		Add(labels[i].label_avg);
		labels[i].label_last = new mygui::Label(260, 80 + i * 30, ("N/A"));
		Add(labels[i].label_last);
		labels[i].label_cnt = new mygui::Label(320, 80 + i * 30, ("N/A"));
		Add(labels[i].label_cnt);
	}

	label_all.channel_number = new mygui::Label(20, 80 + 8 * 30, "All");
	Add(label_all.channel_number);
	label_all.label_min = new mygui::Label(80, 80 + 8 * 30, ("N/A"));
	Add(label_all.label_min);
	label_all.label_max = new mygui::Label(140, 80 + 8 * 30, ("N/A"));
	Add(label_all.label_max);
	label_all.label_avg = new mygui::Label(200, 80 + 8 * 30, ("N/A"));
	Add(label_all.label_avg);
	label_all.label_last = new mygui::Label(260, 80 + 8 * 30, ("N/A"));
	Add(label_all.label_last);
	label_all.label_cnt = new mygui::Label(320, 80 + 8 * 30, ("N/A"));
	Add(label_all.label_cnt);
}


AnalysisWindow::~AnalysisWindow() {
	delete m_analysis;
	delete button_clear_all;
	delete label_info_win_to_det_min;
	delete label_info_win_to_det_max;
	delete label_info_win_to_det_avg;
	delete label_info_win_to_det_last;
	delete label_info_win_to_det_cnt;
}

AnalysisWindow::InfoLabel::~InfoLabel() {
	delete channel_number;
	delete label_min;
	delete label_max;
	delete label_avg;
	delete label_last;
	delete label_cnt;
}