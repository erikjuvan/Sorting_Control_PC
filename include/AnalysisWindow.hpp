#pragma once

#include "Application.hpp"
#include "Helpers.hpp"
#include "Window.hpp"
#include <mygui/Button.hpp>
#include <mygui/Label.hpp>

struct SortingAnalysis {

    Statistics<int> channel[N_CHANNELS];
    Statistics<int> total;
    bool            m_record{false};

    void ClearAll();
    void Add(uint32_t* data, int size);
    void SaveRecord(char const* fname);
};

class AnalysisWindow : public Window
{
private:
    SortingAnalysis* m_analysis;

public:
    // Methods
    //////////

    AnalysisWindow(char const* title);
    ~AnalysisWindow();

    void NewData(uint32_t* data, int size);

    static void button_clear_all_Clicked();
    static void button_record_Clicked();
    static void button_save_Clicked();

    // Members
    //////////

    mygui::Button* button_clear_all;
    mygui::Button* button_record;
    mygui::Button* button_save;

    mygui::Label* label_info_win_to_det_min;
    mygui::Label* label_info_win_to_det_max;
    mygui::Label* label_info_win_to_det_avg;
    mygui::Label* label_info_win_to_det_std_dev;
    mygui::Label* label_info_win_to_det_last;
    mygui::Label* label_info_win_to_det_cnt;

    struct InfoLabel {
        mygui::Label* channel_number;
        mygui::Label* label_min;
        mygui::Label* label_max;
        mygui::Label* label_avg;
        mygui::Label* label_std_dev;
        mygui::Label* label_last;
        mygui::Label* label_cnt;

        ~InfoLabel();
    };

    InfoLabel labels[N_CHANNELS];
    InfoLabel label_all;
};