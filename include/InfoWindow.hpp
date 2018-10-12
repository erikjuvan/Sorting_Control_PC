#pragma once

#include "Application.hpp"
#include "Helpers.hpp"
#include "Window.hpp"
#include <mygui/Button.hpp>
#include <mygui/Label.hpp>
#include <vector>

class InfoWindow : public Window
{
private:
    std::vector<Statistics<int64_t>*> m_channel;
    Statistics<int64_t>*              m_all;
    std::string                       m_save_filename;

    void Clear();
    void SaveRecord();

public:
    InfoWindow(char const* title, std::string const& save_filename);
    ~InfoWindow();

    void push_back(Statistics<int64_t>* s) { m_channel.push_back(s); }
    void SetAll(Statistics<int64_t>* all) { m_all = all; }
    void RefreshTable();

    // Members
    //////////

    mygui::Button* button_clear;
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

    InfoLabel infolabels_chs[N_CHANNELS];
    InfoLabel infolabel_all;
};