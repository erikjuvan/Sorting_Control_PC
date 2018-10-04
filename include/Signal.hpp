#pragma once

#include <SFML/Graphics.hpp>

class Signal : public sf::Drawable {

public:
	static bool GetError();

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
	void SetColor(sf::Color const& col);

	// Return false if a signal never reached the threashold value when the window was on
	void Edit(float* buf, int start, int size);

private:
	enum class Threashold { IDLE, REACHED, MISSED, SEARCHING };

	static constexpr int N_TRIGGER_FRAME_POINTS = 60;	// should be enough for ~ 60 / 3 = 20 windows	

	static bool error;

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
