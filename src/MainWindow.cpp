#include "MainWindow.hpp"
#include "Application.hpp"
#include "Communication.hpp"
#include "Helpers.hpp"
#include "InfoWindow.hpp"
#include <functional>
#include <thread>

extern Communication* g_communication;
extern InfoWindow*    g_detectionInfoWindow;
extern InfoWindow*    g_frameInfoWindow;

extern Running      g_running;
extern Record       g_record;
extern View         g_view;
extern TriggerFrame g_triggerframe;

extern int g_n_samples;

static int chart_frame_idx = -1; // -1 so that when we first press right arrow we get the first [0] frame

void MainWindow::button_connect_Click()
{
    if (!g_communication->IsConnected()) {
        if (g_communication->Connect(textbox_comport->GetText())) {

            button_connect->SetText("Disconnect");

            std::string              buf;
            std::vector<std::string> strings;

            auto const& read_and_parse = [&buf, &strings](std::string const& str) {
                g_communication->Write(str);
                buf     = g_communication->Readline();
                strings = Help::TokenizeString(buf);
                if (buf[buf.size() - 1] == '\n')
                    buf.pop_back();
            };

            read_and_parse("GETFREQ\n");
            textbox_frequency->SetText(buf);

            read_and_parse("GETPARAMS\n");
            for (auto& s : signals) {
                s.SetThreashold(std::stof(strings[3]));
            }
            textbox_filter_params->SetText(buf);

            read_and_parse("GETTIMES\n");
            for (auto& s : signals) {
                s.SetBlindTime(std::stoi(strings[2]));
            }
            textbox_times->SetText(buf);

            read_and_parse("GETTRGFRM\n");
            if (TriggerFrame::ON == static_cast<TriggerFrame>(std::stoi(buf))) {
                chart->EnableTriggerFrame();
                button_trigger_frame->SetText("Frame ON");
            } else {
                chart->DisableTriggerFrame();
                button_trigger_frame->SetText("Frame OFF");
            }

            read_and_parse("GETVIEW\n");
            if (View::FILTERED == static_cast<View>(std::stoi(buf))) {
                button_view_mode->SetText("Filtered");
                g_view = View::FILTERED;
            } else if (View::RAW == static_cast<View>(std::stoi(buf))) {
                button_view_mode->SetText("Raw");
                g_view = View::RAW;
            } else if (View::TRAINED == static_cast<View>(std::stoi(buf))) {
                button_view_mode->SetText("Trained");
                g_view = View::TRAINED;
            }
            textbox_comport->Enabled(false);
        } else {
            std::cout << "Can't connect to port " << textbox_comport->GetText() << std::endl;
        }
    } else {
        if (g_running == Running::RUNNING)
            button_run_Click();
        g_communication->Disconnect();
        button_connect->SetText("Connect");
        textbox_comport->Enabled(true);
    }
}

void MainWindow::button_run_Click()
{
    if (!g_communication->IsConnected())
        return;

    if (g_running == Running::STOPPED) {
        // Send data first before setting g_running = Running::RUNNING;
        g_communication->Write("UART_SORT\n");
        g_communication->Write("VRBS,1\n");
        g_running = Running::RUNNING;
        button_run->SetText("Running");

        for (int i = 0; i < N_CHANNELS; ++i)
            chart->ChangeSignal(i, &signals[i]);
    } else if (g_running == Running::RUNNING) {
        // Order of statements here matters, to insure PC app doesn't get stuck on serial->read function
        g_running = Running::STOPPED;
        g_communication->Write("VRBS,0\n");
        size_t len = 0;
        while ((len = g_communication->GetRxBufferLen()) > 0) {
            g_communication->Purge();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        button_run->SetText("Stopped");
    }
}

void MainWindow::button_trigger_frame_Click()
{
    if (g_triggerframe == TriggerFrame::OFF) {
        g_triggerframe = TriggerFrame::ON;
        g_communication->Write("SETTRGFRM,1\n");
        chart->EnableTriggerFrame();
        button_trigger_frame->SetText("Frame ON");
    } else {
        g_triggerframe = TriggerFrame::OFF;
        g_communication->Write("SETTRGFRM,0\n");
        chart->DisableTriggerFrame();
        button_trigger_frame->SetText("Frame OFF");
    }
}

void MainWindow::button_view_mode_Click()
{
    if (g_view == View::FILTERED) {
        g_view = View::RAW;
        g_communication->Write("RAW\n");
        button_view_mode->SetText("Raw");
    } else if (g_view == View::RAW || g_view == View::TRAINED) { // If somehow we end up in TRAINED view, go back to filtered
        g_view = View::FILTERED;
        g_communication->Write("FILTERED\n");
        button_view_mode->SetText("Filtered");
    }
}

void MainWindow::button_set_frequency_Click()
{
    g_communication->Write("SETFREQ," + textbox_frequency->GetText() + "\n");
}

void MainWindow::button_set_filter_params_Click()
{
    g_communication->Write("SETPARAMS," + textbox_filter_params->GetText() + "\n");

    // Set threashold for all signals
    std::vector<std::string> strings = Help::TokenizeString(textbox_filter_params->GetText());
    for (auto& s : signals) {
        s.SetThreashold(std::stof(strings[3]));
    }
}

void MainWindow::button_set_times_Click()
{
    g_communication->Write("SETTIMES," + textbox_times->GetText() + "\n");

    // Set blind time for all signals
    std::vector<std::string> strings = Help::TokenizeString(textbox_times->GetText());
    for (auto& s : signals) {
        s.SetBlindTime(std::stoi(strings[2]));
    }
}

void MainWindow::button_record_Click()
{
    auto const& ResetSignals = [this]() {
        for (int i = 0; i < N_CHANNELS; ++i)
            chart->ChangeSignal(i, &signals[i]);
        chart_frame_idx = -1;
    };

    if (g_record == Record::NO) {
        g_record = Record::ALL;
        button_record->SetColor(sf::Color::Red);
        recorded_signals.clear();
        ResetSignals();
        label_recorded_signals_counter->SetText("0");
    } else if (g_record == Record::ALL) {
        g_record = Record::EVENTS;
        button_record->SetColor(sf::Color::Yellow);
        recorded_signals.clear();
        ResetSignals();
        label_recorded_signals_counter->SetText("0");
    } else if (g_record == Record::EVENTS) {
        g_record = Record::NO;
        button_record->ResetColor();
        recorded_signals.clear();
        ResetSignals();
        label_recorded_signals_counter->SetText("0");
    }
}

void MainWindow::button_info_Click()
{
    if (!g_detectionInfoWindow->IsOpen()) {
        delete g_detectionInfoWindow;
        g_detectionInfoWindow = new InfoWindow("Detection Info", "info_det.txt");
        g_detectionInfoWindow->SetPosition(GetPosition() + sf::Vector2i(1850 - 480, 40));
        for (auto& s : signals) {
            g_detectionInfoWindow->push_back(&s.GetDetecionStats());
        }
        g_detectionInfoWindow->SetAll(Signal::GetDetecionStatsAll());
    }
    if (!g_frameInfoWindow->IsOpen()) {
        delete g_frameInfoWindow;
        g_frameInfoWindow = new InfoWindow("Frame Info", "info_frm.txt");
        g_frameInfoWindow->SetPosition(GetPosition() + sf::Vector2i(1850 - 1000, 40));
        for (auto& s : signals) {
            g_frameInfoWindow->push_back(&s.GetTriggerWindowStats());
        }
        g_frameInfoWindow->SetAll(Signal::GetTriggerWindowStatsAll());
    }
}

void MainWindow::button_clear_all_Click()
{
    auto const& ResetSignals = [this]() {
        for (int i = 0; i < N_CHANNELS; ++i)
            chart->ChangeSignal(i, &signals[i]);
        chart_frame_idx = -1;
    };

    g_frameInfoWindow->Clear();
    g_detectionInfoWindow->Clear();
    label_info_detected_in_window_Clicked();
    label_info_detected_out_window_Clicked();
    label_info_signal_missed_Clicked();

    recorded_signals.clear();
    ResetSignals();
    label_recorded_signals_counter->SetText("0");
}

void MainWindow::textbox_detection_time_min_KeyPress()
{
    try {
        int val = std::stoi(textbox_detection_time_min->GetText());
        Signal::DetectionTimeMin(val * 10); // * 10 because signal ticks are in 100us
    } catch (std::invalid_argument) {
        Signal::DetectionTimeMin(0);
    }
}

void MainWindow::textbox_detection_time_max_KeyPress()
{
    try {
        int val = std::stoi(textbox_detection_time_max->GetText());
        Signal::DetectionTimeMax(val * 10); // * 10 because signal ticks are in 100us
    } catch (std::invalid_argument) {
        Signal::DetectionTimeMax(1000000 * 10); // arbitrarily long number
    }
}

void MainWindow::textbox_window_time_min_KeyPress()
{
    try {
        int val = std::stoi(textbox_window_time_min->GetText());
        Signal::WindowTimeMin(val * 10); // * 10 because signal ticks are in 100us
    } catch (std::invalid_argument) {
        Signal::WindowTimeMin(0);
    }
}

void MainWindow::textbox_window_time_max_KeyPress()
{
    try {
        int val = std::stoi(textbox_window_time_max->GetText());
        Signal::WindowTimeMax(val * 10); // * 10 because signal ticks are in 100us
    } catch (std::invalid_argument) {
        Signal::WindowTimeMax(1000000 * 10); // arbitrarily long number
    }
}

void MainWindow::label_info_detected_in_window_Clicked()
{
    for (auto& s : signals) {
        s.ClearDetectionsInWindow();
    }
}

void MainWindow::label_info_detected_out_window_Clicked()
{
    for (auto& s : signals) {
        s.ClearDetectionsOutWindow();
    }
}

void MainWindow::label_info_signal_missed_Clicked()
{
    for (auto& s : signals) {
        s.ClearMissed();
    }
}

void MainWindow::label_detection_time_Clicked()
{
    // not yet implemented
}

void MainWindow::label_window_time_Clicked()
{
    // not yet implemented
}

void MainWindow::checkbox_transparent_Clicked()
{
    static bool transparent = false;
    if (!transparent) {
        transparent = true;
        MakeTransparent();
        SetTransparency(180);
    } else {
        transparent = false;
        SetTransparency(255);
    }
}

void MainWindow::checkbox_only_show_framed_Clicked()
{
    for (auto& s : signals) {
        s.OnlyDrawOnTrigger(checkbox_only_show_framed->Checked());
    }
}
void MainWindow::checkbox_show_event_indicator_Clicked()
{
    for (auto& s : signals) {
        s.ShowEventIndicator(checkbox_show_event_indicator->Checked());
    }

    for (auto& rs : recorded_signals) {
        rs.ShowEventIndicator(checkbox_show_event_indicator->Checked());
    }
}

static auto lambda_SetEvent = [](bool on, Signal::Event e) {
    Signal::Event ev = Signal::EventsToRecord();
    if (on)
        ev = static_cast<Signal::Event>(ev | e);
    else
        ev = static_cast<Signal::Event>(ev & ~e);
    Signal::EventsToRecord(ev);
};

void MainWindow::checkbox_detected_in_Clicked()
{
    lambda_SetEvent(checkbox_detected_in->Checked(), Signal::Event::DETECTED_IN);
}

void MainWindow::checkbox_detected_out_Clicked()
{
    lambda_SetEvent(checkbox_detected_out->Checked(), Signal::Event::DETECTED_OUT);
}

void MainWindow::checkbox_missed_Clicked()
{
    lambda_SetEvent(checkbox_missed->Checked(), Signal::Event::MISSED);
}

void MainWindow::checkbox_detection_time_Clicked()
{
    lambda_SetEvent(checkbox_detection_time->Checked(), Signal::Event::DETECTION_TIME);
}

void MainWindow::checkbox_window_time_Clicked()
{
    lambda_SetEvent(checkbox_window_time->Checked(), Signal::Event::WINDOW_TIME);
}

void MainWindow::chart_OnKeyPress(const sf::Event& event)
{
    if ((g_record == Record::ALL || g_record == Record::EVENTS) && g_running == Running::STOPPED) {
        const int size = ((int)recorded_signals.size() / N_CHANNELS); // conversion from size_t to int (const int size) must be made or the bottom evaluation is wrong since comparing signed to unsigned, compiler promotes signed to unsigned converting -1 to maximum int value

        if (event.key.code == sf::Keyboard::Left) {
            if (chart_frame_idx > 0 && chart_frame_idx < size) {
                chart_frame_idx--;
                for (int i = 0; i < N_CHANNELS; ++i)
                    chart->ChangeSignal(i, &recorded_signals[chart_frame_idx * N_CHANNELS + i]);
            }
        }
        if (event.key.code == sf::Keyboard::Right) {
            if (chart_frame_idx < (size - 1)) {
                chart_frame_idx++;
                for (int i = 0; i < N_CHANNELS; ++i)
                    chart->ChangeSignal(i, &recorded_signals[chart_frame_idx * N_CHANNELS + i]);
            }
        }
    }

    switch (event.key.code) {
    case sf::Keyboard::Num0:
        chart->ToggleDrawAllSignals();
        break;
    case sf::Keyboard::Num1:
        chart->ToggleDrawSignal(1);
        break;
    case sf::Keyboard::Num2:
        chart->ToggleDrawSignal(2);
        break;
    case sf::Keyboard::Num3:
        chart->ToggleDrawSignal(3);
        break;
    case sf::Keyboard::Num4:
        chart->ToggleDrawSignal(4);
        break;
    case sf::Keyboard::Num5:
        chart->ToggleDrawSignal(5);
        break;
    case sf::Keyboard::Num6:
        chart->ToggleDrawSignal(6);
        break;
    case sf::Keyboard::Num7:
        chart->ToggleDrawSignal(7);
        break;
    case sf::Keyboard::Num8:
        chart->ToggleDrawSignal(8);
        break;
    }
}

void MainWindow::CreateChart(int samples)
{
    g_n_samples = samples;
    chart       = new Chart(240, 10, 1600, 880, g_n_samples, 100);
    chart->CreateGrid(9);
    chart->OnKeyPress(std::bind(&MainWindow::chart_OnKeyPress, this, std::placeholders::_1));
    signals.clear();
    signals.reserve(N_CHANNELS);
    recorded_signals.reserve(N_CHANNELS * 10); // make an arbitrary reservation, just so there aren't so many reallocations when first recording
    for (int i = 0; i < N_CHANNELS; ++i) {
        signals.push_back(Signal(g_n_samples, sf::Color(m_Colors[i]), chart->GraphRegion(), &chart->MaxVal()));
        chart->AddSignal(&signals[signals.size() - 1]);
    }
    Signal::EventsToRecord(static_cast<Signal::Event>(Signal::Event::MISSED | Signal::Event::DETECTED_OUT));
}

void MainWindow::RunClick()
{
    button_run_Click();
}

MainWindow::MainWindow(int w, int h, const char* title, sf::Uint32 style) :
    Window(w, h, title, style)
{
    ///////////
    // Chart //
    ///////////
    constexpr int N_SAMPLES = 10000;
    CreateChart(N_SAMPLES);

    /////////////
    // Buttons //
    /////////////
    button_connect = new mygui::Button(10, 50, "Connect", 100);
    button_connect->OnClick(std::bind(&MainWindow::button_connect_Click, this));

    button_run = new mygui::Button(10, 90, "Stopped", 100);
    button_run->OnClick(std::bind(&MainWindow::button_run_Click, this));

    button_trigger_frame = new mygui::Button(125, 50, "Frame OFF", 100, 30, 18);
    button_trigger_frame->OnClick(std::bind(&MainWindow::button_trigger_frame_Click, this));

    button_view_mode = new mygui::Button(125, 90, "Raw", 100, 30, 18);
    button_view_mode->OnClick(std::bind(&MainWindow::button_view_mode_Click, this));

    button_info_windows = new mygui::Button(125, 130, "Info", 100, 30, 18);
    button_info_windows->OnClick(std::bind(&MainWindow::button_info_Click, this));

    button_set_frequency = new mygui::Button(10, 260, "Send");
    button_set_frequency->OnClick(std::bind(&MainWindow::button_set_frequency_Click, this));

    button_set_filter_params = new mygui::Button(10, 380, "Send");
    button_set_filter_params->OnClick(std::bind(&MainWindow::button_set_filter_params_Click, this));

    button_set_times = new mygui::Button(10, 500, "Send");
    button_set_times->OnClick(std::bind(&MainWindow::button_set_times_Click, this));

    button_record = new mygui::Button(10, 650, "Record");
    button_record->OnClick(std::bind(&MainWindow::button_record_Click, this));

    button_clear_all = new mygui::Button(10, 855, "Clear ALL");
    button_clear_all->OnClick(std::bind(&MainWindow::button_clear_all_Click, this));

    //////////////
    // Texboxes //
    //////////////
    textbox_comport            = new mygui::Textbox(10, 10, "COM", 80);
    textbox_frequency          = new mygui::Textbox(10, 220, "", 80);
    textbox_filter_params      = new mygui::Textbox(10, 340, "", 170);
    textbox_times              = new mygui::Textbox(10, 460, "", 140);
    textbox_detection_time_min = new mygui::Textbox(35, 787, "", 40, 25);
    textbox_detection_time_min->onKeyPress(std::bind(&MainWindow::textbox_detection_time_min_KeyPress, this));
    textbox_detection_time_max = new mygui::Textbox(185, 787, "", 40, 25);
    textbox_detection_time_max->onKeyPress(std::bind(&MainWindow::textbox_detection_time_max_KeyPress, this));
    textbox_window_time_min = new mygui::Textbox(35, 817, "", 40, 25);
    textbox_window_time_min->onKeyPress(std::bind(&MainWindow::textbox_window_time_min_KeyPress, this));
    textbox_window_time_max = new mygui::Textbox(185, 817, "", 40, 25);
    textbox_window_time_max->onKeyPress(std::bind(&MainWindow::textbox_window_time_max_KeyPress, this));

    ////////////
    // Labels //
    ////////////
    label_frequency                = new mygui::Label(10, 190, "Sample frequency:");
    label_filter_params            = new mygui::Label(10, 310, "Filter params(a1,a2,a3,thr):");
    label_times                    = new mygui::Label(10, 430, "Times (dly, dur, blind):");
    label_recorded_signals_counter = new mygui::Label(120, 654, "0");
    label_info_rx_bytes            = new mygui::Label(10, 590, "Rx buf: 0 bytes");
    label_info_detected_in_window  = new mygui::Label(120, 698, "0");
    label_info_detected_in_window->OnClick(std::bind(&MainWindow::label_info_detected_in_window_Clicked, this));
    label_info_detected_out_window = new mygui::Label(120, 729, "0");
    label_info_detected_out_window->OnClick(std::bind(&MainWindow::label_info_detected_out_window_Clicked, this));
    label_info_signal_missed = new mygui::Label(120, 760, "0");
    label_info_signal_missed->OnClick(std::bind(&MainWindow::label_info_signal_missed_Clicked, this));
    label_detection_time = new mygui::Label(80, 787, "> det time >");
    label_detection_time->OnClick(std::bind(&MainWindow::label_detection_time_Clicked, this));
    label_window_time = new mygui::Label(80, 817, "> win time >");
    label_window_time->OnClick(std::bind(&MainWindow::label_window_time_Clicked, this));

    ////////////////
    // Checkboxes //
    ////////////////
    checkbox_transparent = new mygui::Checkbox(125, 16, "Transparent", 15, 15, 15);
    checkbox_transparent->OnClick(std::bind(&MainWindow::checkbox_transparent_Clicked, this));

    checkbox_only_show_framed = new mygui::Checkbox(10, 550, "Only show framed");
    checkbox_only_show_framed->OnClick(std::bind(&MainWindow::checkbox_only_show_framed_Clicked, this));

    checkbox_show_event_indicator = new mygui::Checkbox(10, 620, "Show event lines");
    checkbox_show_event_indicator->OnClick(std::bind(&MainWindow::checkbox_show_event_indicator_Clicked, this));
    checkbox_show_event_indicator->Checked(true);

    checkbox_detected_in = new mygui::Checkbox(10, 700, "Det IN: ");
    checkbox_detected_in->OnClick(std::bind(&MainWindow::checkbox_detected_in_Clicked, this));
    checkbox_detected_in->Checked(false);

    checkbox_detected_out = new mygui::Checkbox(10, 730, "Det OUT: ");
    checkbox_detected_out->OnClick(std::bind(&MainWindow::checkbox_detected_out_Clicked, this));
    checkbox_detected_out->Checked(true);

    checkbox_missed = new mygui::Checkbox(10, 760, "Missed: ");
    checkbox_missed->OnClick(std::bind(&MainWindow::checkbox_missed_Clicked, this));
    checkbox_missed->Checked(true);

    checkbox_detection_time = new mygui::Checkbox(10, 790, "");
    checkbox_detection_time->OnClick(std::bind(&MainWindow::checkbox_detection_time_Clicked, this));
    checkbox_detection_time->Checked(false);

    checkbox_window_time = new mygui::Checkbox(10, 820, "");
    checkbox_window_time->OnClick(std::bind(&MainWindow::checkbox_window_time_Clicked, this));
    checkbox_window_time->Checked(false);

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
    Add(button_info_windows);
    Add(button_clear_all);

    // Texboxes
    Add(textbox_comport);
    Add(textbox_frequency);
    Add(textbox_filter_params);
    Add(textbox_times);
    Add(textbox_detection_time_min);
    Add(textbox_detection_time_max);
    Add(textbox_window_time_min);
    Add(textbox_window_time_max);

    // Labels
    Add(label_frequency);
    Add(label_filter_params);
    Add(label_times);
    Add(label_recorded_signals_counter);
    Add(label_info_rx_bytes);
    Add(label_info_detected_in_window);
    Add(label_info_detected_out_window);
    Add(label_info_signal_missed);
    Add(label_detection_time);
    Add(label_window_time);

    // Checkboxes
    Add(checkbox_transparent);
    Add(checkbox_only_show_framed);
    Add(checkbox_show_event_indicator);
    Add(checkbox_detected_in);
    Add(checkbox_detected_out);
    Add(checkbox_missed);
    Add(checkbox_detection_time);
    Add(checkbox_window_time);
}

MainWindow::~MainWindow()
{
    if (g_running == Running::RUNNING)
        button_run_Click();
}