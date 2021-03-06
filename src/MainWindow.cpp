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
    try {
        *m_sample_freq_hz = std::stoi(textbox_frequency->GetText());
    } catch (std::invalid_argument& ia) {
        std::cerr << ia.what() << std::endl;
    } catch (std::out_of_range& oor) {
        std::cerr << oor.what() << std::endl;
    }
}

void MainWindow::ImportSCParameters()
{
    // Sample frequency
    ////////////////////////////////
    auto tokens = m_communication->WriteAndTokenizeResult("FRQG\n");
    textbox_frequency->SetText(tokens[0]);
    SetSampleFreq();

    // Sorting ticks
    ////////////////////////////////
    tokens = m_communication->WriteAndTokenizeResult("SRTG\n");
    if (tokens.size() < 3)
        std::cerr << "Received invalid ticks\n";
    else
        for (auto& s : signals)
            s->SetBlindTicks(std::stoi(tokens[2]));

    // Convert ticks to times_ms
    std::string txtbx_times_ms;
    for (auto t : tokens)
        if (*m_sample_freq_hz > 0) // prevent division by zero
            txtbx_times_ms += std::to_string(std::stoi(t) * 1000 / *m_sample_freq_hz) + ",";
        else
            txtbx_times_ms += "0,";
    txtbx_times_ms.pop_back();
    textbox_times->SetText(txtbx_times_ms);

    // Filter coefficients
    ////////////////////////////////
    tokens = m_communication->WriteAndTokenizeResult("FILG\n");
    if (tokens.size() < 3)
        std::cerr << "Received invalid filter coefficients\n";
    std::string txtbx_filter_coeffs;
    for (const auto& t : tokens)
        txtbx_filter_coeffs += t + ",";
    txtbx_filter_coeffs.pop_back();
    textbox_filter_coeffs->SetText(txtbx_filter_coeffs);

    // Threshold
    ////////////////////////////////
    tokens = m_communication->WriteAndTokenizeResult("THRG\n");
    if (tokens.size() < 1)
        std::cerr << "Received invalid threshold\n";
    else
        for (auto& s : signals)
            s->SetThreashold(std::stof(tokens[0]));
    textbox_threshold->SetText(tokens[0]);
}

void MainWindow::button_connect_Click()
{
    if (!m_communication->IsConnected()) {
        if (m_communication->Connect(textbox_comport->GetText())) {

            button_connect->SetText("Disconnect");

            // If we previously crashed we could still be receving m_data, so make sure we stop before configuring
            m_communication->StopTransmissionAndSuperPurge();

            ImportSCParameters();

            textbox_comport->Enabled(false);

            // Enable all textboxes and send buttons
            textbox_frequency->Enabled(true);
            textbox_times->Enabled(true);
            textbox_filter_coeffs->Enabled(true);
            textbox_threshold->Enabled(true);
            textbox_send_raw->Enabled(true);
            button_set_frequency->Enabled(true);
            button_set_times->Enabled(true);
            button_set_filter_coeffs->Enabled(true);
            button_set_threshold->Enabled(true);
            button_send_raw->Enabled(true);
        } else {
            std::cout << "Can't connect to port " << textbox_comport->GetText() << std::endl;
        }
    } else {
        if (*m_running)
            button_run_Click();
        m_communication->Disconnect();
        button_connect->SetText("Connect");

        textbox_comport->Enabled(true);
        // Disable all textboxes and send buttons
        textbox_frequency->SetText("");
        textbox_frequency->Enabled(false);
        textbox_times->SetText("");
        textbox_times->Enabled(false);
        textbox_filter_coeffs->SetText("");
        textbox_filter_coeffs->Enabled(false);
        textbox_threshold->SetText("");
        textbox_threshold->Enabled(false);
        textbox_send_raw->Enabled(false);
        button_set_frequency->Enabled(false);
        button_set_times->Enabled(false);
        button_set_filter_coeffs->Enabled(false);
        button_set_threshold->Enabled(false);
        button_save->Enabled(false);
        button_send_raw->Enabled(false);
    }
}

void MainWindow::button_run_Click()
{
    if (!m_communication->IsConnected())
        return;

    if (!*m_running) {

        // Just to make sure serial buffers are clear before importing parameters
        m_communication->StopTransmissionAndSuperPurge();

        // Get the latest parameters that are actually on SC
        ImportSCParameters();

        // Make sure we enter sort mode
        m_communication->Write("SORT\n");
        m_communication->ConfirmTransmission("SORT\n"); // currently all ConfirmTransmissions are neccessary since they also clear the serial rx buffer

        // Set verbose mode, so we start receving data
        m_communication->Write("VRBS,1\n");
        m_communication->ConfirmTransmission("VRBS,1\n");

        // Disable all textboxes and send buttons so as to not be able to overwrite any parameters (so savefile parameters are 100% sure to be correct)
        textbox_frequency->Enabled(false);
        textbox_times->Enabled(false);
        textbox_filter_coeffs->Enabled(false);
        textbox_threshold->Enabled(false);
        textbox_send_raw->Enabled(false);
        button_set_frequency->Enabled(false);
        button_set_times->Enabled(false);
        button_set_filter_coeffs->Enabled(false);
        button_set_threshold->Enabled(false);
        button_save->Enabled(false);
        button_send_raw->Enabled(false);

        *m_running       = true;
        m_run_start_time = std::chrono::steady_clock::now();

        for (int i = 0; i < N_CHANNELS; ++i)
            chart->ChangeSignal(i, signals[i]);

        button_run->SetText("Running");

    } else {
        *m_running = false;
        m_communication->StopTransmissionAndSuperPurge();

        button_save->Enabled(true);
        button_send_raw->Enabled(true);
        textbox_send_raw->Enabled(true);

        button_run->SetText("Stopped");
    }
}

void MainWindow::button_save_Click()
{
    if (signals.size() <= 0) {
        std::cerr << "Error saving to file: signals.size() is 0\n";
        return;
    }
    if (signals[0]->GetRXData().size() <= 0) {
        std::cerr << "Error saving to file: there is no data to save\n";
        return;
    }

    struct Header {
        uint32_t file_id = 0x43535453;  // "STSC" - STream Sorting Control
        uint32_t time_since_epoch_s;    // time since epoch in seconds when file was saved
        uint32_t num_of_channels;       // number of channels
        uint32_t sizeof_sample;         // size of sample in bytes
        uint32_t num_of_samples_per_ch; // number of samples per channel

        struct {
            uint32_t sample_frequency_hz;
            uint32_t delay_ticks;
            uint32_t duration_ticks;
            uint32_t blind_ticks;
            float    lpf1_K;
            float    hpf_K;
            float    lpf2_K;
            float    threshold;
        } sort_params;
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

    Header        header;
    auto          fname = get_available_filename("sc_data");
    std::ofstream write_file(fname, std::ofstream::binary);

    if (write_file.is_open()) {
        using namespace std::chrono;
        header.time_since_epoch_s    = static_cast<uint32_t>(duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
        header.num_of_channels       = N_CHANNELS;
        header.sizeof_sample         = sizeof(signals[0]->GetRXData()[0]);
        header.num_of_samples_per_ch = signals[0]->GetRXData().size();

        // First... super purge
        m_communication->StopTransmissionAndSuperPurge();

        // Get all sorting control parameters
        m_communication->Write("STTG");

        auto buf                               = m_communication->Readline();
        auto params                            = Help::TokenizeString(buf, " ,\n:");
        header.sort_params.sample_frequency_hz = std::stoi(params[1]);

        buf                               = m_communication->Readline();
        params                            = Help::TokenizeString(buf, " ,\n:");
        header.sort_params.delay_ticks    = std::stoi(params[1]);
        header.sort_params.duration_ticks = std::stoi(params[2]);
        header.sort_params.blind_ticks    = std::stoi(params[3]);

        buf                       = m_communication->Readline();
        params                    = Help::TokenizeString(buf, " ,\n:");
        header.sort_params.lpf1_K = std::stof(params[1]);
        header.sort_params.hpf_K  = std::stof(params[2]);
        header.sort_params.lpf2_K = std::stof(params[3]);

        buf                          = m_communication->Readline();
        params                       = Help::TokenizeString(buf, " ,\n:");
        header.sort_params.threshold = std::stof(params[1]);

        std::cout << "Saving data to " << fname << " ... ";

        write_file.write((const char*)&header, sizeof(Header));

        for (auto const& signal : signals) {
            auto const& vec = signal->GetRXData();
            if (vec.size() != header.num_of_samples_per_ch) {
                std::cerr << "Error saving to file: channel size mismatch!\n";
                write_file.close();
                return;
            }

            write_file.write((const char*)&vec[0], header.num_of_samples_per_ch * header.sizeof_sample);
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
        if (std::memcmp(&tmp, &header, sizeof(Header))) {
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
        std::ifstream::pos_type correct_size = sizeof(Header) + header.num_of_channels * header.num_of_samples_per_ch * header.sizeof_sample;
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
    if (!m_triggerframe) {
        m_triggerframe = true;
        chart->EnableTriggerFrame();
        button_trigger_frame->SetText("Frame ON");
    } else {
        m_triggerframe = false;
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
    std::string cmd = "FRQS," + textbox_frequency->GetText() + "\n";
    m_communication->Write(cmd);
    try {
        m_communication->ConfirmTransmission(cmd);
        std::cout << cmd;
    } catch (std::runtime_error& er) {
        std::cout << er.what() << std::endl;
        auto ret = m_communication->WriteAndTokenizeResult("FRQG\n");
        textbox_frequency->SetText(ret[0]);
    }
    SetSampleFreq();
}

void MainWindow::button_set_times_Click()
{
    auto             tok = Help::TokenizeString(textbox_times->GetText(), ", \n");
    std::vector<int> times_ms, ticks;
    for (auto t : tok) {
        try {
            times_ms.push_back(std::stoi(t));
        } catch (...) {
            std::cerr << "Error setting sorting times\n";
            return;
        }
    }

    for (auto t_ms : times_ms) {
        ticks.push_back((*m_sample_freq_hz * t_ms) / 1000); // ticks = desired_period_s / sample_period_s = desired_period_s / (1 / sample_freq_hz)
    }

    std::string command = "SRTS," + std::to_string(ticks.at(0)) + "," + std::to_string(ticks.at(1)) + "," + std::to_string(ticks.at(2)) + "\n";
    m_communication->Write(command);
    m_communication->ConfirmTransmission(command);
    std::cout << command;

    // Set blind time for all signals
    std::vector<std::string> strings = Help::TokenizeString(textbox_times->GetText(), ",");
    for (auto& s : signals) {
        s->SetBlindTicks(ticks.at(2));
    }
}

void MainWindow::button_set_filter_coeffs_Click()
{
    auto cmd = "FILS," + textbox_filter_coeffs->GetText() + "\n";
    m_communication->Write(cmd);
    m_communication->ConfirmTransmission(cmd);
    std::cout << cmd;

    // Set threashold for all signals
    std::vector<std::string> strings = Help::TokenizeString(textbox_filter_coeffs->GetText(), ",");
}

void MainWindow::button_set_threshold_Click()
{
    auto cmd = "THRS," + textbox_threshold->GetText() + "\n";
    m_communication->Write(cmd);
    m_communication->ConfirmTransmission(cmd);
    std::cout << cmd;

    // Set threashold for all signals
    std::vector<std::string> strings = Help::TokenizeString(textbox_threshold->GetText(), ",\n");
    for (auto& s : signals) {
        s->SetThreashold(std::stof(strings[0]));
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
    if (m_detectionInfoWindow->IsVisible() && m_frameInfoWindow->IsVisible()) {
        m_detectionInfoWindow->SetVisible(false);
        m_frameInfoWindow->SetVisible(false);
    } else {
        if (!m_detectionInfoWindow->IsVisible()) {
            m_detectionInfoWindow->SetVisible(true);
            m_detectionInfoWindow->SetPosition(
                {static_cast<int>(GetPosition().x + GetSize().x - m_detectionInfoWindow->GetSize().x - 10),
                 GetPosition().y + 40});
        }
        if (!m_frameInfoWindow->IsVisible()) {
            m_frameInfoWindow->SetVisible(true);
            m_frameInfoWindow->SetPosition(
                {static_cast<int>(m_detectionInfoWindow->GetPosition().x - m_frameInfoWindow->GetSize().x - 5),
                 m_detectionInfoWindow->GetPosition().y});
        }
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

    // Enable all textboxes and send buttons to again be able to set parameters
    if (!*m_running) {
        textbox_frequency->Enabled(true);
        textbox_times->Enabled(true);
        textbox_filter_coeffs->Enabled(true);
        textbox_threshold->Enabled(true);
        textbox_send_raw->Enabled(true);
        button_set_frequency->Enabled(true);
        button_set_times->Enabled(true);
        button_set_filter_coeffs->Enabled(true);
        button_set_threshold->Enabled(true);
        button_send_raw->Enabled(true);
    }
}

void MainWindow::button_send_raw_Click()
{
    m_communication->Write(textbox_send_raw->GetText() + "\n");
    m_communication->SetTimeout(100);
    auto lines = m_communication->Readlines();
    m_communication->SetTimeout(0); // Reset timeout (back to blocking mode)
    for (const auto& l : lines)
        std::cout << l;

    try {
        label_recv_raw->SetText(lines.at(0));
    } catch (std::exception& ex) {
    }
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

void MainWindow::textbox_send_raw_EnterPress()
{
    button_send_raw_Click();
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
    // TODO: yet to implement
}

void MainWindow::label_window_time_Clicked()
{
    // TODO: yet to implement
}

void MainWindow::label_recv_raw_Clicked()
{
    label_recv_raw->SetText("");
}

void MainWindow::checkbox_transparent_Clicked()
{
    if (!m_transparent) {
        m_transparent = true;
        MakeTransparent();
        SetTransparency(180);
    } else {
        m_transparent = false;
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
    if ((*m_record == Record::ALL || *m_record == Record::EVENTS) && !*m_running) {
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

void MainWindow::CreateChart()
{
    chart = std::make_shared<Chart>(190, 10, 1650, 880, m_config_number_of_samples, 100.f);
    chart->CreateGrid(9);
    chart->OnKeyPress(std::bind(&MainWindow::chart_OnKeyPress, this, std::placeholders::_1));
    signals.clear();
    signals.reserve(N_CHANNELS);
    recorded_signals.reserve(N_CHANNELS * 10); // make an arbitrary reservation, just so there aren't so many reallocations when first recording
    for (int i = 0; i < N_CHANNELS; ++i) {
        chart->AddSignal(
            signals.emplace_back(
                std::make_shared<Signal>(m_config_number_of_samples, sf::Color(m_Colors[i]), chart->GraphRegion(), chart->MaxVal())));
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
    m_sample_freq_hz = std::make_shared<int>(0);

    ///////////
    // Chart //
    ///////////
    m_events_to_record   = std::make_shared<Signal::Event>();
    m_detection_time_min = std::make_shared<uint32_t>(0);
    m_detection_time_max = std::make_shared<uint32_t>(1000000);
    m_window_time_min    = std::make_shared<uint32_t>(0);
    m_window_time_max    = std::make_shared<uint32_t>(1000000);
    CreateChart();

    /////////////
    // Buttons //
    /////////////
    button_connect = std::make_shared<mygui::Button>(10, 50, "Connect");
    button_connect->OnClick(std::bind(&MainWindow::button_connect_Click, this));

    button_run = std::make_shared<mygui::Button>(10, 90, "Stopped");
    button_run->OnClick(std::bind(&MainWindow::button_run_Click, this));

    button_save = std::make_shared<mygui::Button>(10, 130, "Save");
    button_save->OnClick(std::bind(&MainWindow::button_save_Click, this));

    button_trigger_frame = std::make_shared<mygui::Button>(100, 50, "Frame ON");
    button_trigger_frame->OnClick(std::bind(&MainWindow::button_trigger_frame_Click, this));

    button_view_mode = std::make_shared<mygui::Button>(100, 90, "Filtered");
    button_view_mode->OnClick(std::bind(&MainWindow::button_view_mode_Click, this));

    button_info_windows = std::make_shared<mygui::Button>(100, 130, "Info");
    button_info_windows->OnClick(std::bind(&MainWindow::button_info_Click, this));

    button_set_frequency = std::make_shared<mygui::Button>(120, 190, "Send", 50);
    button_set_frequency->OnClick(std::bind(&MainWindow::button_set_frequency_Click, this));
    button_set_frequency->Enabled(false);

    // Set times is correct since we will be inputing time in ms and then converting it to ticks to send to MCU
    button_set_times = std::make_shared<mygui::Button>(120, 250, "Send", 50);
    button_set_times->OnClick(std::bind(&MainWindow::button_set_times_Click, this));
    button_set_times->Enabled(false);

    button_set_filter_coeffs = std::make_shared<mygui::Button>(120, 310, "Send", 50);
    button_set_filter_coeffs->OnClick(std::bind(&MainWindow::button_set_filter_coeffs_Click, this));
    button_set_filter_coeffs->Enabled(false);

    button_set_threshold = std::make_shared<mygui::Button>(120, 370, "Send", 50);
    button_set_threshold->OnClick(std::bind(&MainWindow::button_set_threshold_Click, this));
    button_set_threshold->Enabled(false);

    button_record = std::make_shared<mygui::Button>(10, 480, "Record");
    button_record->OnClick(std::bind(&MainWindow::button_record_Click, this));

    button_clear_all = std::make_shared<mygui::Button>(10, 680, "Clear ALL");
    button_clear_all->OnClick(std::bind(&MainWindow::button_clear_all_Click, this));

    button_send_raw = std::make_shared<mygui::Button>(120, 740, "Send", 50);
    button_send_raw->OnClick(std::bind(&MainWindow::button_send_raw_Click, this));
    button_send_raw->Enabled(false);

    //////////////
    // Texboxes //
    //////////////
    textbox_comport = std::make_shared<mygui::Textbox>(10, 10, "COM");
    textbox_comport->SetText(com_port);
    textbox_notes     = std::make_shared<mygui::Textbox>(100, 10, "");
    textbox_frequency = std::make_shared<mygui::Textbox>(10, 190, "", 100);
    textbox_frequency->Enabled(false);
    textbox_times = std::make_shared<mygui::Textbox>(10, 250, "", 100);
    textbox_times->Enabled(false);
    textbox_filter_coeffs = std::make_shared<mygui::Textbox>(10, 310, "", 100);
    textbox_filter_coeffs->Enabled(false);
    textbox_threshold = std::make_shared<mygui::Textbox>(10, 370, "", 100);
    textbox_threshold->Enabled(false);
    textbox_detection_time_min = std::make_shared<mygui::Textbox>(35, 605, "", 30);
    textbox_detection_time_min->OnKeyPress(std::bind(&MainWindow::textbox_detection_time_min_KeyPress, this));
    textbox_detection_time_max = std::make_shared<mygui::Textbox>(145, 605, "", 30);
    textbox_detection_time_max->OnKeyPress(std::bind(&MainWindow::textbox_detection_time_max_KeyPress, this));
    textbox_window_time_min = std::make_shared<mygui::Textbox>(35, 635, "", 30);
    textbox_window_time_min->OnKeyPress(std::bind(&MainWindow::textbox_window_time_min_KeyPress, this));
    textbox_window_time_max = std::make_shared<mygui::Textbox>(145, 635, "", 30);
    textbox_window_time_max->OnKeyPress(std::bind(&MainWindow::textbox_window_time_max_KeyPress, this));
    textbox_send_raw = std::make_shared<mygui::Textbox>(10, 740, "", 100);
    textbox_send_raw->Enabled(false);
    textbox_send_raw->OnEnterPress(std::bind(&MainWindow::textbox_send_raw_EnterPress, this));

    ////////////
    // Labels //
    ////////////
    label_frequency                = std::make_shared<mygui::Label>(10, 178, "Sample frequency [Hz]:");
    label_times                    = std::make_shared<mygui::Label>(10, 238, "Times [ms] (dly, dur, blind):");
    label_filter_coeffs            = std::make_shared<mygui::Label>(10, 298, "Filter coeffs (lpf1, hpf, lpf2):");
    label_threshold                = std::make_shared<mygui::Label>(10, 358, "Threshold:");
    label_recorded_signals_counter = std::make_shared<mygui::Label>(100, 495, "Rec cnt: 0");
    label_info_rx_id_avail         = std::make_shared<mygui::Label>(10, 840, "Rx id: 0 available: 0 bytes");
    label_info_rx_time_took_speed  = std::make_shared<mygui::Label>(10, 860, "Rx took: 0 ms at: 0 kB/s");
    label_info_parse_data_time     = std::make_shared<mygui::Label>(10, 880, "Parsing data took: 0 ms");
    label_info_detected_in_window  = std::make_shared<mygui::Label>(100, 527, "0");
    label_info_detected_in_window->OnClick(std::bind(&MainWindow::label_info_detected_in_window_Clicked, this));
    label_info_detected_out_window = std::make_shared<mygui::Label>(100, 557, "0");
    label_info_detected_out_window->OnClick(std::bind(&MainWindow::label_info_detected_out_window_Clicked, this));
    label_info_signal_missed = std::make_shared<mygui::Label>(100, 587, "0");
    label_info_signal_missed->OnClick(std::bind(&MainWindow::label_info_signal_missed_Clicked, this));
    label_detection_time = std::make_shared<mygui::Label>(70, 617, "> det time >");
    label_detection_time->OnClick(std::bind(&MainWindow::label_detection_time_Clicked, this));
    label_window_time = std::make_shared<mygui::Label>(70, 647, "> win time >");
    label_window_time->OnClick(std::bind(&MainWindow::label_window_time_Clicked, this));
    label_send_raw = std::make_shared<mygui::Label>(10, 728, "Send raw:");
    label_recv_raw = std::make_shared<mygui::Label>(10, 785, "");
    label_recv_raw->OnClick(std::bind(&MainWindow::label_recv_raw_Clicked, this));

    ////////////////
    // Checkboxes //
    ///////////////
    checkbox_only_show_framed = std::make_shared<mygui::Checkbox>(10, 420, "Only show framed");
    checkbox_only_show_framed->OnClick(std::bind(&MainWindow::checkbox_only_show_framed_Clicked, this));

    checkbox_show_event_indicator = std::make_shared<mygui::Checkbox>(10, 450, "Show event lines");
    checkbox_show_event_indicator->OnClick(std::bind(&MainWindow::checkbox_show_event_indicator_Clicked, this));
    checkbox_show_event_indicator->Checked(true);

    checkbox_detected_in = std::make_shared<mygui::Checkbox>(10, 520, "Det IN: ");
    checkbox_detected_in->OnClick(std::bind(&MainWindow::checkbox_detected_in_Clicked, this));
    checkbox_detected_in->Checked(false);

    checkbox_detected_out = std::make_shared<mygui::Checkbox>(10, 550, "Det OUT: ");
    checkbox_detected_out->OnClick(std::bind(&MainWindow::checkbox_detected_out_Clicked, this));
    checkbox_detected_out->Checked(true);

    checkbox_missed = std::make_shared<mygui::Checkbox>(10, 580, "Missed: ");
    checkbox_missed->OnClick(std::bind(&MainWindow::checkbox_missed_Clicked, this));
    checkbox_missed->Checked(true);

    checkbox_detection_time = std::make_shared<mygui::Checkbox>(10, 610, "");
    checkbox_detection_time->OnClick(std::bind(&MainWindow::checkbox_detection_time_Clicked, this));
    checkbox_detection_time->Checked(false);

    checkbox_window_time = std::make_shared<mygui::Checkbox>(10, 640, "");
    checkbox_window_time->OnClick(std::bind(&MainWindow::checkbox_window_time_Clicked, this));
    checkbox_window_time->Checked(false);

    checkbox_transparent = std::make_shared<mygui::Checkbox>(10, 810, "Transparent");
    checkbox_transparent->OnClick(std::bind(&MainWindow::checkbox_transparent_Clicked, this));

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
    Add(button_set_times);
    Add(button_set_filter_coeffs);
    Add(button_set_threshold);
    Add(button_view_mode);
    Add(button_record);
    Add(button_info_windows);
    Add(button_clear_all);
    Add(button_send_raw);

    // Texboxes
    Add(textbox_comport);
    Add(textbox_notes);
    Add(textbox_frequency);
    Add(textbox_times);
    Add(textbox_filter_coeffs);
    Add(textbox_threshold);
    Add(textbox_detection_time_min);
    Add(textbox_detection_time_max);
    Add(textbox_window_time_min);
    Add(textbox_window_time_max);
    Add(textbox_send_raw);

    // Labels
    Add(label_frequency);
    Add(label_times);
    Add(label_filter_coeffs);
    Add(label_threshold);
    Add(label_recorded_signals_counter);
    Add(label_info_rx_id_avail);
    Add(label_info_rx_time_took_speed);
    Add(label_info_parse_data_time);
    Add(label_info_detected_in_window);
    Add(label_info_detected_out_window);
    Add(label_info_signal_missed);
    Add(label_detection_time);
    Add(label_window_time);
    Add(label_send_raw);
    Add(label_recv_raw);

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
    if (*m_running)
        button_run_Click();
}

void MainWindow::ConnectCrossData(
    std::shared_ptr<Communication> communication,
    std::shared_ptr<InfoWindow>    detectionInfoWindow,
    std::shared_ptr<InfoWindow>    frameInfoWindow,
    std::shared_ptr<bool>          running,
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
                recorded_signals.emplace_back(std::make_shared<Signal>(*s));
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
                        recorded_signals.push_back(std::make_shared<Signal>(*s));
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