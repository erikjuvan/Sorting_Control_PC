#include "AnalysisWindow.hpp"
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>

extern AnalysisWindow	*g_analysisWindow;

SortingAnalysis::Channel::Channel() {
	ClearAll();
	record_buf.reserve(1000);
}

void SortingAnalysis::Channel::ClearAll() {
	min = 1000;
	max = avg = prev_avg = std_dev = S = last = cnt = sum = 0;
	record_buf.clear();
}

void SortingAnalysis::ClearAll() {
	for (auto& c : channel) {
		c.ClearAll();
	}
	total.ClearAll();
}

void SortingAnalysis::Add(uint32_t* data, int size) {

	for (int i = 0; i < size; ++i) {
		int val = data[i] & 0x00FFFFFF;
		if (val > 0) {
			int ch = ((data[i] >> 24) & 0xFF);

			channel[ch].last = val;
			channel[ch].sum += val;
			channel[ch].cnt++;
			if (channel[ch].min > val) channel[ch].min = val;
			if (channel[ch].max < val) channel[ch].max = val;

			// Std dev
			channel[ch].prev_avg = channel[ch].avg;
			channel[ch].avg = channel[ch].sum / channel[ch].cnt;
			channel[ch].S = channel[ch].S + (val - channel[ch].avg) * (val - channel[ch].prev_avg);
			channel[ch].std_dev = sqrt(channel[ch].S / channel[ch].cnt);			

			///////////
			// Total //
			///////////
			total.sum += val;
			total.cnt++;
			if (total.min > val) total.min = val;
			if (total.max < val) total.max = val;

			// Std dev
			total.prev_avg = total.avg;
			total.avg = total.sum / total.cnt;
			total.S = total.S + (val - total.avg) * (val - total.prev_avg);
			total.std_dev = sqrt(total.S / total.cnt);

			////////////
			// Record //
			////////////
			if (m_record) {
				channel[ch].record_buf.push_back(val);
				total.record_buf.push_back(val);				
			}			
		}		
	}
}

void SortingAnalysis::SaveRecord(char const* fname) {	
	std::ofstream f(fname, std::ios::out | std::ios::app);	// open for writting and append data
	if (f.is_open()) {
		for (int i = 0; i < N_CHANNELS; ++i) {
			std::string buf;
			for (auto const &num : channel[i].record_buf) {
				buf += std::to_string(num) + ",";
			}
			if (buf.size() > 0)
				buf.pop_back();
			buf += '\n';
			f << buf;
		}
		std::string buf;
		for (auto const &num : total.record_buf) {
			buf += std::to_string(num) + ",";
		}
		if (buf.size() > 0)
			buf.pop_back();
		buf += '\n';
		f << buf;

		f.close();
		std::cout << "Info saved to " << fname << std::endl;
	}

}

// Callback
///////////
void AnalysisWindow::button_clear_all_Clicked() {
	g_analysisWindow->m_analysis->ClearAll();
}

void AnalysisWindow::button_record_Clicked() {
	if (!g_analysisWindow->m_analysis->m_record) {
		g_analysisWindow->m_analysis->m_record = true;
		g_analysisWindow->button_record->SetColor(sf::Color::Red);		
	}
	else {
		g_analysisWindow->m_analysis->m_record = false;
		g_analysisWindow->button_record->ResetColor();
	}
}

void AnalysisWindow::button_save_Clicked() {
	g_analysisWindow->m_analysis->SaveRecord("info.txt");
}
///////////


void AnalysisWindow::NewData(uint32_t* data, int size) {
	m_analysis->Add(data, size);

	for (int i = 0; i < N_CHANNELS; ++i) {		
		labels[i].label_min->SetText(std::to_string(m_analysis->channel[i].min));
		labels[i].label_max->SetText(std::to_string(m_analysis->channel[i].max));
		labels[i].label_avg->SetText(std::to_string(m_analysis->channel[i].avg));
		labels[i].label_std_dev->SetText(std::to_string(m_analysis->channel[i].std_dev));
		labels[i].label_last->SetText(std::to_string(m_analysis->channel[i].last));
		labels[i].label_cnt->SetText(std::to_string(m_analysis->channel[i].cnt));		
	}	
	label_all.label_min->SetText(std::to_string(m_analysis->total.min));
	label_all.label_max->SetText(std::to_string(m_analysis->total.max));
	label_all.label_avg->SetText(std::to_string(m_analysis->total.avg));
	label_all.label_std_dev->SetText(std::to_string(m_analysis->total.std_dev));
	// Last value for all tracks is undefined
	label_all.label_cnt->SetText(std::to_string(m_analysis->total.cnt));
}

AnalysisWindow::AnalysisWindow(char const *title) 
	: Window(450, 360, title, sf::Style::None | sf::Style::Close) {
	m_analysis = new SortingAnalysis;
	
	button_clear_all = new mygui::Button(60, 10, "Clear", 80, 25);
	button_clear_all->OnClick(&AnalysisWindow::button_clear_all_Clicked);
	Add(button_clear_all);
	button_record = new mygui::Button(180, 10, "Record", 80, 25);
	button_record->OnClick(&AnalysisWindow::button_record_Clicked);
	Add(button_record);
	button_save = new mygui::Button(300, 10, "Save", 80, 25);
	button_save->OnClick(&AnalysisWindow::button_save_Clicked);
	Add(button_save);

	label_info_win_to_det_min = new mygui::Label(80, 50, "Min");
	Add(label_info_win_to_det_min);
	label_info_win_to_det_max = new mygui::Label(140, 50, "Max:");
	Add(label_info_win_to_det_max);
	label_info_win_to_det_avg = new mygui::Label(200, 50, "Avg:");
	Add(label_info_win_to_det_avg);
	label_info_win_to_det_std_dev = new mygui::Label(260, 50, "Stdev:");
	Add(label_info_win_to_det_std_dev);
	label_info_win_to_det_last = new mygui::Label(320, 50, "Last:");
	Add(label_info_win_to_det_last);
	label_info_win_to_det_cnt = new mygui::Label(380, 50, "N: ");
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
		labels[i].label_std_dev = new mygui::Label(260, 80 + i * 30, ("N/A"));
		Add(labels[i].label_std_dev);
		labels[i].label_last = new mygui::Label(320, 80 + i * 30, ("N/A"));
		Add(labels[i].label_last);
		labels[i].label_cnt = new mygui::Label(380, 80 + i * 30, ("N/A"));
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
	label_all.label_std_dev = new mygui::Label(260, 80 + 8 * 30, ("N/A"));
	Add(label_all.label_std_dev);
	label_all.label_last = new mygui::Label(320, 80 + 8 * 30, ("N/A"));
	Add(label_all.label_last);
	label_all.label_cnt = new mygui::Label(380, 80 + 8 * 30, ("N/A"));
	Add(label_all.label_cnt);
}


AnalysisWindow::~AnalysisWindow() {
	delete m_analysis;
	delete button_clear_all;
	delete label_info_win_to_det_min;
	delete label_info_win_to_det_max;
	delete label_info_win_to_det_avg;
	delete label_info_win_to_det_std_dev;
	delete label_info_win_to_det_last;
	delete label_info_win_to_det_cnt;
}

AnalysisWindow::InfoLabel::~InfoLabel() {
	delete channel_number;
	delete label_min;
	delete label_max;
	delete label_avg;
	delete label_std_dev;
	delete label_last;
	delete label_cnt;
}