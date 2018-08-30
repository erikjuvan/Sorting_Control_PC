#include "AnalysisWindow.hpp"


void SortingAnalysis::Add(uint16_t* data, int size) {
	for (int i = 0; i < size; ++i) {
		uint8_t val = data[i] & 0xFF;
		if (val > 0) {
			int ch = ((data[i] >> 8) & 0xFF);
			channel[ch].last = val;
			channel[ch].sum += val;
			channel[ch].cnt++;
			if (channel[ch].min > val) channel[ch].min = val;
			if (channel[ch].max < val) channel[ch].max = val;
		}		
	}
	for (int i = 0; i < Application::N_CHANNELS; ++i) {		 
		channel[i].avg = channel[i].sum / channel[i].cnt;
	}
}

void AnalysisWindow::button_clear_all_Clicked(void* vobj) {
	AnalysisWindow& obj = *static_cast<AnalysisWindow*>(vobj);
	for (auto& l : obj.labels) {		
		l.label_min->SetText("N/A");
		l.label_max->SetText("N/A");
		l.label_avg->SetText("N/A");
		l.label_last->SetText("N/A");
	}
}

void AnalysisWindow::Show() {
	m_window->Show();
}

void AnalysisWindow::Hide() {
	m_window->Hide();
}

bool AnalysisWindow::IsOpen() {
	return m_window->IsOpen();
}

void AnalysisWindow::Update(uint16_t* data, int size) {
	m_analysis->Add(data, size);

	for (int i = 0; i < Application::N_CHANNELS; ++i) {		
		labels[i].label_min->SetText(std::to_string(m_analysis->channel[i].min));
		labels[i].label_max->SetText(std::to_string(m_analysis->channel[i].max));
		labels[i].label_avg->SetText(std::to_string(m_analysis->channel[i].avg));
		labels[i].label_last->SetText(std::to_string(m_analysis->channel[i].last));
				
	}
}

AnalysisWindow::AnalysisWindow(int w, int h, const char* title, Application* application) : m_application(application) {
	m_window = new Window(w, h, title);
	m_analysis = new SortingAnalysis;
	
	button_clear_all = new gui::Button(220, 20, "Clear");
	button_clear_all->OnClick(this, &AnalysisWindow::button_clear_all_Clicked);

	label_info_win_to_det_min = new gui::Label(100, 70, "Min");
	label_info_win_to_det_max = new gui::Label(200, 70, "Max:");
	label_info_win_to_det_avg = new gui::Label(300, 70, "Avg:");
	label_info_win_to_det_last = new gui::Label(400, 70, "Last:");

	for (int i = 0; i < Application::N_CHANNELS; ++i) {
		labels[i].channel_number = new gui::Label(20, 120 + i * 40, ("Ch " + std::to_string(i+1)).c_str());
		m_window->Attach(labels[i].channel_number);
		labels[i].label_min = new gui::Label(100, 120 + i * 40, ("N/A"));
		m_window->Attach(labels[i].label_min);
		labels[i].label_max = new gui::Label(200, 120 + i * 40, ("N/A"));
		m_window->Attach(labels[i].label_max);
		labels[i].label_avg = new gui::Label(300, 120 + i * 40, ("N/A"));
		m_window->Attach(labels[i].label_avg);
		labels[i].label_last = new gui::Label(400, 120 + i * 40, ("N/A"));
		m_window->Attach(labels[i].label_last);		
	}
	
	m_window->Attach(button_clear_all);
	m_window->Attach(label_info_win_to_det_min);
	m_window->Attach(label_info_win_to_det_max);
	m_window->Attach(label_info_win_to_det_avg);
	m_window->Attach(label_info_win_to_det_last);
}


AnalysisWindow::~AnalysisWindow() {
	delete m_window;
	delete label_info_win_to_det_min;
	delete label_info_win_to_det_max;
	delete label_info_win_to_det_avg;
	delete label_info_win_to_det_last;
}

