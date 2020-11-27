#include "InfoWindow.hpp"
#include <fstream>

InfoWindow::InfoWindow(std::string const& title, std::string const& save_filename) :
    Window(450, 360, title, sf::Style::None | sf::Style::Close), m_save_filename(save_filename)
{
    AlwaysOnTop(true);
    MakeTransparent();
    SetTransparency(150);

    button_clear = std::make_shared<mygui::Button>(60, 10, "Clear", 80, 25);
    button_clear->OnClick(std::bind(&InfoWindow::Clear, this));
    Add(button_clear);
    button_save = std::make_shared<mygui::Button>(300, 10, "Save", 80, 25);
    button_save->OnClick(std::bind(&InfoWindow::SaveRecord, this));
    Add(button_save);

    label_info_win_to_det_min = std::make_shared<mygui::Label>(80, 50, "Min");
    Add(label_info_win_to_det_min);
    label_info_win_to_det_max = std::make_shared<mygui::Label>(140, 50, "Max:");
    Add(label_info_win_to_det_max);
    label_info_win_to_det_avg = std::make_shared<mygui::Label>(200, 50, "Avg:");
    Add(label_info_win_to_det_avg);
    label_info_win_to_det_std_dev = std::make_shared<mygui::Label>(260, 50, "Stdev:");
    Add(label_info_win_to_det_std_dev);
    label_info_win_to_det_last = std::make_shared<mygui::Label>(320, 50, "Last:");
    Add(label_info_win_to_det_last);
    label_info_win_to_det_cnt = std::make_shared<mygui::Label>(380, 50, "N: ");
    Add(label_info_win_to_det_cnt);

    for (int i = 0; i < N_CHANNELS; ++i) {
        infolabels_chs[i].channel_number = std::make_shared<mygui::Label>(20, 80 + i * 30, ("Ch " + std::to_string(i + 1)).c_str());
        Add(infolabels_chs[i].channel_number);
        infolabels_chs[i].label_min = std::make_shared<mygui::Label>(80, 80 + i * 30, ("N/A"));
        Add(infolabels_chs[i].label_min);
        infolabels_chs[i].label_max = std::make_shared<mygui::Label>(140, 80 + i * 30, ("N/A"));
        Add(infolabels_chs[i].label_max);
        infolabels_chs[i].label_avg = std::make_shared<mygui::Label>(200, 80 + i * 30, ("N/A"));
        Add(infolabels_chs[i].label_avg);
        infolabels_chs[i].label_std_dev = std::make_shared<mygui::Label>(260, 80 + i * 30, ("N/A"));
        Add(infolabels_chs[i].label_std_dev);
        infolabels_chs[i].label_last = std::make_shared<mygui::Label>(320, 80 + i * 30, ("N/A"));
        Add(infolabels_chs[i].label_last);
        infolabels_chs[i].label_cnt = std::make_shared<mygui::Label>(380, 80 + i * 30, ("N/A"));
        Add(infolabels_chs[i].label_cnt);
    }

    infolabel_all.channel_number = std::make_shared<mygui::Label>(20, 80 + 8 * 30, "All");
    Add(infolabel_all.channel_number);
    infolabel_all.label_min = std::make_shared<mygui::Label>(80, 80 + 8 * 30, ("N/A"));
    Add(infolabel_all.label_min);
    infolabel_all.label_max = std::make_shared<mygui::Label>(140, 80 + 8 * 30, ("N/A"));
    Add(infolabel_all.label_max);
    infolabel_all.label_avg = std::make_shared<mygui::Label>(200, 80 + 8 * 30, ("N/A"));
    Add(infolabel_all.label_avg);
    infolabel_all.label_std_dev = std::make_shared<mygui::Label>(260, 80 + 8 * 30, ("N/A"));
    Add(infolabel_all.label_std_dev);
    infolabel_all.label_last = std::make_shared<mygui::Label>(320, 80 + 8 * 30, ("N/A"));
    Add(infolabel_all.label_last);
    infolabel_all.label_cnt = std::make_shared<mygui::Label>(380, 80 + 8 * 30, ("N/A"));
    Add(infolabel_all.label_cnt);
}

void InfoWindow::Events()
{
    while (m_window->pollEvent(*m_event)) {
        if (m_event->type == sf::Event::Closed) {
            SetVisible(false); // don't close, only hide window
        }

        for (auto& w : m_widgets) {
            w->Handle(*m_event);
        }
    }
}

void InfoWindow::RefreshTable()
{
    std::optional<int64_t> min;
    int64_t                max = 0, avg = 0, stdev = 0, last = 0, cnt = 0;

    // Safety check
    if (!m_sample_freq_hz || *m_sample_freq_hz <= 0)
        return;

    for (int i = 0; i < N_CHANNELS; ++i) {
        // Min is special since at init it can't hold a sensible min value
        if (m_channel[i]->min) {
            infolabels_chs[i].label_min->SetText(std::to_string(m_channel[i]->min.value() * 1000 / *m_sample_freq_hz));

            if (!min || m_channel[i]->min.value() < min.value())
                min = m_channel[i]->min;
        } else
            infolabels_chs[i].label_min->SetText("N/A");

        infolabels_chs[i].label_max->SetText(std::to_string(m_channel[i]->max * 1000 / *m_sample_freq_hz));
        infolabels_chs[i].label_avg->SetText(std::to_string(m_channel[i]->avg * 1000 / *m_sample_freq_hz));
        infolabels_chs[i].label_std_dev->SetText(std::to_string(m_channel[i]->stdev * 1000 / *m_sample_freq_hz));
        infolabels_chs[i].label_last->SetText(std::to_string(m_channel[i]->last * 1000 / *m_sample_freq_hz));
        infolabels_chs[i].label_cnt->SetText(std::to_string(m_channel[i]->cnt));

        if (m_channel[i]->max > max)
            max = m_channel[i]->max;
        avg += m_channel[i]->avg * m_channel[i]->cnt;
        cnt += m_channel[i]->cnt;
    }
    if (cnt > 0) // avoid division by zero
        avg /= cnt;

    // Total
    if (min)
        infolabel_all.label_min->SetText(std::to_string(min.value() * 1000 / *m_sample_freq_hz));
    else
        infolabel_all.label_min->SetText("N/A");

    infolabel_all.label_max->SetText(std::to_string(max * 1000 / *m_sample_freq_hz));
    infolabel_all.label_avg->SetText(std::to_string(avg * 1000 / *m_sample_freq_hz));
    infolabel_all.label_cnt->SetText(std::to_string(cnt));
    // Not supported currently
    //infolabel_all.label_std_dev->SetText(std::to_string(stdev));
    //infolabel_all.label_last->SetText(std::to_string(last));
}

void InfoWindow::Clear()
{
    for (auto& c : m_channel) {
        c->Clear();
    }
    //m_all->Clear(); not currently supported
    RefreshTable();
}

void InfoWindow::SaveRecord()
{
    std::ofstream f(m_save_filename, std::ios::out | std::ios::app); // open for writting and append m_data
    std::string   buf;

    auto buf_correction = [&buf]() { if (buf[buf.size() - 1] == ',') buf.pop_back(); buf += "]\n"; };

    if (f.is_open()) {
        buf.clear();

        // Channels
        for (int i = 0; i < N_CHANNELS; ++i) {
            buf += "ch" + std::to_string(i + 1) + "=[";
            for (auto const& num : m_channel[i]->buffer) {
                buf += std::to_string(num) + ",";
            }
            buf_correction();
        }

        /* Not currently supported
		// Total (the way they came)
		buf += "total=[";
		size_t total_idx = m_channel.size() - 1;
		for (auto const& num : m_all->buffer) {
			buf += std::to_string(num) + ",";
		}
		buf_correction();
		*/

        // Total (sorted by channels)
        buf += "total_sorted_by_channel_id=[";
        for (int i = 0; i < N_CHANNELS; ++i) {
            for (auto const& num : m_channel[i]->buffer) {
                buf += std::to_string(num) + ",";
            }
        }
        buf_correction();

        f << buf; // Write entire content to file

        f.close();
        std::cout << "Analysis info saved to " << m_save_filename << std::endl;
    }
}
