#pragma once

#include <mygui/Object.hpp>

class Signal : public sf::Drawable {

public:
	Signal(int n, sf::Color col, const sf::FloatRect& region, float *max_val);

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

	void SetThreashold(float threashold);
	void SetBlindTime(int blind_time_value);
	void EnableTriggerFrame();
	void DisableTriggerFrame();
	void EnableDraw();
	void DisableDraw();
	void OnlyDrawOnTrigger(bool on);
	bool ThreasholdMissed();
	int  GetDetectionsInWindow() const;
	void ClearDetectionsInWindow();
	int  GetDetectionsOutWindow() const;
	void ClearDetectionsOutWindow();
	int  GetMissed() const;
	void ClearMissed();

	// Return false if a signal never reached the threashold value when the window was on
	void Edit(float* buf, int start, int size);

private:
	enum class Threashold { IDLE, REACHED, MISSED, SEARCHING };

	static constexpr int N_TRIGGER_FRAME_POINTS = 60;	// should be enough for ~ 60 / 3 = 20 windows	

	sf::VertexArray	m_curve;
	sf::VertexArray	m_trigger_frame;
	int m_trigger_frame_idx{ 0 };
	sf::FloatRect	m_graph_region;

	bool m_draw{ true };
	bool m_only_draw_on_trigger{ false };

	float *m_max_val{ nullptr };
	float m_threashold_value;
	bool m_draw_trigger_frame{ false };
	
	Threashold m_threashold;

	int m_diff{ 0 };
	int m_trigger_val{ 0 };
	int m_trigger_val_prev{ 0 };

	int m_detected_in_window_cnt{ 0 };
	int m_detected_out_window_cnt{ 0 };
	int m_detection_missed{ 0 };
	int m_blind_time{ 0 };
	int m_blind_time_value;
};


class Chart : public mygui::Object {

	using fptr = void(*)(const sf::Event&);

public:

	Chart(int x, int y, int w, int h, int num_of_points, float max_val, const std::string& font_name = "arial.ttf");

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
	virtual void Handle(const sf::Event& event) override;

	void AddSignal(Signal* signal);
	void ChangeSignal(int idx, Signal* signal);

	// n_lines - number of one type of lines (vertical or horizontal), there are same number of other lines
	void CreateGrid(int n_lines);
	void CreateAxisMarkers();
	const sf::FloatRect GetGraphRegion();
	float* GetMaxVal();
	void EnableTriggerFrame();
	void DisableTriggerFrame();
	void ToggleDrawSignal(int idx);
	void ToggleDrawAllSignals();

	// Actions
	void OnKeyPress(const fptr& f);

private:
	static constexpr int m_margin{ 20 };

	sf::RectangleShape	m_background;
	sf::RectangleShape	m_chart_region;
	sf::FloatRect		m_chart_rect;
	sf::VertexArray		m_outline;
	sf::VertexArray		m_axes;
	sf::VertexArray		m_grid;
	int					m_num_grid_lines{ 0 };
	sf::Text			m_x_axis;
	sf::Text			m_y_axis;
	sf::Text			m_title;

	std::vector<sf::Text> m_x_axis_markers;
	std::vector<sf::Text> m_y_axis_markers;

	sf::Font			m_font;

	std::vector<Signal*> m_signals;
	std::vector<bool>	m_draw_signal;
	float				m_max_val;
	int					m_num_of_points;

	bool	m_mouseover;

	fptr m_onKeyPress{ nullptr };
};
