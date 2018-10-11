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

public:
    // Methods
    //////////

    MainWindow(int w, int h, const char* title, sf::Uint32 style = sf::Style::Default);
    ~MainWindow();

    void RunClick();
    void CreateChart(int n_sample);

    static void button_connect_Click();
    static void button_run_Click();
    static void button_trigger_frame_Click();
    static void button_set_frequency_Click();
    static void button_set_filter_params_Click();
    static void button_set_times_Click();
    static void button_view_mode_Click();
    static void button_capture_Click();
    static void button_record_Click();
    static void button_analysis_window_Click();

    static void label_info_detected_in_window_Clicked();
    static void label_info_detected_out_window_Clicked();
    static void label_info_signal_missed_Clicked();

    static void checkbox_detected_in_Clicked();
    static void checkbox_detected_out_Clicked();
    static void checkbox_missed_Clicked();
    static void checkbox_only_show_framed_Clicked();
    static void checkbox_transparent_Clicked();

    static void chart_OnKeyPress(const sf::Event&);

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
    mygui::Button* button_analysis_window;
    // Texbox
    mygui::Textbox* textbox_comport;
    mygui::Textbox* textbox_frequency;
    mygui::Textbox* textbox_filter_params;
    mygui::Textbox* textbox_times;

    // Labels
    mygui::Label* label_frequency;
    mygui::Label* label_filter_params;
    mygui::Label* label_times;
    mygui::Label* label_recorded_signals_counter;
    mygui::Label* label_info_rx_bytes;
    mygui::Label* label_info_detected_in_window;
    mygui::Label* label_info_detected_out_window;
    mygui::Label* label_info_signal_missed;

    // Checkboxes
    mygui::Checkbox* checkbox_detected_in;
    mygui::Checkbox* checkbox_detected_out;
    mygui::Checkbox* checkbox_missed;
    mygui::Checkbox* checkbox_only_show_framed;
    mygui::Checkbox* checkbox_transparent;

    std::vector<Signal> signals;
    std::vector<Signal> recorded_signals;
};
