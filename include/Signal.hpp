#pragma once

#include <SFML/Graphics.hpp>

class Signal : public sf::Drawable {
private:
	enum class Threashold { IDLE, REACHED, MISSED, SEARCHING };	

public:	
	enum Event { NONE = 0x0, DETECTED_IN = 0x1, DETECTED_OUT = 0x2, MISSED = 0x4 };	

	Signal();
	Signal(int n, sf::Color col, const sf::FloatRect& region, float *max_val);

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

	void SetThreashold(float threashold);
	void SetBlindTime(int blind_time_value);
	void EnableTriggerFrame();
	void DisableTriggerFrame();
	void EnableDraw();
	void DisableDraw();
	void OnlyDrawOnTrigger(bool on);
	int  GetDetectionsInWindow() const;
	void ClearDetectionsInWindow();
	int  GetDetectionsOutWindow() const;
	void ClearDetectionsOutWindow();
	int  GetMissed() const;
	void ClearMissed();
	void SetColor(sf::Color const& col);
	static void EventsToRecord(Event const events);
	static Event EventsToRecord();
	bool AnyEvents() const;
	void ClearEvents();
	// Return false if a signal never reached the threashold value when the window was on
	void Edit(float* buf, int start, int size);

private:	

	static constexpr int N_TRIGGER_FRAME_POINTS = 60;	// should be enough for ~ 60 / 3 = 20 windows		

	inline static Event m_events_to_record;

	Threashold m_threashold;
	Event m_events;	

	sf::VertexArray	m_curve;
	sf::VertexArray	m_trigger_frame;
	int m_trigger_frame_idx{ 0 };
	sf::FloatRect	m_graph_region;

	bool m_draw{ true };
	bool m_only_draw_on_trigger{ false };

	float *m_max_val;
	float m_threashold_value;
	bool m_draw_trigger_frame{ false };	

	int m_diff{ 0 };
	int m_trigger_val{ 0 };
	int m_trigger_val_prev{ 0 };

	int m_detected_in_window_cnt{ 0 };
	int m_detected_out_window_cnt{ 0 };
	int m_detection_missed{ 0 };
	int m_blind_time{ 0 };
	int m_blind_time_value;
};
