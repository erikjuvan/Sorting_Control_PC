#pragma once

#include "Application.hpp"
#include "Helpers.hpp"
#include "Window.hpp"
#include <array>
#include <mygui/Button.hpp>
#include <mygui/Label.hpp>
#include <vector>

class InfoWindow : public Window
{
private:
    std::vector<std::shared_ptr<Statistics<int64_t>>> m_channel;
    std::shared_ptr<Statistics<int64_t>>              m_all;
    std::string                                       m_save_filename;

    virtual void Events() override final;

public:
    InfoWindow(std::shared_ptr<mygui::ResourceManager> const& rm, std::string const& title, std::string const& save_filename);

    void Clear();
    void SaveRecord();

    void push_back(std::shared_ptr<Statistics<int64_t>> s) { m_channel.push_back(s); }
    void SetAll(std::shared_ptr<Statistics<int64_t>> all) { m_all = all; }
    void RefreshTable();

    // Members
    //////////

    std::shared_ptr<mygui::Button> button_clear;
    std::shared_ptr<mygui::Button> button_save;

    std::shared_ptr<mygui::Label> label_info_win_to_det_min;
    std::shared_ptr<mygui::Label> label_info_win_to_det_max;
    std::shared_ptr<mygui::Label> label_info_win_to_det_avg;
    std::shared_ptr<mygui::Label> label_info_win_to_det_std_dev;
    std::shared_ptr<mygui::Label> label_info_win_to_det_last;
    std::shared_ptr<mygui::Label> label_info_win_to_det_cnt;

    struct InfoLabel {
        std::shared_ptr<mygui::Label> channel_number;
        std::shared_ptr<mygui::Label> label_min;
        std::shared_ptr<mygui::Label> label_max;
        std::shared_ptr<mygui::Label> label_avg;
        std::shared_ptr<mygui::Label> label_std_dev;
        std::shared_ptr<mygui::Label> label_last;
        std::shared_ptr<mygui::Label> label_cnt;
    };

    std::array<InfoLabel, N_CHANNELS> infolabels_chs;
    InfoLabel                         infolabel_all;
};