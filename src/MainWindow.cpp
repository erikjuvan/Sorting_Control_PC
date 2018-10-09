#include "MainWindow.hpp"
#include "AnalysisWindow.hpp"
#include "Communication.hpp"
#include "Helpers.hpp"
#include "Application.hpp"

extern Communication	*g_communication;
extern MainWindow		*g_mainWindow;
extern AnalysisWindow	*g_analysisWindow;

extern Running		g_running;
extern Record		g_record;
extern View			g_view;
extern TriggerFrame g_triggerframe;

extern int g_n_samples;

static int chart_frame_idx = -1;	// -1 so that when we first press right arrow we get the first [0] frame

void MainWindow::button_connect_Click() {
	if (!g_communication->IsConnected()) {
		if (g_communication->Connect(g_mainWindow->textbox_comport->GetText())) {

			g_mainWindow->button_connect->SetText("Disconnect");

			std::string buf;
			std::vector<std::string> strings;

			auto& read_and_parse = [&buf, &strings](std::string const& str) {
				g_communication->Write(str);
				buf = g_communication->Readline();
				strings = Help::TokenizeString(buf);
				if (buf[buf.size() - 1] == '\n') buf.pop_back();
			};

			read_and_parse("GETFREQ\n");
			g_mainWindow->textbox_frequency->SetText(buf);

			read_and_parse("GETPARAMS\n");
			for (auto& s : g_mainWindow->signals) {
				s.SetThreashold(std::stof(strings[3]));
			}
			g_mainWindow->textbox_filter_params->SetText(buf);

			read_and_parse("GETTIMES\n");
			for (auto& s : g_mainWindow->signals) {
				s.SetBlindTime(std::stof(strings[2]));
			}
			g_mainWindow->textbox_times->SetText(buf);

			read_and_parse("GETTRGFRM\n");
			if (TriggerFrame::ON == static_cast<TriggerFrame>(std::stoi(buf))) {
				g_mainWindow->chart->EnableTriggerFrame();
				g_mainWindow->button_trigger_frame->SetText("Frame ON");
			}
			else {
				g_mainWindow->chart->DisableTriggerFrame();
				g_mainWindow->button_trigger_frame->SetText("Frame OFF");
			}				

			read_and_parse("GETVIEW\n");
			if (View::FILTERED == static_cast<View>(std::stoi(buf))) {
				g_mainWindow->button_view_mode->SetText("Filtered");
				g_view = View::FILTERED;
			}				
			else if (View::RAW == static_cast<View>(std::stoi(buf))) {
				g_mainWindow->button_view_mode->SetText("Raw");
				g_view = View::RAW;
			}				
			else if (View::TRAINED == static_cast<View>(std::stoi(buf))) {
				g_mainWindow->button_view_mode->SetText("Trained");
				g_view = View::TRAINED;
			}				
		}
		else {
			std::cout << "Can't connect to port " << g_mainWindow->textbox_comport->GetText() << std::endl;
		}
	}
	else {
		if (g_running == Running::RUNNING)
			button_run_Click();
		g_communication->Disconnect();
		g_mainWindow->button_connect->SetText("Connect");
	}
}

void MainWindow::button_run_Click() {
	if (!g_communication->IsConnected())
		return;

	if (g_running == Running::STOPPED) {
		// Send data first before setting g_running = Running::RUNNING;
		g_communication->Write("UART_SORT\n");
		g_communication->Write("VRBS,1\n");
		g_running = Running::RUNNING;
		g_mainWindow->button_run->SetText("Running");

		for (int i = 0; i < N_CHANNELS; ++i)
			g_mainWindow->chart->ChangeSignal(i, &g_mainWindow->signals[i]);
	}
	else if (g_running == Running::RUNNING) {
		// Order of statements here matters, to insure PC app doesn't get stuck on serial->read function
		g_running = Running::STOPPED;
		g_communication->Write("VRBS,0\n");
		g_mainWindow->button_run->SetText("Stopped");
	}
}

void MainWindow::button_trigger_frame_Click() {	
	if (g_triggerframe == TriggerFrame::OFF) {
		g_triggerframe = TriggerFrame::ON;
		g_communication->Write("SETTRGFRM,1\n");
		g_mainWindow->chart->EnableTriggerFrame();
		g_mainWindow->button_trigger_frame->SetText("Frame ON");
	}
	else {
		g_triggerframe = TriggerFrame::OFF;
		g_communication->Write("GETTRGFRM,0\n");
		g_mainWindow->chart->DisableTriggerFrame();
		g_mainWindow->button_trigger_frame->SetText("Frame OFF");
	}
}

void MainWindow::button_view_mode_Click() {
	if (g_view == View::FILTERED) {
		g_view = View::RAW;
		g_communication->Write("RAW\n");
		g_mainWindow->button_view_mode->SetText("Raw");
	}
	else if (g_view == View::RAW || g_view == View::TRAINED) { // If somehow we end up in TRAINED view, go back to filtered
		g_view = View::FILTERED;
		g_communication->Write("FILTERED\n");
		g_mainWindow->button_view_mode->SetText("Filtered");
	}
}

void MainWindow::button_set_frequency_Click() {
	g_communication->Write("SETFREQ," + g_mainWindow->textbox_frequency->GetText() + "\n");
}

void MainWindow::button_set_filter_params_Click() {
	g_communication->Write("SETPARAMS," + g_mainWindow->textbox_filter_params->GetText() + "\n");

	// Set threashold for all signals
	std::vector<std::string> strings = Help::TokenizeString(g_mainWindow->textbox_filter_params->GetText());
	for (auto& s : g_mainWindow->signals) {
		s.SetThreashold(std::stof(strings[3]));
	}
}

void MainWindow::button_set_times_Click() {
	g_communication->Write("SETTIMES," + g_mainWindow->textbox_times->GetText() + "\n");

	// Set blind time for all signals
	std::vector<std::string> strings = Help::TokenizeString(g_mainWindow->textbox_times->GetText());
	for (auto& s : g_mainWindow->signals) {
		s.SetBlindTime(std::stoi(strings[2]));
	}
}

void MainWindow::button_record_Click() {
	auto& ResetSignals = []() {for (int i = 0; i < N_CHANNELS; ++i)
		g_mainWindow->chart->ChangeSignal(i, &g_mainWindow->signals[i]); };

	chart_frame_idx = -1;

	if (g_record == Record::NO) {
		g_record = Record::ALL;
		g_mainWindow->button_record->SetColor(sf::Color::Red);
		g_mainWindow->recorded_signals.clear();
		ResetSignals();
		g_mainWindow->label_recorded_signals_counter->SetText("0");
	}
	else if (g_record == Record::ALL) {
		g_record = Record::EVENTS;
		g_mainWindow->button_record->SetColor(sf::Color::Yellow);	
		g_mainWindow->recorded_signals.clear();
		ResetSignals();
		g_mainWindow->label_recorded_signals_counter->SetText("0");
	}
	else if (g_record == Record::EVENTS) {
		g_record = Record::NO;
		g_mainWindow->button_record->ResetColor();
		g_mainWindow->recorded_signals.clear();
		ResetSignals();
		g_mainWindow->label_recorded_signals_counter->SetText("0");
	}
}

void MainWindow::button_analysis_window_Click() {
	if (!g_analysisWindow->IsOpen()) {
		g_analysisWindow->Create(390, 360, "Info", sf::Style::None | sf::Style::Close);
		g_analysisWindow->SetPosition(g_mainWindow->GetPosition() + sf::Vector2i(1850 - 420, 40));
		g_analysisWindow->AlwaysOnTop(true);
		g_analysisWindow->MakeTransparent();
		g_analysisWindow->SetTransparency(120);

	}	
}

void MainWindow::label_info_detected_in_window_Clicked() {
	for (auto& s : g_mainWindow->signals) {
		s.ClearDetectionsInWindow();
	}
}

void MainWindow::label_info_detected_out_window_Clicked() {

	for (auto& s : g_mainWindow->signals) {
		s.ClearDetectionsOutWindow();
	}
}

void MainWindow::label_info_signal_missed_Clicked() {
	for (auto& s : g_mainWindow->signals) {
		s.ClearMissed();
	}
}

static auto TmpSetEvent = [](bool on, Signal::Event e) {
	Signal::Event ev = Signal::EventsToRecord();
	if (on) ev |= e;
	else ev &= ~e;
	Signal::EventsToRecord(ev);
};

void MainWindow::checkbox_detected_in_Clicked() {
	TmpSetEvent(g_mainWindow->checkbox_detected_in->Checked() , Signal::Event::DETECTED_IN);
}

void MainWindow::checkbox_detected_out_Clicked() {
	TmpSetEvent(g_mainWindow->checkbox_detected_out->Checked(), Signal::Event::DETECTED_OUT);
}

void MainWindow::checkbox_missed_Clicked() {
	TmpSetEvent(g_mainWindow->checkbox_missed->Checked(), Signal::Event::MISSED);
}

void MainWindow::checkbox_only_show_framed_Clicked() {
	for (auto& s : g_mainWindow->signals) {
		s.OnlyDrawOnTrigger(g_mainWindow->checkbox_only_show_framed->Checked());
	}
}

void MainWindow::checkbox_transparent_Clicked() {
	static bool transparent = false;
	if (!transparent) {
		transparent = true;
		g_mainWindow->MakeTransparent();
		g_mainWindow->SetTransparency(120);
	}
	else {
		transparent = false;
		g_mainWindow->SetTransparency(255);
	}
}

void MainWindow::chart_OnKeyPress(const sf::Event& event) {	
	if ((g_record == Record::ALL || g_record == Record::EVENTS) 
		&& g_running == Running::STOPPED) {
		const int size = ((int)g_mainWindow->recorded_signals.size() / N_CHANNELS);	// conversion from size_t to int (const int size) must be made or the bottom evaluation is wrong since comparing signed to unsigned, compiler promotes signed to unsigned converting -1 to maximum int value		

		if (event.key.code == sf::Keyboard::Left) {
			if (chart_frame_idx > 0 && chart_frame_idx < size) {
				chart_frame_idx--;
				for (int i = 0; i < N_CHANNELS; ++i)
					g_mainWindow->chart->ChangeSignal(i, &g_mainWindow->recorded_signals[chart_frame_idx * N_CHANNELS + i]);
			}
		}
		if (event.key.code == sf::Keyboard::Right) {			
			if (chart_frame_idx < (size - 1)) {
				chart_frame_idx++;
				for (int i = 0; i < N_CHANNELS; ++i)
					g_mainWindow->chart->ChangeSignal(i, &g_mainWindow->recorded_signals[chart_frame_idx * N_CHANNELS + i]);
			}
		}
	}

	switch (event.key.code) {
	case sf::Keyboard::Num0:
		g_mainWindow->chart->ToggleDrawAllSignals();
		break;
	case sf::Keyboard::Num1:
		g_mainWindow->chart->ToggleDrawSignal(1);
		break;
	case sf::Keyboard::Num2:
		g_mainWindow->chart->ToggleDrawSignal(2);
		break;
	case sf::Keyboard::Num3:
		g_mainWindow->chart->ToggleDrawSignal(3);
		break;
	case sf::Keyboard::Num4:
		g_mainWindow->chart->ToggleDrawSignal(4);
		break;
	case sf::Keyboard::Num5:
		g_mainWindow->chart->ToggleDrawSignal(5);
		break;
	case sf::Keyboard::Num6:
		g_mainWindow->chart->ToggleDrawSignal(6);
		break;
	case sf::Keyboard::Num7:
		g_mainWindow->chart->ToggleDrawSignal(7);
		break;
	case sf::Keyboard::Num8:
		g_mainWindow->chart->ToggleDrawSignal(8);
		break;
	}
}

void MainWindow::CreateChart(int samples) {
	g_n_samples = samples;
	chart = new Chart(240, 10, 1600, 880, g_n_samples, 100);
	chart->CreateGrid(9);
	chart->OnKeyPress(&MainWindow::chart_OnKeyPress);
	signals.clear();
	signals.reserve(N_CHANNELS);
	recorded_signals.reserve(N_CHANNELS * 10); // make an arbitrary reservation, just so there aren't so many reallocations when first recording
	for (int i = 0; i < N_CHANNELS; ++i) {
		signals.push_back(Signal(g_n_samples, sf::Color(m_Colors[i]), chart->GraphRegion(), chart->MaxVal()));
		chart->AddSignal(&signals[signals.size() - 1]);
	}
	Signal::EventsToRecord(Signal::Event::MISSED | Signal::Event::DETECTED_OUT);
}

void MainWindow::RunClick() {
	button_run_Click();
}

MainWindow::MainWindow(int w, int h, const char* title, sf::Uint32 style) 
	: Window(w, h, title, style) {
	///////////
	// Chart //
	///////////
	constexpr int N_SAMPLES = 10000;
	CreateChart(N_SAMPLES);

	/////////////
	// Buttons //
	/////////////
	button_connect = new mygui::Button(10, 50, "Connect", 100);
	button_connect->OnClick(&MainWindow::button_connect_Click);

	button_run = new mygui::Button(10, 90, "Stopped", 100);
	button_run->OnClick(&MainWindow::button_run_Click);

	button_trigger_frame = new mygui::Button(125, 50, "Frame OFF", 100, 30, 18);
	button_trigger_frame->OnClick(&MainWindow::button_trigger_frame_Click);

	button_view_mode = new mygui::Button(125, 90, "Raw", 100, 30, 18);
	button_view_mode->OnClick(&MainWindow::button_view_mode_Click);

	button_set_frequency = new mygui::Button(10, 260, "Send");
	button_set_frequency->OnClick(&MainWindow::button_set_frequency_Click);

	button_set_filter_params = new mygui::Button(10, 380, "Send");
	button_set_filter_params->OnClick(&MainWindow::button_set_filter_params_Click);

	button_set_times = new mygui::Button(10, 500, "Send");
	button_set_times->OnClick(&MainWindow::button_set_times_Click);

	button_record = new mygui::Button(10, 650, "Record");
	button_record->OnClick(&MainWindow::button_record_Click);

	button_analysis_window = new mygui::Button(10, 800, "Info");
	button_analysis_window->OnClick(&MainWindow::button_analysis_window_Click);

	//////////////
	// Texboxes //
	//////////////
	textbox_comport = new mygui::Textbox(10, 10, "COM", 80);
	textbox_frequency = new mygui::Textbox(10, 220, "", 80);
	textbox_filter_params = new mygui::Textbox(10, 340, "", 170);
	textbox_times = new mygui::Textbox(10, 460, "", 140);

	////////////
	// Labels //
	////////////			
	label_frequency = new mygui::Label(10, 190, "Sample frequency:");
	label_filter_params = new mygui::Label(10, 310, "Filter params(a1,a2,a3,thr):");
	label_times = new mygui::Label(10, 430, "Times (dly, dur, blind):");
	label_recorded_signals_counter = new mygui::Label(120, 654, "0");
	label_info_rx_bytes = new mygui::Label(10, 590, "Rx buf: 0 bytes");
	label_info_detected_in_window = new mygui::Label(120, 698, "0");
	label_info_detected_in_window->OnClick(&MainWindow::label_info_detected_in_window_Clicked);
	label_info_detected_out_window = new mygui::Label(120, 729, "0");
	label_info_detected_out_window->OnClick(&MainWindow::label_info_detected_out_window_Clicked);
	label_info_signal_missed = new mygui::Label(120, 760, "0");
	label_info_signal_missed->OnClick(&MainWindow::label_info_signal_missed_Clicked);


	////////////////
	// Checkboxes //
	////////////////
	checkbox_detected_in = new mygui::Checkbox(10, 700, "Det IN: ");
	checkbox_detected_in->OnClick(&MainWindow::checkbox_detected_in_Clicked);	
	checkbox_detected_in->Checked(false);

	checkbox_detected_out = new mygui::Checkbox(10, 730, "Det OUT: ");
	checkbox_detected_out->OnClick(&MainWindow::checkbox_detected_out_Clicked);
	checkbox_detected_out->Checked(true);

	checkbox_missed = new mygui::Checkbox(10, 760, "Missed: ");
	checkbox_missed->OnClick(&MainWindow::checkbox_missed_Clicked);
	checkbox_missed->Checked(true);

	checkbox_only_show_framed = new mygui::Checkbox(10, 550, "Only show framed");
	checkbox_only_show_framed->OnClick(&MainWindow::checkbox_only_show_framed_Clicked);

	checkbox_transparent = new mygui::Checkbox(10, 850, "Transparent");
	checkbox_transparent->OnClick(&MainWindow::checkbox_transparent_Clicked);


	/////////////////
	// Main window //
	/////////////////
	Add(chart);
	// Buttons
	Add(button_connect);
	Add(button_run);
	Add(button_trigger_frame);
	Add(button_set_frequency);
	Add(button_set_filter_params);
	Add(button_set_times);	
	Add(button_view_mode);
	Add(button_record);
	Add(button_analysis_window);
	// Texboxes
	Add(textbox_comport);
	Add(textbox_frequency);
	Add(textbox_filter_params);
	Add(textbox_times);
	// Labels		
	Add(label_frequency);
	Add(label_filter_params);
	Add(label_times);
	Add(label_recorded_signals_counter);
	Add(label_info_rx_bytes);
	Add(label_info_detected_in_window);
	Add(label_info_detected_out_window);
	Add(label_info_signal_missed);
	// Checkboxes
	Add(checkbox_detected_in);
	Add(checkbox_detected_out);
	Add(checkbox_missed);
	Add(checkbox_only_show_framed);
	Add(checkbox_transparent);
}

MainWindow::~MainWindow() {	
	if (g_running == Running::RUNNING)
		button_run_Click();
}