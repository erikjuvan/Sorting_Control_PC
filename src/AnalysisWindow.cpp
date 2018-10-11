#include "AnalysisWindow.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

extern AnalysisWindow* g_analysisWindow;

void SortingAnalysis::ClearAll()
{
    for (auto& c : channel) {
        c.Clear();
    }
    total.Clear();
}

void SortingAnalysis::Add(uint32_t* data, int size)
{
    for (int i = 0; i < size; ++i) {
        int val = data[i] & 0x00FFFFFF;
        if (val > 0) {
            int ch = ((data[i] >> 24) & 0xFF);

            channel[ch].Update(val);
            total.Update(val);

            if (m_record) {
                channel[ch].push_back(val);
                total.push_back(val);
            }
        }
    }
}

void SortingAnalysis::SaveRecord(char const* fname)
{
    std::ofstream f(fname, std::ios::out | std::ios::app); // open for writting and append data
    std::string   buf;

    auto buf_correction = [&buf]() { if (buf[buf.size() - 1] == ',') buf.pop_back(); buf += "]\n"; };

    if (f.is_open()) {
        buf.clear();
        // Channels
        for (int i = 0; i < N_CHANNELS; ++i) {
            buf += "ch" + std::to_string(i + 1) + "=[";
            for (auto const& num : channel[i].buffer) {
                buf += std::to_string(num) + ",";
            }
            buf_correction();
        }

        // Total (the way they came)
        buf += "total=[";
        for (auto const& num : total.buffer) {
            buf += std::to_string(num) + ",";
        }
        buf_correction();

        // Total (sorted by channels)
        buf += "total_sorted_by_channel_id=[";
        for (int i = 0; i < N_CHANNELS; ++i) {
            for (auto const& num : channel[i].buffer) {
                buf += std::to_string(num) + ",";
            }
        }
        buf_correction();

        f << buf; // Write entire content to file

        f.close();
        std::cout << "Analysis info saved to " << fname << std::endl;
    }
}

// Callback
///////////
void AnalysisWindow::button_clear_all_Clicked()
{
    g_analysisWindow->m_analysis->ClearAll();
    g_analysisWindow->NewData(nullptr, 0); // A call to update label text
}

void AnalysisWindow::button_record_Clicked()
{
    if (!g_analysisWindow->m_analysis->m_record) {
        g_analysisWindow->m_analysis->m_record = true;
        g_analysisWindow->button_record->SetColor(sf::Color::Red);
    } else {
        g_analysisWindow->m_analysis->m_record = false;
        g_analysisWindow->button_record->ResetColor();
    }
}

void AnalysisWindow::button_save_Clicked()
{
    g_analysisWindow->m_analysis->SaveRecord("info.txt");
}
///////////

void AnalysisWindow::NewData(uint32_t* data, int size)
{
    m_analysis->Add(data, size);

    for (int i = 0; i < N_CHANNELS; ++i) {
        labels[i].label_min->SetText(std::to_string(m_analysis->channel[i].min));
        labels[i].label_max->SetText(std::to_string(m_analysis->channel[i].max));
        labels[i].label_avg->SetText(std::to_string(m_analysis->channel[i].avg));
        labels[i].label_std_dev->SetText(std::to_string(m_analysis->channel[i].std_dev));
        labels[i].label_last->SetText(std::to_string(m_analysis->channel[i].last));
        labels[i].label_cnt->SetText(std::to_string(m_analysis->channel[i].cnt));
    }
    label_all.label_min->SetText(std::to_string(m_analysis->total.min));
    label_all.label_max->SetText(std::to_string(m_analysis->total.max));
    label_all.label_avg->SetText(std::to_string(m_analysis->total.avg));
    label_all.label_std_dev->SetText(std::to_string(m_analysis->total.std_dev));
    // Last value for all tracks is undefined
    label_all.label_cnt->SetText(std::to_string(m_analysis->total.cnt));
}

AnalysisWindow::AnalysisWindow(char const* title) :
    Window(450, 360, title, sf::Style::None | sf::Style::Close)
{
    m_analysis = new SortingAnalysis;

    button_clear_all = new mygui::Button(60, 10, "Clear", 80, 25);
    button_clear_all->OnClick(&AnalysisWindow::button_clear_all_Clicked);
    Add(button_clear_all);
    button_record = new mygui::Button(180, 10, "Record", 80, 25);
    button_record->OnClick(&AnalysisWindow::button_record_Clicked);
    Add(button_record);
    button_save = new mygui::Button(300, 10, "Save", 80, 25);
    button_save->OnClick(&AnalysisWindow::button_save_Clicked);
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
        labels[i].channel_number = new mygui::Label(20, 80 + i * 30, ("Ch " + std::to_string(i + 1)).c_str());
        Add(labels[i].channel_number);
        labels[i].label_min = new mygui::Label(80, 80 + i * 30, ("N/A"));
        Add(labels[i].label_min);
        labels[i].label_max = new mygui::Label(140, 80 + i * 30, ("N/A"));
        Add(labels[i].label_max);
        labels[i].label_avg = new mygui::Label(200, 80 + i * 30, ("N/A"));
        Add(labels[i].label_avg);
        labels[i].label_std_dev = new mygui::Label(260, 80 + i * 30, ("N/A"));
        Add(labels[i].label_std_dev);
        labels[i].label_last = new mygui::Label(320, 80 + i * 30, ("N/A"));
        Add(labels[i].label_last);
        labels[i].label_cnt = new mygui::Label(380, 80 + i * 30, ("N/A"));
        Add(labels[i].label_cnt);
    }

    label_all.channel_number = new mygui::Label(20, 80 + 8 * 30, "All");
    Add(label_all.channel_number);
    label_all.label_min = new mygui::Label(80, 80 + 8 * 30, ("N/A"));
    Add(label_all.label_min);
    label_all.label_max = new mygui::Label(140, 80 + 8 * 30, ("N/A"));
    Add(label_all.label_max);
    label_all.label_avg = new mygui::Label(200, 80 + 8 * 30, ("N/A"));
    Add(label_all.label_avg);
    label_all.label_std_dev = new mygui::Label(260, 80 + 8 * 30, ("N/A"));
    Add(label_all.label_std_dev);
    label_all.label_last = new mygui::Label(320, 80 + 8 * 30, ("N/A"));
    Add(label_all.label_last);
    label_all.label_cnt = new mygui::Label(380, 80 + 8 * 30, ("N/A"));
    Add(label_all.label_cnt);
}

AnalysisWindow::~AnalysisWindow()
{
    delete m_analysis;
    delete button_clear_all;
    delete label_info_win_to_det_min;
    delete label_info_win_to_det_max;
    delete label_info_win_to_det_avg;
    delete label_info_win_to_det_std_dev;
    delete label_info_win_to_det_last;
    delete label_info_win_to_det_cnt;
}

AnalysisWindow::InfoLabel::~InfoLabel()
{
    delete channel_number;
    delete label_min;
    delete label_max;
    delete label_avg;
    delete label_std_dev;
    delete label_last;
    delete label_cnt;
}