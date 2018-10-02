#include "Signal.hpp"

Signal::Signal(int n, sf::Color col, const sf::FloatRect& region, float *max_val) :
	m_curve(sf::PrimitiveType::LineStrip, n),
	m_trigger_frame(sf::PrimitiveType::Lines, N_TRIGGER_FRAME_POINTS),
	m_draw_trigger_frame(false), m_graph_region(region), m_max_val(max_val) {

	for (int i = 0; i < n; ++i) {
		m_curve[i].color = col;
		m_curve[i].position.x = static_cast<float>(region.left + i * region.width / (n - 1));
	}

	// Trigger Frame		
	for (int i = 0; i < N_TRIGGER_FRAME_POINTS; ++i)
		m_trigger_frame[i].color = col;
}

void Signal::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	if (m_draw) {
		target.draw(m_curve);
		if (m_draw_trigger_frame)
			target.draw(m_trigger_frame);
	}
}

void Signal::SetThreashold(float threashold) {
	m_threashold_value = threashold;
}

void Signal::SetBlindTime(int blind_time_value) {
	m_blind_time_value = blind_time_value;
}

void Signal::EnableTriggerFrame() {
	m_draw_trigger_frame = true;
}

void Signal::DisableTriggerFrame() {
	m_draw_trigger_frame = false;
}

void Signal::EnableDraw() {
	m_draw = true;
}

void Signal::DisableDraw() {
	m_draw = false;
}

void Signal::OnlyDrawOnTrigger(bool on) {
	if (on)
		m_only_draw_on_trigger = true;
	else
		m_only_draw_on_trigger = false;
}

bool Signal::ThreasholdMissed() {
	if (m_threashold == Threashold::MISSED) {
		m_threashold = Threashold::IDLE;
		return true;
	}
	else {
		return false;
	}
}

int Signal::GetDetectionsInWindow() const {
	return m_detected_in_window_cnt;
}
void Signal::ClearDetectionsInWindow() {
	m_detected_in_window_cnt = 0;
}

int Signal::GetDetectionsOutWindow() const {
	return m_detected_out_window_cnt;
}
void Signal::ClearDetectionsOutWindow() {
	m_detected_out_window_cnt = 0;
}

int Signal::GetMissed() const {
	return m_detection_missed;
}
void Signal::ClearMissed() {
	m_detection_missed = 0;
}

// Return false if a signal never reached the threashold value when the window was on
void Signal::Edit(float* buf, int start, int size) {
	const float y_zero = m_graph_region.top + m_graph_region.height;
	const float y_high = y_zero - (m_threashold_value / *m_max_val) * m_graph_region.height + 1;

	if (m_draw_trigger_frame) {

		if (start == 0) {
			// Clear all frames
			for (int i = 0; i <= m_trigger_frame_idx; ++i) {
				m_trigger_frame[i].position = sf::Vector2f(0.f, 0.f);
			}
			m_trigger_frame_idx = 0;

			if (m_only_draw_on_trigger) DisableDraw();
			else EnableDraw();
		}

		for (int i = 0, s = start; i < size; ++i, ++s) {

			// Edge detection
			/////////////////
			if ((((uint32_t*)buf)[i] & 0x80000000) != 0) {	// trigger is active
				m_trigger_val = 1;
				((uint32_t*)buf)[i] -= 0x80000000;	// Correct buf by removing the encoding in the MSB
			}
			else {
				m_trigger_val = 0;
			}
			m_diff = m_trigger_val - m_trigger_val_prev;
			m_trigger_val_prev = m_trigger_val;
			/////////////////

			// Threashold detection
			///////////////////////
			m_blind_time -= (m_blind_time > 0);
			if (buf[i] >= m_threashold_value && m_blind_time <= 0) {
				m_threashold = Threashold::REACHED;
				m_blind_time = m_blind_time_value;
			}
			///////////////////////

			if (m_diff == 1) { // rising edge
				m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(m_curve[s].position.x, y_zero);
				m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(m_curve[s].position.x, y_high);
				m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(m_curve[s].position.x, y_high);

				if (m_only_draw_on_trigger) EnableDraw();
				m_threashold = Threashold::SEARCHING;
			}
			else if (m_diff == -1) { // falling edge
				m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(m_curve[s].position.x, y_high);
				m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(m_curve[s].position.x, y_high);
				m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(m_curve[s].position.x, y_zero);

				if (m_threashold == Threashold::SEARCHING) {
					m_threashold = Threashold::MISSED;
					m_detection_missed++;
				}
			}
			else if (m_trigger_val == 1) { // frame active
				if (s == 0) { // new start						
					m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(m_curve[0].position.x, y_high);
				}
				m_trigger_frame[m_trigger_frame_idx].position = sf::Vector2f(m_curve[s].position.x, y_high);

				if (m_threashold == Threashold::REACHED) {
					m_detected_in_window_cnt++;
					m_threashold = Threashold::IDLE;
				}
			}
			else { // no frame
				if (m_threashold == Threashold::REACHED) {
					m_detected_out_window_cnt++;
					m_threashold = Threashold::IDLE;
				}
			}
		}
	}

	for (int i = 0, s = start; i < size; ++i, ++s) {
		m_curve[s].position.y = y_zero - (buf[i] / *m_max_val) * m_graph_region.height + 1;
	}
}
