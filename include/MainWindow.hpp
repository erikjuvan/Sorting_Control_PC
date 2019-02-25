#pragma once

#include <deque>

#include <mygui/Button.hpp>
#include <mygui/Checkbox.hpp>
#include <mygui/Label.hpp>
#include <mygui/Textbox.hpp>

#include "Application.hpp"
#include "Chart.hpp"
#include "Window.hpp"

class MainWindow : public Window
{
private:
    // Members
    const uint32_t m_Colors[10]{0xFF0000FF, 0x00FF00FF, 0x0000FFFF, 0xFFFF00FF, 0x00FFFFFF, 0xFF00FFFF, 0xFF8000FF, 0xC0C0C0FF, 0x800000FF, 0x808000FF};
    int            m_chart_frame_idx = -1; // -1 so that when we first press right arrow we get the first [0] frame

    std::chrono::time_point<std::chrono::steady_clock> m_run_start_time;

    bool m_transparent = false;

    int m_config_number_of_samples = 0;
    int m_signal_update_cntr       = 0;

    bool m_triggerframe = true;
    View m_view         = View::FILTERED;

    std::shared_ptr<Communication> m_communication;
    std::shared_ptr<InfoWindow>    m_detectionInfoWindow;
    std::shared_ptr<InfoWindow>    m_frameInfoWindow;

    std::shared_ptr<bool>   m_running;
    std::shared_ptr<Record> m_record;

    std::shared_ptr<Signal::Event> m_events_to_record;
    std::shared_ptr<uint32_t>      m_detection_time_min;
    std::shared_ptr<uint32_t>      m_detection_time_max;
    std::shared_ptr<uint32_t>      m_window_time_min;
    std::shared_ptr<uint32_t>      m_window_time_max;

    // Methods
    void RunClick();
    void CreateChart(std::shared_ptr<mygui::ResourceManager> const& rm);

    void SetSampleFreq();
    void RecordEvent(Signal::Event e, bool on);

    void button_connect_Click();
    void button_run_Click();
    void button_save_Click();
    void button_trigger_frame_Click();
    void button_set_frequency_Click();
    void button_set_filter_params_Click();
    void button_set_times_Click();
    void button_view_mode_Click();
    void button_record_Click();
    void button_info_Click();
    void button_clear_all_Click();

    void textbox_detection_time_min_KeyPress();
    void textbox_detection_time_max_KeyPress();
    void textbox_window_time_min_KeyPress();
    void textbox_window_time_max_KeyPress();

    void label_info_detected_in_window_Clicked();
    void label_info_detected_out_window_Clicked();
    void label_info_signal_missed_Clicked();
    void label_detection_time_Clicked();
    void label_window_time_Clicked();

    void checkbox_transparent_Clicked();
    void checkbox_only_show_framed_Clicked();
    void checkbox_show_event_indicator_Clicked();
    void checkbox_detected_in_Clicked();
    void checkbox_detected_out_Clicked();
    void checkbox_missed_Clicked();
    void checkbox_detection_time_Clicked();
    void checkbox_window_time_Clicked();

    void chart_OnKeyPress(const sf::Event&);

public:
    // Methods
    MainWindow(std::shared_ptr<mygui::ResourceManager> const& rm, int w, int h, std::string const& title, std::string const& com_port, uint32_t num_of_samples, sf::Uint32 style = sf::Style::Default);
    ~MainWindow();

    auto const& GetRunStartTime() { return m_run_start_time; }
    void        ConnectCrossData(std::shared_ptr<Communication> m_communication,
                                 std::shared_ptr<InfoWindow>    m_detectionInfoWindow,
                                 std::shared_ptr<InfoWindow>    m_frameInfoWindow,
                                 std::shared_ptr<bool>          m_running,
                                 std::shared_ptr<Record>        m_record);
    void        UpdateSignals(ProtocolDataType* data);

    // Members
    //////////

    std::shared_ptr<Chart> chart;

    // Button
    std::shared_ptr<mygui::Button> button_connect;
    std::shared_ptr<mygui::Button> button_run;
    std::shared_ptr<mygui::Button> button_save;
    std::shared_ptr<mygui::Button> button_trigger_frame;
    std::shared_ptr<mygui::Button> button_set_frequency;
    std::shared_ptr<mygui::Button> button_set_filter_params;
    std::shared_ptr<mygui::Button> button_set_times;
    std::shared_ptr<mygui::Button> button_view_mode;
    std::shared_ptr<mygui::Button> button_record;
    std::shared_ptr<mygui::Button> button_info_windows;
    std::shared_ptr<mygui::Button> button_clear_all;

    // Texbox
    std::shared_ptr<mygui::Textbox> textbox_comport;
    std::shared_ptr<mygui::Textbox> textbox_frequency;
    std::shared_ptr<mygui::Textbox> textbox_filter_params;
    std::shared_ptr<mygui::Textbox> textbox_times;
    std::shared_ptr<mygui::Textbox> textbox_detection_time_min;
    std::shared_ptr<mygui::Textbox> textbox_detection_time_max;
    std::shared_ptr<mygui::Textbox> textbox_window_time_min;
    std::shared_ptr<mygui::Textbox> textbox_window_time_max;

    // Labels
    std::shared_ptr<mygui::Label> label_frequency;
    std::shared_ptr<mygui::Label> label_filter_params;
    std::shared_ptr<mygui::Label> label_times;
    std::shared_ptr<mygui::Label> label_recorded_signals_counter;
    std::shared_ptr<mygui::Label> label_info_rx_id_avail;
    std::shared_ptr<mygui::Label> label_info_rx_time_took_speed;
    std::shared_ptr<mygui::Label> label_info_parse_data_time;
    std::shared_ptr<mygui::Label> label_info_detected_in_window;
    std::shared_ptr<mygui::Label> label_info_detected_out_window;
    std::shared_ptr<mygui::Label> label_info_signal_missed;
    std::shared_ptr<mygui::Label> label_detection_time;
    std::shared_ptr<mygui::Label> label_window_time;

    // Checkboxes
    std::shared_ptr<mygui::Checkbox> checkbox_transparent;
    std::shared_ptr<mygui::Checkbox> checkbox_only_show_framed;
    std::shared_ptr<mygui::Checkbox> checkbox_show_event_indicator;
    std::shared_ptr<mygui::Checkbox> checkbox_detected_in;
    std::shared_ptr<mygui::Checkbox> checkbox_detected_out;
    std::shared_ptr<mygui::Checkbox> checkbox_missed;
    std::shared_ptr<mygui::Checkbox> checkbox_detection_time;
    std::shared_ptr<mygui::Checkbox> checkbox_window_time;

    std::vector<std::shared_ptr<Signal>> signals;
    std::vector<std::shared_ptr<Signal>> recorded_signals;
};
