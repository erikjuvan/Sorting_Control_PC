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
    static constexpr uint32_t m_Colors[10]{0xFF0000FF, 0x00FF00FF, 0x0000FFFF, 0xFFFF00FF, 0x00FFFFFF, 0xFF00FFFF, 0xFF8000FF, 0xC0C0C0FF, 0x800000FF, 0x808000FF};

    void RunClick();
    void CreateChart(int n_sample);

    void button_connect_Click();
    void button_run_Click();
    void button_trigger_frame_Click();
    void button_set_frequency_Click();
    void button_set_filter_params_Click();
    void button_set_times_Click();
    void button_view_mode_Click();
    void button_record_Click();
    void button_info_Click();

    void textbox_detection_time_min_KeyPress();
    void textbox_detection_time_max_KeyPress();
    void textbox_window_time_min_KeyPress();
    void textbox_window_time_max_KeyPress();

    void label_info_detected_in_window_Clicked();
    void label_info_detected_out_window_Clicked();
    void label_info_signal_missed_Clicked();
    void label_detection_time_Clicked();
    void label_window_time_Clicked();

    void checkbox_detected_in_Clicked();
    void checkbox_detected_out_Clicked();
    void checkbox_missed_Clicked();
    void checkbox_only_show_framed_Clicked();
    void checkbox_transparent_Clicked();
    void checkbox_detection_time_Clicked();
    void checkbox_window_time_Clicked();

    void chart_OnKeyPress(const sf::Event&);

public:
    MainWindow(int w, int h, const char* title, sf::Uint32 style = sf::Style::Default);
    ~MainWindow();

    // Members
    //////////

    Chart* chart;
    // Button
    mygui::Button* button_connect;
    mygui::Button* button_run;
    mygui::Button* button_trigger_frame;
    mygui::Button* button_set_frequency;
    mygui::Button* button_set_filter_params;
    mygui::Button* button_set_times;
    mygui::Button* button_view_mode;
    mygui::Button* button_record;
    mygui::Button* button_info_windows;
    // Texbox
    mygui::Textbox* textbox_comport;
    mygui::Textbox* textbox_frequency;
    mygui::Textbox* textbox_filter_params;
    mygui::Textbox* textbox_times;
    mygui::Textbox* textbox_detection_time_min;
    mygui::Textbox* textbox_detection_time_max;
    mygui::Textbox* textbox_window_time_min;
    mygui::Textbox* textbox_window_time_max;

    // Labels
    mygui::Label* label_frequency;
    mygui::Label* label_filter_params;
    mygui::Label* label_times;
    mygui::Label* label_recorded_signals_counter;
    mygui::Label* label_info_rx_bytes;
    mygui::Label* label_info_detected_in_window;
    mygui::Label* label_info_detected_out_window;
    mygui::Label* label_info_signal_missed;
    mygui::Label* label_detection_time;
    mygui::Label* label_window_time;

    // Checkboxes
    mygui::Checkbox* checkbox_transparent;
    mygui::Checkbox* checkbox_only_show_framed;
    mygui::Checkbox* checkbox_detected_in;
    mygui::Checkbox* checkbox_detected_out;
    mygui::Checkbox* checkbox_missed;
    mygui::Checkbox* checkbox_detection_time;
    mygui::Checkbox* checkbox_window_time;

    std::vector<Signal> signals;
    std::vector<Signal> recorded_signals;
};
