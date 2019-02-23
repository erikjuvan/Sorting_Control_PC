#include "MainWindow.hpp"
#include "Application.hpp"
#include "Communication.hpp"
#include "Helpers.hpp"
#include "InfoWindow.hpp"
#include <fstream>
#include <functional>
#include <thread>

void MainWindow::SetSampleFreq()
{
    int freq_hz = std::stoi(textbox_frequency->GetText());
    for (auto& s : signals)
        s->SetSampleFreq(freq_hz);
}

void MainWindow::button_connect_Click()
{
    if (!m_communication->IsConnected()) {
        if (m_communication->Connect(textbox_comport->GetText())) {

            button_connect->SetText("Disconnect");

            // If we previously crashed we could still be receving m_data, so make sure we stop before configuring
            m_communication->Write("VRBS,0\n");

            std::string              buf;
            std::vector<std::string> strings;

            auto const& read_and_parse = [this, &buf, &strings](std::string const& str) {
                m_communication->Write(str);
                buf     = m_communication->Readline();
                strings = Help::TokenizeString(buf);
                if (buf[buf.size() - 1] == '\n')
                    buf.pop_back();
            };

            read_and_parse("GETFREQ\n");
            textbox_frequency->SetText(buf);
            SetSampleFreq();

            read_and_parse("GETPARAMS\n");
            if (strings.size() < 4)
                std::cerr << "Received invalid params\n";
            else
                for (auto& s : signals)
                    s->SetThreashold(std::stof(strings[3]));
            textbox_filter_params->SetText(buf);

            read_and_parse("GETTIMES\n");
            if (strings.size() < 3)
                std::cerr << "Received invalid times\n";
            else
                for (auto& s : signals)
                    s->SetBlindTime(std::stoi(strings[2]));
            textbox_times->SetText(buf);

            if (m_triggerframe == TriggerFrame::ON) {
                chart->EnableTriggerFrame();
                button_trigger_frame->SetText("Frame ON");
            } else {
                chart->DisableTriggerFrame();
                button_trigger_frame->SetText("Frame OFF");
            }

            textbox_comport->Enabled(false);
        } else {
            std::cout << "Can't connect to port " << textbox_comport->GetText() << std::endl;
        }
    } else {
        if (*m_running == Running::RUNNING)
            button_run_Click();
        m_communication->Disconnect();
        button_connect->SetText("Connect");
        textbox_comport->Enabled(true);
    }
}

void MainWindow::button_run_Click()
{
    if (!m_communication->IsConnected())
        return;

    if (*m_running == Running::STOPPED) {
        // Send m_data first before setting m_running = Running::RUNNING;
        m_communication->Write("UART_SORT\n");
        m_communication->Write("VRBS,1\n");
        *m_running = Running::RUNNING;
        button_run->SetText("Running");
        m_run_start_time = std::chrono::steady_clock::now();

        for (int i = 0; i < N_CHANNELS; ++i)
            chart->ChangeSignal(i, signals[i]);
    } else if (*m_running == Running::RUNNING) {
        // Order of statements here matters, to insure PC app doesn't get stuck on serial->read function
        m_communication->Write("VRBS,0\n");
        size_t len = 0;
        while ((len = m_communication->GetRxBufferLen()) > 0) {
            m_communication->Purge();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        *m_running = Running::STOPPED;
        button_run->SetText("Stopped");
    }
}

void MainWindow::button_save_Click()
{
    if (signals.size() <= 0) {
        std::cerr << "Error saving to file: signals.size() is 0 !\n";
        return;
    }
    if (signals[0]->GetRXData().size() <= 0) {
        std::cerr << "Error saving to file: there is no data to save!\n";
        return;
    }

    struct Header {
        uint32_t start_id = 0x43535453; // "STSC" - STream Sorting Control
        uint32_t time_since_epoch_s;
        uint32_t num_of_channels;
        uint32_t sizeof_sample;
        uint32_t num_of_samples_per_ch;

        struct {
            uint32_t sample_frequency_hz;
            uint32_t delay_ms;
            uint32_t duration_ms;
            uint32_t blind_ms;
            float    lpf1_K;
            float    hpf_K;
            float    lpf2_K;
            float    threshold;
        } sorting_parameters;
    };

    auto get_available_filename = [](std::string base_name) -> auto
    {
        std::string suffix;
        std::string extension{".bin"};
        int         cnt = 0;
        while (true) {
            std::string   fname = base_name + suffix + extension;
            std::ifstream f(fname);
            if (!f.good())
                return fname;
            suffix = "_" + std::to_string(++cnt);
        }
    };

    Header        head;
    auto          fname = get_available_filename("sc_data");
    std::ofstream write_file(fname, std::ofstream::binary);

    if (write_file.is_open()) {
        using namespace std::chrono;
        head.time_since_epoch_s    = static_cast<uint32_t>(duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
        head.num_of_channels       = N_CHANNELS;
        head.sizeof_sample         = sizeof(signals[0]->GetRXData()[0]);
        head.num_of_samples_per_ch = signals[0]->GetRXData().size();

        std::cout << "Saving data to " << fname << " ... ";

        write_file.write((const char*)&head, sizeof(Header));

        for (auto const& signal : signals) {
            auto const& vec = signal->GetRXData();
            if (vec.size() != head.num_of_samples_per_ch) {
                std::cerr << "Error saving to file: channel size mismatch!\n";
                write_file.close();
                return;
            }

            write_file.write((const char*)&vec[0], head.num_of_samples_per_ch * head.sizeof_sample);
        }

        write_file.close();
    } else {
        std::cerr << "Error: can't open file for writting!\n";
        return;
    }

    // Check file for correct header m_data
    /////////////////////////////////////
    std::ifstream read_file(fname, std::ifstream::binary);
    if (read_file.is_open()) {
        Header tmp;
        read_file.read((char*)&tmp, sizeof(Header));
        if (std::memcmp(&tmp, &head, sizeof(Header))) {
            std::cerr << "Error write failed: Incorrect header when reading back file!\n";
            read_file.close();
            return;
        }
    } else {
        std::cerr << "Error: can't open file for reading!\n";
        return;
    }

    // Check file for correct size
    //////////////////////////////
    std::ifstream           in(fname, std::ifstream::ate | std::ifstream::binary);
    std::ifstream::pos_type fsize = 0;
    if (in.is_open()) {
        fsize                                = in.tellg();
        std::ifstream::pos_type correct_size = sizeof(Header) + head.num_of_channels * head.num_of_samples_per_ch * head.sizeof_sample;
        in.close();
        if (fsize != correct_size) {
            std::cerr << "Error write failed: Written " << fsize << " bytes to file. Should have written " << correct_size << " bytes.\n";
            return;
        }
    } else {
        std::cerr << "Error: can't open file for reading!\n";
        return;
    }

    // All is well :)
    std::cout << "Successfully written " << fsize << " bytes to " << fname << std::endl;
}

void MainWindow::button_trigger_frame_Click()
{
    if (m_triggerframe == TriggerFrame::OFF) {
        m_triggerframe = TriggerFrame::ON;
        chart->EnableTriggerFrame();
        button_trigger_frame->SetText("Frame ON");
    } else {
        m_triggerframe = TriggerFrame::OFF;
        chart->DisableTriggerFrame();
        button_trigger_frame->SetText("Frame OFF");
    }
}

void MainWindow::button_view_mode_Click()
{
    if (m_view == View::FILTERED) {
        m_view = View::RAW;
        button_view_mode->SetText("Raw");
    } else if (m_view == View::RAW) {
        m_view = View::FILTERED;
        button_view_mode->SetText("Filtered");
    }
}

void MainWindow::button_set_frequency_Click()
{
    m_communication->Write("SETFREQ," + textbox_frequency->GetText() + "\n");
    SetSampleFreq();
}

void MainWindow::button_set_filter_params_Click()
{
    m_communication->Write("SETPARAMS," + textbox_filter_params->GetText() + "\n");

    // Set threashold for all signals
    std::vector<std::string> strings = Help::TokenizeString(textbox_filter_params->GetText());
    for (auto& s : signals) {
        s->SetThreashold(std::stof(strings[3]));
    }
}

void MainWindow::button_set_times_Click()
{
    m_communication->Write("SETTIMES," + textbox_times->GetText() + "\n");

    // Set blind time for all signals
    std::vector<std::string> strings = Help::TokenizeString(textbox_times->GetText());
    for (auto& s : signals) {
        s->SetBlindTime(std::stoi(strings[2]));
    }
}

void MainWindow::button_record_Click()
{
    auto const& ResetSignals = [this]() {
        for (int i = 0; i < N_CHANNELS; ++i)
            chart->ChangeSignal(i, signals[i]);
        m_chart_frame_idx = -1;
    };

    if (*m_record == Record::NO) {
        *m_record = Record::ALL;
        button_record->SetColor(sf::Color::Red);
        recorded_signals.clear();
        ResetSignals();
        label_recorded_signals_counter->SetText("0");
    } else if (*m_record == Record::ALL) {
        *m_record = Record::EVENTS;
        button_record->SetColor(sf::Color::Yellow);
        recorded_signals.clear();
        ResetSignals();
        label_recorded_signals_counter->SetText("0");
    } else if (*m_record == Record::EVENTS) {
        *m_record = Record::NO;
        button_record->ResetColor();
        recorded_signals.clear();
        ResetSignals();
        label_recorded_signals_counter->SetText("0");
    }
}

void MainWindow::button_info_Click()
{
    if (!m_detectionInfoWindow->IsOpen()) {
        m_detectionInfoWindow = std::make_shared<InfoWindow>("Detection Info", "det.py");
        m_detectionInfoWindow->SetPosition(GetPosition() + sf::Vector2i(1850 - 480, 40));
        for (auto& s : signals) {
            m_detectionInfoWindow->push_back(s->GetDetectionStats());
        }
        //m_detectionInfoWindow->SetAll(Signal::GetDetectionStatsAll());
    }
    if (!m_frameInfoWindow->IsOpen()) {
        m_frameInfoWindow = std::make_shared<InfoWindow>("Frame Info", "win.py");
        m_frameInfoWindow->SetPosition(GetPosition() + sf::Vector2i(1850 - 1000, 40));
        for (auto& s : signals) {
            m_frameInfoWindow->push_back(s->GetTriggerWindowStats());
        }
        //m_frameInfoWindow->SetAll(Signal::GetTriggerWindowStatsAll());
    }
}

void MainWindow::button_clear_all_Click()
{
    auto const& ResetSignals = [this]() {
        for (int i = 0; i < N_CHANNELS; ++i)
            chart->ChangeSignal(i, signals[i]);
        m_chart_frame_idx = -1;
    };

    m_frameInfoWindow->Clear();
    m_detectionInfoWindow->Clear();
    label_info_detected_in_window_Clicked();
    label_info_detected_out_window_Clicked();
    label_info_signal_missed_Clicked();

    recorded_signals.clear();
    for (auto& s : signals)
        s->ClearRXData();
    ResetSignals();
    label_recorded_signals_counter->SetText("0");
}

void MainWindow::textbox_detection_time_min_KeyPress()
{
    try {
        int val               = std::stoi(textbox_detection_time_min->GetText());
        *m_detection_time_min = val;
    } catch (std::invalid_argument) {
        *m_detection_time_min = 0;
    }
}

void MainWindow::textbox_detection_time_max_KeyPress()
{
    try {
        int val               = std::stoi(textbox_detection_time_max->GetText());
        *m_detection_time_min = val;
    } catch (std::invalid_argument) {
        *m_detection_time_min = 1000000; // arbitrarily large number
    }
}

void MainWindow::textbox_window_time_min_KeyPress()
{
    try {
        int val            = std::stoi(textbox_window_time_min->GetText());
        *m_window_time_min = val;
    } catch (std::invalid_argument) {
        *m_window_time_min = 0;
    }
}

void MainWindow::textbox_window_time_max_KeyPress()
{
    try {
        int val            = std::stoi(textbox_window_time_max->GetText());
        *m_window_time_max = val;
    } catch (std::invalid_argument) {
        *m_window_time_max = 1000000; // arbitrarily large number
    }
}

void MainWindow::label_info_detected_in_window_Clicked()
{
    for (auto& s : signals) {
        s->ClearDetectionsInWindow();
    }
}

void MainWindow::label_info_detected_out_window_Clicked()
{
    for (auto& s : signals) {
        s->ClearDetectionsOutWindow();
    }
}

void MainWindow::label_info_signal_missed_Clicked()
{
    for (auto& s : signals) {
        s->ClearMissed();
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
        s->OnlyDrawOnTrigger(checkbox_only_show_framed->Checked());
    }
}
void MainWindow::checkbox_show_event_indicator_Clicked()
{
    for (auto& s : signals) {
        s->ShowEventIndicator(checkbox_show_event_indicator->Checked());
    }

    for (auto& rs : recorded_signals) {
        rs->ShowEventIndicator(checkbox_show_event_indicator->Checked());
    }
}

void MainWindow::RecordEvent(Signal::Event e, bool on)
{
    if (on)
        *m_events_to_record = static_cast<Signal::Event>(*m_events_to_record | e);
    else
        *m_events_to_record = static_cast<Signal::Event>(*m_events_to_record & ~e);
};

void MainWindow::checkbox_detected_in_Clicked()
{
    RecordEvent(Signal::Event::DETECTED_IN, checkbox_detected_in->Checked());
}

void MainWindow::checkbox_detected_out_Clicked()
{
    RecordEvent(Signal::Event::DETECTED_OUT, checkbox_detected_out->Checked());
}

void MainWindow::checkbox_missed_Clicked()
{
    RecordEvent(Signal::Event::MISSED, checkbox_missed->Checked());
}

void MainWindow::checkbox_detection_time_Clicked()
{
    RecordEvent(Signal::Event::DETECTION_TIME, checkbox_detection_time->Checked());
}

void MainWindow::checkbox_window_time_Clicked()
{
    RecordEvent(Signal::Event::WINDOW_TIME, checkbox_window_time->Checked());
}

void MainWindow::chart_OnKeyPress(const sf::Event& event)
{
    if ((*m_record == Record::ALL || *m_record == Record::EVENTS) && *m_running == Running::STOPPED) {
        const int size = ((int)recorded_signals.size() / N_CHANNELS); // conversion from size_t to int (const int size) must be made or the bottom evaluation is wrong since comparing signed to unsigned, compiler promotes signed to unsigned converting -1 to maximum int value

        if (event.key.code == sf::Keyboard::Left) {
            if (m_chart_frame_idx > 0 && m_chart_frame_idx < size) {
                m_chart_frame_idx--;
                for (int i = 0; i < N_CHANNELS; ++i)
                    chart->ChangeSignal(i, recorded_signals[m_chart_frame_idx * N_CHANNELS + i]);
            }
        }
        if (event.key.code == sf::Keyboard::Right) {
            if (m_chart_frame_idx < (size - 1)) {
                m_chart_frame_idx++;
                for (int i = 0; i < N_CHANNELS; ++i)
                    chart->ChangeSignal(i, recorded_signals[m_chart_frame_idx * N_CHANNELS + i]);
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
    chart = std::make_shared<Chart>(240, 10, 1600, 880, samples, 100.f);
    chart->CreateGrid(9);
    chart->OnKeyPress(std::bind(&MainWindow::chart_OnKeyPress, this, std::placeholders::_1));
    signals.clear();
    signals.reserve(N_CHANNELS);
    recorded_signals.reserve(N_CHANNELS * 10); // make an arbitrary reservation, just so there aren't so many reallocations when first recording
    for (int i = 0; i < N_CHANNELS; ++i) {
        chart->AddSignal(
            signals.emplace_back(
                std::make_shared<Signal>(samples, sf::Color(m_Colors[i]), chart->GraphRegion(), chart->MaxVal())));
        signals[i]->EventsToRecord(m_events_to_record);
        signals[i]->WindowAndDetectionTimeLimits(m_detection_time_min, m_detection_time_max, m_window_time_min, m_window_time_max);
    }
    *m_events_to_record = static_cast<Signal::Event>(Signal::Event::MISSED | Signal::Event::DETECTED_OUT);
}

void MainWindow::RunClick()
{
    button_run_Click();
}

MainWindow::MainWindow(int w, int h, std::string const& title, std::string const& com_port, uint32_t num_of_samples, sf::Uint32 style) :
    Window(w, h, title, style), m_config_number_of_samples(num_of_samples)
{
    ///////////
    // Chart //
    ///////////
    m_events_to_record   = std::make_shared<Signal::Event>();
    m_detection_time_min = std::make_shared<uint32_t>(1000000);
    m_detection_time_max = std::make_shared<uint32_t>(0);
    m_window_time_min    = std::make_shared<uint32_t>(1000000);
    m_window_time_max    = std::make_shared<uint32_t>(0);
    CreateChart(num_of_samples);

    /////////////
    // Buttons //
    /////////////
    button_connect = std::make_shared<mygui::Button>(10, 50, "Connect", 100);
    button_connect->OnClick(std::bind(&MainWindow::button_connect_Click, this));

    button_run = std::make_shared<mygui::Button>(10, 90, "Stopped", 100);
    button_run->OnClick(std::bind(&MainWindow::button_run_Click, this));

    button_save = std::make_shared<mygui::Button>(10, 130, "Save", 100);
    button_save->OnClick(std::bind(&MainWindow::button_save_Click, this));

    button_trigger_frame = std::make_shared<mygui::Button>(125, 50, "Frame OFF", 100, 30, 18);
    button_trigger_frame->OnClick(std::bind(&MainWindow::button_trigger_frame_Click, this));

    button_view_mode = std::make_shared<mygui::Button>(125, 90, "Raw", 100, 30, 18);
    button_view_mode->OnClick(std::bind(&MainWindow::button_view_mode_Click, this));

    button_info_windows = std::make_shared<mygui::Button>(125, 130, "Info", 100, 30, 18);
    button_info_windows->OnClick(std::bind(&MainWindow::button_info_Click, this));

    button_set_frequency = std::make_shared<mygui::Button>(10, 240, "Send");
    button_set_frequency->OnClick(std::bind(&MainWindow::button_set_frequency_Click, this));

    button_set_filter_params = std::make_shared<mygui::Button>(10, 350, "Send");
    button_set_filter_params->OnClick(std::bind(&MainWindow::button_set_filter_params_Click, this));

    button_set_times = std::make_shared<mygui::Button>(10, 460, "Send");
    button_set_times->OnClick(std::bind(&MainWindow::button_set_times_Click, this));

    button_record = std::make_shared<mygui::Button>(10, 650, "Record");
    button_record->OnClick(std::bind(&MainWindow::button_record_Click, this));

    button_clear_all = std::make_shared<mygui::Button>(10, 855, "Clear ALL");
    button_clear_all->OnClick(std::bind(&MainWindow::button_clear_all_Click, this));

    //////////////
    // Texboxes //
    //////////////
    textbox_comport = std::make_shared<mygui::Textbox>(10, 10, "COM", 80);
    textbox_comport->SetText(com_port);
    textbox_frequency          = std::make_shared<mygui::Textbox>(10, 200, "", 80);
    textbox_filter_params      = std::make_shared<mygui::Textbox>(10, 310, "", 210);
    textbox_times              = std::make_shared<mygui::Textbox>(10, 420, "", 140);
    textbox_detection_time_min = std::make_shared<mygui::Textbox>(35, 787, "", 40, 25);
    textbox_detection_time_min->onKeyPress(std::bind(&MainWindow::textbox_detection_time_min_KeyPress, this));
    textbox_detection_time_max = std::make_shared<mygui::Textbox>(185, 787, "", 40, 25);
    textbox_detection_time_max->onKeyPress(std::bind(&MainWindow::textbox_detection_time_max_KeyPress, this));
    textbox_window_time_min = std::make_shared<mygui::Textbox>(35, 817, "", 40, 25);
    textbox_window_time_min->onKeyPress(std::bind(&MainWindow::textbox_window_time_min_KeyPress, this));
    textbox_window_time_max = std::make_shared<mygui::Textbox>(185, 817, "", 40, 25);
    textbox_window_time_max->onKeyPress(std::bind(&MainWindow::textbox_window_time_max_KeyPress, this));

    ////////////
    // Labels //
    ////////////
    label_frequency                = std::make_shared<mygui::Label>(10, 170, "Sample frequency:");
    label_filter_params            = std::make_shared<mygui::Label>(10, 280, "Filter params(a1,a2,a3,thr):");
    label_times                    = std::make_shared<mygui::Label>(10, 390, "Times (dly, dur, blind):");
    label_recorded_signals_counter = std::make_shared<mygui::Label>(120, 654, "0");
    label_info_rx_id_avail         = std::make_shared<mygui::Label>(10, 530, "Rx cnt: 0 available: 0 bytes", 14);
    label_info_rx_time_took_speed  = std::make_shared<mygui::Label>(10, 550, "Rx took: 0 ms at: 0 kB/s", 14);
    label_info_parse_data_time     = std::make_shared<mygui::Label>(10, 570, "Parsing data took: 0 ms", 14);
    label_info_detected_in_window  = std::make_shared<mygui::Label>(120, 698, "0");
    label_info_detected_in_window->OnClick(std::bind(&MainWindow::label_info_detected_in_window_Clicked, this));
    label_info_detected_out_window = std::make_shared<mygui::Label>(120, 729, "0");
    label_info_detected_out_window->OnClick(std::bind(&MainWindow::label_info_detected_out_window_Clicked, this));
    label_info_signal_missed = std::make_shared<mygui::Label>(120, 760, "0");
    label_info_signal_missed->OnClick(std::bind(&MainWindow::label_info_signal_missed_Clicked, this));
    label_detection_time = std::make_shared<mygui::Label>(80, 787, "> det time >");
    label_detection_time->OnClick(std::bind(&MainWindow::label_detection_time_Clicked, this));
    label_window_time = std::make_shared<mygui::Label>(80, 817, "> win time >");
    label_window_time->OnClick(std::bind(&MainWindow::label_window_time_Clicked, this));

    ////////////////
    // Checkboxes //
    ////////////////
    checkbox_transparent = std::make_shared<mygui::Checkbox>(125, 16, "Transparent", 15, 15, 15);
    checkbox_transparent->OnClick(std::bind(&MainWindow::checkbox_transparent_Clicked, this));

    checkbox_only_show_framed = std::make_shared<mygui::Checkbox>(10, 500, "Only show framed");
    checkbox_only_show_framed->OnClick(std::bind(&MainWindow::checkbox_only_show_framed_Clicked, this));

    checkbox_show_event_indicator = std::make_shared<mygui::Checkbox>(10, 620, "Show event lines");
    checkbox_show_event_indicator->OnClick(std::bind(&MainWindow::checkbox_show_event_indicator_Clicked, this));
    checkbox_show_event_indicator->Checked(true);

    checkbox_detected_in = std::make_shared<mygui::Checkbox>(10, 700, "Det IN: ");
    checkbox_detected_in->OnClick(std::bind(&MainWindow::checkbox_detected_in_Clicked, this));
    checkbox_detected_in->Checked(false);

    checkbox_detected_out = std::make_shared<mygui::Checkbox>(10, 730, "Det OUT: ");
    checkbox_detected_out->OnClick(std::bind(&MainWindow::checkbox_detected_out_Clicked, this));
    checkbox_detected_out->Checked(true);

    checkbox_missed = std::make_shared<mygui::Checkbox>(10, 760, "Missed: ");
    checkbox_missed->OnClick(std::bind(&MainWindow::checkbox_missed_Clicked, this));
    checkbox_missed->Checked(true);

    checkbox_detection_time = std::make_shared<mygui::Checkbox>(10, 790, "");
    checkbox_detection_time->OnClick(std::bind(&MainWindow::checkbox_detection_time_Clicked, this));
    checkbox_detection_time->Checked(false);

    checkbox_window_time = std::make_shared<mygui::Checkbox>(10, 820, "");
    checkbox_window_time->OnClick(std::bind(&MainWindow::checkbox_window_time_Clicked, this));
    checkbox_window_time->Checked(false);

    /////////////////
    // Main window //
    /////////////////
    Add(chart);

    // Buttons
    Add(button_connect);
    Add(button_run);
    Add(button_save);
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
    Add(label_info_rx_id_avail);
    Add(label_info_rx_time_took_speed);
    Add(label_info_parse_data_time);
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
    if (*m_running == Running::RUNNING)
        button_run_Click();
}

void MainWindow::ConnectCrossData(
    std::shared_ptr<Communication> communication,
    std::shared_ptr<InfoWindow>    detectionInfoWindow,
    std::shared_ptr<InfoWindow>    frameInfoWindow,
    std::shared_ptr<Running>       running,
    std::shared_ptr<Record>        record)
{
    m_communication       = communication;
    m_detectionInfoWindow = detectionInfoWindow;
    m_frameInfoWindow     = frameInfoWindow;

    m_running = running;
    m_record  = record;
}

void MainWindow::UpdateSignals(ProtocolDataType* data)
{
    // Update signals with new m_data
    for (auto& s : signals) {
        s->Edit(data, m_signal_update_cntr * DATA_PER_CHANNEL, DATA_PER_CHANNEL, m_view);
        data += DATA_PER_CHANNEL;
    }

    // If we filled up char/signal
    if (++m_signal_update_cntr >= (m_config_number_of_samples / DATA_PER_CHANNEL)) {
        m_signal_update_cntr = 0;
        if (*m_record == Record::ALL) {
            for (auto const& s : signals)
                recorded_signals.push_back(s);
            label_recorded_signals_counter->SetText(std::to_string(recorded_signals.size() / N_CHANNELS));
        } else if (*m_record == Record::EVENTS) {
            bool event_happened = false;
            for (auto const& s : signals) {
                if (s->AnyEvents()) {
                    event_happened = true;
                    break;
                }
            }

            if (event_happened) {
                for (auto& s : signals) {
                    if (s->AnyEvents()) {
                        recorded_signals.push_back(s);
                        s->ClearEvents();
                    } else {
                        recorded_signals.push_back(std::make_shared<Signal>()); // push empty signal
                    }
                }
                label_recorded_signals_counter->SetText(std::to_string(recorded_signals.size() / N_CHANNELS));
            }
        }

        if (m_frameInfoWindow)
            m_frameInfoWindow->RefreshTable();
        if (m_detectionInfoWindow)
            m_detectionInfoWindow->RefreshTable();
    }
}