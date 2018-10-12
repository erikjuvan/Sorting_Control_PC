#pragma once

#include "Application.hpp"
#include "Helpers.hpp"
#include "Window.hpp"
#include <mygui/Button.hpp>
#include <mygui/Label.hpp>
#include <vector>

class FrameInfoWindow : public Window
{
private:
    std::vector<Statistics<int>*> m_channel;

public:
    static void button_clear_all_Clicked();
    static void button_save_Clicked();

    FrameInfoWindow(char const* title);
    ~FrameInfoWindow();

    void push_back(Statistics<int>*);
    void RefreshTable();
    void Clear();
    void SaveRecord(char const* fname);

    // Members
    //////////

    mygui::Button* button_clear_all;
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