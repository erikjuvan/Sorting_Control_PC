#pragma once

#include <SFML\Graphics.hpp>
#include "Helpers.hpp"
#include <iomanip>
#include <sstream>

namespace gui {

class Object : public sf::Drawable {
public:
	virtual void Handle(const sf::Event& event) = 0;
};

class Button : public Object {
	typedef void(*fptr)();

public:
	Button(int x, int y, const char* text, int w = 90, int h = 30, int character_size = 20,
		const char* font_name = "arial.ttf") : m_idle_shape(sf::Vector2f(static_cast<float>(w), static_cast<float>(h))), 
		m_pressed_shape(m_idle_shape), m_pressed(false) {		

		m_idle_shape.setPosition(static_cast<float>(x), static_cast<float>(y));
		m_idle_shape.setFillColor(m_idle_color);
		m_idle_shape.setOutlineColor(sf::Color::Black);
		m_idle_shape.setOutlineThickness(1.f);

		m_pressed_shape.setPosition(static_cast<float>(x), static_cast<float>(y));
		m_pressed_shape.setFillColor(sf::Color(200, 230, 235));
		m_pressed_shape.setOutlineColor(sf::Color::Black);
		m_pressed_shape.setOutlineThickness(1.f);

		// mouseover not yet implemented
		m_mouseover_shape.setPosition(static_cast<float>(x), static_cast<float>(y));
		m_mouseover_shape.setFillColor(sf::Color(215, 240, 245));
		m_mouseover_shape.setOutlineColor(sf::Color::Black);
		m_mouseover_shape.setOutlineThickness(1.f);		

		m_active_shape = &m_idle_shape;

		m_font.loadFromFile(font_name);
		m_text.setFont(m_font);		
		m_text.setCharacterSize(character_size);		
		m_text.setFillColor(sf::Color::Black);
		SetText(text);
	}

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
		target.draw(*m_active_shape, states);
		target.draw(m_text, states);
	}

	void SetText(const std::string& text) {
		m_text.setString(text);		

		float tx = m_text.getLocalBounds().left;
		float tw = m_text.getLocalBounds().width;
		float ty = m_text.getLocalBounds().top;
		float th = m_text.getLocalBounds().height;

		float x = m_idle_shape.getGlobalBounds().left;
		float w = m_idle_shape.getGlobalBounds().width;
		float y = m_idle_shape.getGlobalBounds().top;
		float h = m_idle_shape.getGlobalBounds().height;

		m_text.setOrigin(tx + tw / 2.0f, ty + th / 2.0f);
		m_text.setPosition(x + w / 2.0f, y + h / 2.0f);
	}

	sf::FloatRect GetGlobalBounds() {
		return m_idle_shape.getGlobalBounds();
	}

	void SetColor(const sf::Color& col) {
		m_idle_shape.setFillColor(col);
	}

	void ResetColor() {
		m_idle_shape.setFillColor(m_idle_color);
	}

	virtual void Handle(const sf::Event& event) override {

		if (m_active_shape->getGlobalBounds().contains(sf::Vector2f(event.mouseButton.x, event.mouseButton.y))) {
			if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
				m_active_shape = &m_pressed_shape;
				m_pressed = true;
			}
			else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
				m_active_shape = &m_idle_shape;
				if (m_onClick && m_pressed) m_onClick();
				m_pressed = false;
			}
		} else {
			if (m_active_shape == &m_pressed_shape && event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
				m_active_shape = &m_idle_shape;
				m_pressed = false;
			}
		}
	}

	// Actions
	void OnClick(const fptr& f) {
		m_onClick = f;
	}

private:
	const sf::Color m_idle_color{ sf::Color(200, 200, 200) };
	sf::RectangleShape m_idle_shape;
	sf::RectangleShape m_pressed_shape;
	sf::RectangleShape m_mouseover_shape;
	sf::RectangleShape* m_active_shape;
	sf::Text m_text;
	sf::Font m_font;
	bool m_pressed{ false };

	fptr m_onClick{ nullptr };
};

class Label : public Object {
public:
	Label(int x, int y, const char* text, int character_size = 18,
		const char* font_name = "arial.ttf") {

		m_font.loadFromFile(font_name);
		m_text.setFont(m_font);
		m_text.setString(text);
		m_text.setCharacterSize(character_size);		
		m_text.setPosition(x, y);
		m_text.setFillColor(sf::Color::Black);				
	}

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
		target.draw(m_text);
	}

	virtual void Handle(const sf::Event& event) {
	}

	void SetText(const std::string& text) {
		m_text.setString(text);
	}

private:
	sf::Text m_text;
	sf::Font m_font;	
};

class Textbox : public Object {
	typedef void(*fptr)();
public:
	Textbox(int x, int y, const std::string& text = "", int w = 90, int h = 30, int character_size = 18,
		const char* font_name = "arial.ttf") : m_rect(sf::Vector2f(static_cast<float>(w), static_cast<float>(h))) {

		m_rect.setPosition(static_cast<float>(x), static_cast<float>(y));
		m_rect.setFillColor(sf::Color::White);
		m_rect.setOutlineColor(sf::Color::Black);
		m_rect.setOutlineThickness(1.f);

		m_font.loadFromFile(font_name);
		m_text.setFont(m_font);
		m_text.setString(text);
		m_text.setCharacterSize(character_size);
		m_text.setFillColor(sf::Color::Black);

		float tx = m_text.getLocalBounds().left;
		float tw = m_text.getLocalBounds().width;
		float ty = m_text.getLocalBounds().top;
		float th = m_text.getLocalBounds().height;

		m_text.setOrigin(tx, ty + th / 2.0f);
		m_text.setPosition(x + m_margin, y + m_rect.getLocalBounds().height / 2.0f);
	}

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
		target.draw(m_rect);
		target.draw(m_text);
	}

	virtual void Handle(const sf::Event& event) override {
		if (event.type == sf::Event::TextEntered && m_mouseover) {

			std::string str = m_text.getString();

			if (event.text.unicode == '\b') {
				if (str.length() > 0) str.pop_back();
			}				
			else {
				str += event.text.unicode;
			}
			
			// temporary text to see if the added character is too long to fit into the textbox
			sf::Text txt(m_text); 
			txt.setString(str);

			if (txt.getGlobalBounds().width < (m_rect.getGlobalBounds().width - m_margin))
				m_text.setString(str);
		}
		else if (event.type == sf::Event::MouseMoved) {
			if (m_rect.getGlobalBounds().contains(sf::Vector2f(event.mouseMove.x, event.mouseMove.y))) {
				m_mouseover = true;
			}
			else {
				m_mouseover = false;
			}
		}		
	}

	void SetText(const std::string& text) {
		m_text.setString(text);
	}

	std::string GetText() const {
		return m_text.getString();
	}

	// Actions
	void onKeyPress(const fptr& f) {
		m_keyPress = f;
	}

private:
	static constexpr int m_margin{ 10 };

	sf::RectangleShape m_rect;	
	sf::Text m_text;
	sf::Font m_font;
	bool m_mouseover{ false };
	fptr m_keyPress{ nullptr };
};

class Checkbox : public Object {
	typedef void(*fptr)();
public:
	Checkbox(int x, int y, const char* text = "", int w = 17, int h = 17, int character_size = 18,
		const char* font_name = "arial.ttf") : m_rect_checked(sf::Vector2f(static_cast<float>(w), static_cast<float>(h))),
		m_rect_unchecked(m_rect_checked), m_checked(false) {


		m_rect_checked.setPosition(static_cast<float>(x), static_cast<float>(y));
		m_rect_checked.setFillColor(sf::Color::Black);
		m_rect_checked.setOutlineColor(sf::Color::Black);
		m_rect_checked.setOutlineThickness(1.f);

		m_rect_unchecked.setPosition(static_cast<float>(x), static_cast<float>(y));
		m_rect_unchecked.setFillColor(sf::Color::White);
		m_rect_unchecked.setOutlineColor(sf::Color::Black);
		m_rect_unchecked.setOutlineThickness(1.f);

		m_rect = &m_rect_unchecked;

		m_font.loadFromFile(font_name);
		m_text.setFont(m_font);
		m_text.setString(text);
		m_text.setCharacterSize(character_size);
		sf::FloatRect text_pos = m_text.getGlobalBounds();
		m_text.setPosition(x + w + m_margin / 2.f, y - 2.f);
		m_text.setFillColor(sf::Color::Black);
	}

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
		target.draw(*m_rect);
		target.draw(m_text);
	}

	virtual void Handle(const sf::Event& event) override {		
		if (m_rect->getGlobalBounds().contains(sf::Vector2f(event.mouseButton.x, event.mouseButton.y))) {
			if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
				m_pressed_in_focus = true;
			} else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
				if (m_pressed_in_focus) {
					if (!m_checked) {
						m_rect = &m_rect_checked;
						m_checked = true;
					} else {
						m_rect = &m_rect_unchecked;
						m_checked = false;
					}

					if (m_onClick != nullptr)
						m_onClick();

					m_pressed_in_focus = false;
				}				
			}
		} else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
			m_pressed_in_focus = false;
		}
	}

	// Actions
	void OnClick(const fptr& f) {
		m_onClick = f;
	}

	bool IsChecked() {
		return m_checked;
	}
	

private:	
	static constexpr int m_margin{ 20 };

	sf::RectangleShape m_rect_checked;
	sf::RectangleShape m_rect_unchecked;
	sf::RectangleShape* m_rect;
	bool m_checked;
	sf::Text m_text;
	sf::Font m_font;
	bool m_pressed_in_focus{ false };

	fptr m_onClick{ nullptr };
};

class Signal : public sf::Drawable {

public:
	Signal(int n, sf::Color col, const sf::FloatRect& region, float *max_val) :
		m_curve(sf::PrimitiveType::LineStrip, n),
		m_trigger_frame(sf::PrimitiveType::LineStrip, N_TRIGGER_FRAME_POINTS),
		m_points(n), m_draw_trigger_frame(false), m_region(region), m_max_val(max_val) {	

		for (int i = 0; i < n; ++i) {
			m_curve[i].color = col;
			m_curve[i].position.x = static_cast<float>(region.left + i * region.width / (n - 1));
			m_points[i] = 0.f;
		}

		// Trigger Frame		
		for (int i = 0; i < N_TRIGGER_FRAME_POINTS; ++i)
			m_trigger_frame[i].color = col;
	}

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
		target.draw(m_curve);
		if (m_draw_trigger_frame)
			target.draw(m_trigger_frame);
	}

	void SetTriggerFrame(float threashold) {
		m_threashold_value = threashold;
		UpdateTriggerFrame();
	}

	void UpdateTriggerFrame() {
		const float y_low = m_region.top + m_region.height;
		const float y_high = y_low - (m_threashold_value / *m_max_val) * m_region.height + 1;

		m_trigger_frame[0].position.y = y_low;
		m_trigger_frame[1].position.y = y_high;
		m_trigger_frame[2].position.y = y_high;
		m_trigger_frame[3].position.y = y_low;
	}

	void EnableTriggerFrame() {
		m_draw_trigger_frame = true;
	}
	
	void DisableTriggerFrame() {
		m_draw_trigger_frame = false;
	}

	// Return false if a signal never reached the threashold value when the window was on
	bool Edit(float* buf, int start, int size) {
		bool ret = true;

		if (m_draw_trigger_frame) {

			// Clear frame at start
			if (start == 0) {
				if (!m_frame_found) {
					for (int i = 0; i < N_TRIGGER_FRAME_POINTS; ++i)
						m_trigger_frame[i].position.x = m_region.left;
				}
			}
			
			for (int i = 0, s = start; i < size; ++i, ++s) {
				if ( (((uint32_t*)buf)[i] & 0x80000000) != 0) {	// trigger is active
					if (!m_frame_found) {
						m_trigger_frame[0].position.x = m_trigger_frame[1].position.x =
							m_trigger_frame[2].position.x = m_trigger_frame[3].position.x = m_curve[s].position.x;

						m_frame_found = true;
						m_threashold = Threashold::SEARCHING;
					}
					else {
						m_trigger_frame[2].position.x = m_trigger_frame[3].position.x = m_curve[s].position.x;
					}
					((uint32_t*)buf)[i] -= 0x80000000;
					if (buf[i] >= m_threashold_value)
						m_threashold = Threashold::REACHED;
				}
				else if (m_frame_found) {
					m_frame_found = false;
					if (m_threashold == Threashold::SEARCHING) {
						m_threashold = Threashold::MISSED;
						ret = false;
					}
				}
			}
		}

		const float y_zero = m_region.top + m_region.height;
		for (int i = 0, s = start; i < size; ++i, ++s) {
			m_curve[s].position.y = y_zero - (buf[i] / *m_max_val) * m_region.height + 1;
		}

		return ret;
	}

private:
	static constexpr int N_TRIGGER_FRAME_POINTS = 4;

	sf::VertexArray m_curve;
	sf::VertexArray m_trigger_frame;
	std::vector<float> m_points;
	sf::FloatRect m_region;

	float *m_max_val;
	float m_threashold_value;
	bool m_draw_trigger_frame{ false };
	bool m_frame_found{ false };

	float m_prev_buf[100];

	enum class Threashold {IDLE, REACHED, MISSED, SEARCHING};
	Threashold m_threashold;
};

class Chart : public Object {
public:

	Chart(int x, int y, int w, int h, int num_of_points, float max_val, const std::string& font_name = "arial.ttf") :
		m_num_of_points(num_of_points),	m_max_val(max_val), m_background(sf::Vector2f(w, h)),
		m_chart_region(sf::Vector2f(w - 6 * m_margin, h - 5 * m_margin)) {

		m_font.loadFromFile(font_name);		

		m_background.setPosition(x, y);
		m_background.setOutlineColor(sf::Color::Black);
		m_background.setOutlineThickness(1.f);

		m_chart_region.setPosition(x + 4.f * m_margin, y + 2.f * m_margin);
		m_chart_region.setOutlineColor(sf::Color::Black);
		m_chart_region.setOutlineThickness(1.f);
		m_chart_rect = m_chart_region.getGlobalBounds();

		m_title.setFont(m_font);

		m_title.setFillColor(sf::Color::Black);
		m_title.setString("Sorting control");
		m_title.setPosition(sf::Vector2f(x + w / 2.f - m_title.getLocalBounds().width / 2.f, y));
		
		m_x_axis.setFont(m_font);		
		m_x_axis.setFillColor(sf::Color::Black);
		m_x_axis.setCharacterSize(24);
		m_x_axis.setString("Sample");
		m_x_axis.setPosition(sf::Vector2f(x + w / 2.f - m_x_axis.getLocalBounds().width / 2.f, h - 1.25f  * m_margin));	

		m_y_axis.setFont(m_font);		
		m_y_axis.setFillColor(sf::Color::Black);
		m_y_axis.setCharacterSize(24);
		m_y_axis.setRotation(-90.f);
		m_y_axis.setString("ADC value");
		m_y_axis.setPosition(sf::Vector2f(x + m_margin/4.f, y + h / 2.f + m_y_axis.getLocalBounds().width / 2.f));			
	}

	~Chart() {}

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
		target.draw(m_background, states);
		target.draw(m_chart_region, states);
		target.draw(m_x_axis);
		target.draw(m_y_axis);
		target.draw(m_title);
		target.draw(m_grid);
		for (auto& m : m_x_axis_markers)
			target.draw(m);
		for (auto& m : m_y_axis_markers)
			target.draw(m);
		for (int i = 0; i < m_signals.size(); ++i)
			target.draw(*m_signals[i]);
	}

	virtual void Handle(const sf::Event& event) {
		//  && m_chart_region.getGlobalBounds().contains(sf::Vector2f(event.mouseButton.x, event.mouseButton.y))
		if (event.type == sf::Event::MouseWheelScrolled && m_mouseover) {
			if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {				
				if (m_max_val >= 100.f) {
					m_max_val -= event.mouseWheelScroll.delta * 50.f;
				}
				else if (m_max_val > 5.f) {
					m_max_val -= event.mouseWheelScroll.delta * 5.f;
				}
				else {
					if (event.mouseWheelScroll.delta < 0.f)
						m_max_val -= event.mouseWheelScroll.delta * 5.f;
				}

				CreateAxisMarkers();
				UpdateTriggerFrame();
			}			
		}
		else if (event.type == sf::Event::MouseMoved) {
			if (m_chart_region.getGlobalBounds().contains(sf::Vector2f(event.mouseMove.x, event.mouseMove.y))) {
				m_mouseover = true;
			}
			else {
				m_mouseover = false;
			}
		}
	}

	void AddSignal(Signal* signal) {
		m_signals.push_back(signal);
	}

	// n_lines - number of one type of lines (vertical or horizontal), there are same number of other lines
	void CreateGrid(int n_lines) {
		m_num_grid_lines = n_lines;
		m_grid = sf::VertexArray(sf::PrimitiveType::Lines, n_lines * 4);
		const sf::Color color = sf::Color(100, 100, 100, 65);

		// vertical lines
		for (int i = 0; i < n_lines; ++i) {
			m_grid[2 * i].position = sf::Vector2f(m_chart_rect.left + (i+1) * (m_chart_rect.width / (n_lines + 1)), m_chart_rect.top);
			m_grid[2 * i].color = color;
			m_grid[2 * i + 1].position = sf::Vector2f(m_chart_rect.left + (i+1) * (m_chart_rect.width / (n_lines + 1)), m_chart_rect.top + m_chart_rect.height);
			m_grid[2 * i + 1].color = color;
		}

		// horizontal lines
		for (int i = n_lines, j = 0; i < n_lines * 2; ++i, ++j) {
			m_grid[2 * i].position = sf::Vector2f(m_chart_rect.left, m_chart_rect.top + ((j + 1) * (m_chart_rect.height / (n_lines + 1))));
			m_grid[2 * i].color = color;
			m_grid[2 * i + 1].position = sf::Vector2f(m_chart_rect.left + m_chart_rect.width, m_chart_rect.top + ((j + 1) * (m_chart_rect.height / (n_lines + 1))));
			m_grid[2 * i + 1].color = color;
		}

		CreateAxisMarkers();
	}

	void CreateAxisMarkers() {
		sf::FloatRect rect = m_chart_region.getGlobalBounds();
		int n = m_num_grid_lines + 2;	// + 2 is for 0 and max

		m_x_axis_markers.clear();
		m_y_axis_markers.clear();

		// X
		m_x_axis_markers.reserve(n);
		for (int i = 0; i < n; ++i) {
			m_x_axis_markers.push_back(sf::Text());
			auto& marker = m_x_axis_markers[m_x_axis_markers.size() - 1];
			marker.setFont(m_font);
			marker.setFillColor(sf::Color::Black);
			marker.setCharacterSize(18);
			int tmp = i * m_num_of_points / (n - 1);
			marker.setString(std::to_string(tmp));
			marker.setOrigin(marker.getLocalBounds().left + marker.getLocalBounds().width / 2.f,
				marker.getLocalBounds().top + marker.getLocalBounds().height / 2.f);
			marker.setPosition(rect.left + i * rect.width / (n-1), rect.top + rect.height + marker.getLocalBounds().height);
		}		

		// Y
		m_y_axis_markers.reserve(n);
		for (int i = 0; i < n; ++i) {
			m_y_axis_markers.push_back(sf::Text());
			auto& marker = m_y_axis_markers[m_y_axis_markers.size() - 1];
			marker.setFont(m_font);
			marker.setFillColor(sf::Color::Black);
			marker.setCharacterSize(18);
			float tmp = i * m_max_val / (n - 1);
			std::stringstream ss;
			ss << std::fixed << std::setprecision(1) << tmp;
			marker.setString(ss.str());			
			marker.setOrigin(marker.getLocalBounds().left + marker.getLocalBounds().width / 2.f,
				marker.getLocalBounds().top + marker.getLocalBounds().height / 2.f);
			marker.setPosition(rect.left - marker.getLocalBounds().width / 2 - 3, rect.top + rect.height - i * rect.height / (n-1));
		}
	}

	const sf::FloatRect GetGraphRegion() {
		return m_chart_rect;
	}

	float* GetMaxVal() {
		return &m_max_val;
	}

	void UpdateTriggerFrame() {
		for (const auto& s : m_signals)
			s->UpdateTriggerFrame();
	}

	void SetTriggerFrame(float threashold) {
		for (const auto& s : m_signals)
			s->SetTriggerFrame(threashold);
	}

	void EnableTriggerFrame() {
		for (const auto& s : m_signals)
			s->EnableTriggerFrame();
	}

	void DisableTriggerFrame() {
		for (const auto& s : m_signals)
			s->DisableTriggerFrame();
	}

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
	float				m_max_val;
	int					m_num_of_points;
	
	bool	m_mouseover;
};

}
