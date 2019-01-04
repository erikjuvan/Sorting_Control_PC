#include "InfoWindow.hpp"
#include <fstream>

InfoWindow::InfoWindow(char const* title, std::string const& save_filename) :
    Window(450, 360, title, sf::Style::None | sf::Style::Close), m_save_filename(save_filename)
{

    AlwaysOnTop(true);
    MakeTransparent();
    SetTransparency(150);

    button_clear = new mygui::Button(60, 10, "Clear", 80, 25);
    button_clear->OnClick(std::bind(&InfoWindow::Clear, this));
    Add(button_clear);
    button_save = new mygui::Button(300, 10, "Save", 80, 25);
    button_save->OnClick(std::bind(&InfoWindow::SaveRecord, this));
    Add(button_save);

    label_info_win_to_det_min = new mygui::Label(80, 50, "Min");
    Add(label_info_win_to_det_min);
    label_info_win_to_det_max = new mygui::Label(140, 50, "Max:");
    Add(label_info_win_to_det_max);
    label_info_win_to_det_avg = new mygui::Label(200, 50, "Avg:");
    Add(label_info_win_to_det_avg);
    label_info_win_to_det_std_dev = new mygui::Label(260, 50, "Stdev:");
    Add(label_info_win_to_det_std_dev);
    label_info_win_to_det_last = new mygui::Label(320, 50, "Last:");
    Add(label_info_win_to_det_last);
    label_info_win_to_det_cnt = new mygui::Label(380, 50, "N: ");
    Add(label_info_win_to_det_cnt);

    for (int i = 0; i < N_CHANNELS; ++i) {
        infolabels_chs[i].channel_number = new mygui::Label(20, 80 + i * 30, ("Ch " + std::to_string(i + 1)).c_str());
        Add(infolabels_chs[i].channel_number);
        infolabels_chs[i].label_min = new mygui::Label(80, 80 + i * 30, ("N/A"));
        Add(infolabels_chs[i].label_min);
        infolabels_chs[i].label_max = new mygui::Label(140, 80 + i * 30, ("N/A"));
        Add(infolabels_chs[i].label_max);
        infolabels_chs[i].label_avg = new mygui::Label(200, 80 + i * 30, ("N/A"));
        Add(infolabels_chs[i].label_avg);
        infolabels_chs[i].label_std_dev = new mygui::Label(260, 80 + i * 30, ("N/A"));
        Add(infolabels_chs[i].label_std_dev);
        infolabels_chs[i].label_last = new mygui::Label(320, 80 + i * 30, ("N/A"));
        Add(infolabels_chs[i].label_last);
        infolabels_chs[i].label_cnt = new mygui::Label(380, 80 + i * 30, ("N/A"));
        Add(infolabels_chs[i].label_cnt);
    }

    infolabel_all.channel_number = new mygui::Label(20, 80 + 8 * 30, "All");
    Add(infolabel_all.channel_number);
    infolabel_all.label_min = new mygui::Label(80, 80 + 8 * 30, ("N/A"));
    Add(infolabel_all.label_min);
    infolabel_all.label_max = new mygui::Label(140, 80 + 8 * 30, ("N/A"));
    Add(infolabel_all.label_max);
    infolabel_all.label_avg = new mygui::Label(200, 80 + 8 * 30, ("N/A"));
    Add(infolabel_all.label_avg);
    infolabel_all.label_std_dev = new mygui::Label(260, 80 + 8 * 30, ("N/A"));
    Add(infolabel_all.label_std_dev);
    infolabel_all.label_last = new mygui::Label(320, 80 + 8 * 30, ("N/A"));
    Add(infolabel_all.label_last);
    infolabel_all.label_cnt = new mygui::Label(380, 80 + 8 * 30, ("N/A"));
    Add(infolabel_all.label_cnt);
}

InfoWindow::~InfoWindow()
{
    delete button_clear;
    delete button_save;
    delete label_info_win_to_det_min;
    delete label_info_win_to_det_max;
    delete label_info_win_to_det_avg;
    delete label_info_win_to_det_std_dev;
    delete label_info_win_to_det_last;
    delete label_info_win_to_det_cnt;
}

InfoWindow::InfoLabel::~InfoLabel()
{
    delete channel_number;
    delete label_min;
    delete label_max;
    delete label_avg;
    delete label_std_dev;
    delete label_last;
    delete label_cnt;
}

void InfoWindow::RefreshTable()
{
    // Divide all by 10 to get miliseconds

    for (int i = 0; i < N_CHANNELS; ++i) {
        infolabels_chs[i].label_min->SetText(std::to_string(m_channel[i]->min / 10));
        infolabels_chs[i].label_max->SetText(std::to_string(m_channel[i]->max / 10));
        infolabels_chs[i].label_avg->SetText(std::to_string(m_channel[i]->avg / 10));
        infolabels_chs[i].label_std_dev->SetText(std::to_string(m_channel[i]->std_dev / 10));
        infolabels_chs[i].label_last->SetText(std::to_string(m_channel[i]->last / 10));
        infolabels_chs[i].label_cnt->SetText(std::to_string(m_channel[i]->cnt));
    }

    // Total
    infolabel_all.label_min->SetText(std::to_string(m_all->min / 10));
    infolabel_all.label_max->SetText(std::to_string(m_all->max / 10));
    infolabel_all.label_avg->SetText(std::to_string(m_all->avg / 10));
    infolabel_all.label_std_dev->SetText(std::to_string(m_all->std_dev / 10));
    infolabel_all.label_last->SetText(std::to_string(m_all->last / 10));
    infolabel_all.label_cnt->SetText(std::to_string(m_all->cnt));
}

void InfoWindow::Clear()
{
    for (auto& c : m_channel) {
        c->Clear();
    }
    m_all->Clear();
    RefreshTable();
}

void InfoWindow::SaveRecord()
{
    std::ofstream f(m_save_filename, std::ios::out | std::ios::app); // open for writting and append data
    std::string   buf;

    auto buf_correction = [&buf]() { if (buf[buf.size() - 1] == ',') buf.pop_back(); buf += "]\n"; };

    if (f.is_open()) {
        buf.clear();
        // Channels
        for (int i = 0; i < N_CHANNELS; ++i) {
            buf += "ch" + std::to_string(i + 1) + "=[";
            for (auto const& num : m_channel[i]->buffer) {
                buf += std::to_string(num / 10) + ",";
            }
            buf_correction();
        }

        // Total (the way they came)

        buf += "total=[";
        size_t total_idx = m_channel.size() - 1;
        for (auto const& num : m_all->buffer) {
            buf += std::to_string(num / 10) + ",";
        }
        buf_correction();

        // Total (sorted by channels)
        buf += "total_sorted_by_channel_id=[";
        for (int i = 0; i < N_CHANNELS; ++i) {
            for (auto const& num : m_channel[i]->buffer) {
                buf += std::to_string(num / 10) + ",";
            }
        }
        buf_correction();

        f << buf; // Write entire content to file

        f.close();
        std::cout << "Analysis info saved to " << m_save_filename << std::endl;
    }
}
