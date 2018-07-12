#pragma once

#include <SFML\Graphics.hpp>
#include "Helpers.hpp"

namespace gui {

class Object : public sf::Drawable {
public:
	virtual void Handle(const sf::Event& event) = 0;
};

class Button : public Object {
	typedef void(*fptr)();

public:
	Button(int x, int y, const char* name, int w = 90, int h = 40, int character_size = 22,
		const char* font_name = "arial.ttf") : m_idle_shape(sf::Vector2f(static_cast<float>(w), static_cast<float>(h))), 
		m_pressed_shape(m_idle_shape), m_pressed(false) {

		m_idle_shape.setPosition(static_cast<float>(x), static_cast<float>(y));
		m_idle_shape.setFillColor(sf::Color(200, 200, 200));
		m_idle_shape.setOutlineColor(sf::Color::Black);
		m_idle_shape.setOutlineThickness(1.f);

		m_pressed_shape.setPosition(static_cast<float>(x), static_cast<float>(y));
		m_pressed_shape.setFillColor(sf::Color(200, 230, 235));
		m_pressed_shape.setOutlineColor(sf::Color::Black);
		m_pressed_shape.setOutlineThickness(1.f);

		m_active_shape = &m_idle_shape;

		m_font.loadFromFile(font_name);
		m_text.setFont(m_font);
		m_text.setString(name);
		m_text.setCharacterSize(character_size);
		sf::FloatRect text_pos = m_text.getGlobalBounds();
		m_text.setPosition(x  + w / 2 - text_pos.width / 2, y + h / 2 - text_pos.height);
		m_text.setFillColor(sf::Color::Black);
	}

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
		target.draw(*m_active_shape, states);
		target.draw(m_text, states);
	}

	void SetText(const std::string& text) {
		m_text.setString(text);
		sf::FloatRect text_pos = m_text.getGlobalBounds();
		float x = m_idle_shape.getGlobalBounds().left;
		float w = m_idle_shape.getGlobalBounds().width;
		float y = m_idle_shape.getGlobalBounds().top;
		float h = m_idle_shape.getGlobalBounds().height;
		m_text.setPosition(x + w / 2 - text_pos.width / 2, y + h / 2 - text_pos.height);
	}

	sf::FloatRect GetGlobalBounds() {
		return m_idle_shape.getGlobalBounds();
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
	sf::RectangleShape m_idle_shape;
	sf::RectangleShape m_pressed_shape;
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
	Textbox(int x, int y, const std::string& text = "", int w = 100, int h = 30, int character_size = 18,
		const char* font_name = "arial.ttf") : m_rect(sf::Vector2f(static_cast<float>(w), static_cast<float>(h))) {

		m_rect.setPosition(static_cast<float>(x), static_cast<float>(y));
		m_rect.setFillColor(sf::Color::White);
		m_rect.setOutlineColor(sf::Color::Black);
		m_rect.setOutlineThickness(1.f);

		m_font.loadFromFile(font_name);
		m_text.setFont(m_font);
		m_text.setString(text);
		m_text.setCharacterSize(character_size);
		sf::FloatRect text_pos = m_text.getGlobalBounds();
		m_text.setPosition(x + m_margin / 2, y+3);
		m_text.setFillColor(sf::Color::Black);
	}

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
		target.draw(m_rect);
		target.draw(m_text);
	}

	virtual void Handle(const sf::Event& event) override {
		static bool in_focus = false;
		if (event.type == sf::Event::TextEntered && in_focus) {

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
				in_focus = true;
			}
			else {
				in_focus = false;
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
		m_text.setPosition(x + w + m_margin / 2, y - 2);
		m_text.setFillColor(sf::Color::Black);
	}

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
		target.draw(*m_rect);
		target.draw(m_text);
	}

	virtual void Handle(const sf::Event& event) override {
		static bool pressed_in_focus = false;
		if (m_rect->getGlobalBounds().contains(sf::Vector2f(event.mouseButton.x, event.mouseButton.y))) {
			if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
				pressed_in_focus = true;
			} else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
				if (pressed_in_focus) {
					if (!m_checked) {
						m_rect = &m_rect_checked;
						m_checked = true;
					} else {
						m_rect = &m_rect_unchecked;
						m_checked = false;
					}

					if (m_onClick != nullptr)
						m_onClick();

					pressed_in_focus = false;
				}				
			}
		} else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
			pressed_in_focus = false;
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

	fptr m_onClick{ nullptr };
};


class Chart : public Object {
public:

	Chart(int x, int y, int w, int h, float max_val, const char* font_name = "arial.ttf") : 
		m_max_val(max_val), m_background(sf::Vector2f(w, h)),
		m_chart_region(sf::Vector2f(w - 6 * m_margin, h - 5 * m_margin)) {

		m_font.loadFromFile(font_name);		

		m_background.setPosition(x, y);
		m_background.setOutlineColor(sf::Color::Black);
		m_background.setOutlineThickness(1.f);

		m_chart_region.setPosition(x + 4 * m_margin, y + 2 * m_margin);
		m_chart_region.setOutlineColor(sf::Color::Black);
		m_chart_region.setOutlineThickness(1.f);
		m_chart_rect = m_chart_region.getGlobalBounds();

		m_title.setFont(m_font);

		m_title.setFillColor(sf::Color::Black);
		m_title.setString("Sorting control");
		m_title.setPosition(sf::Vector2f(x + w / 2 - m_title.getLocalBounds().width / 2, y));
		
		m_x_axis.setFont(m_font);		
		m_x_axis.setFillColor(sf::Color::Black);
		m_x_axis.setCharacterSize(24);
		m_x_axis.setString("Sample");
		m_x_axis.setPosition(sf::Vector2f(x + w / 2 - m_x_axis.getLocalBounds().width / 2, h - 1.25  * m_margin));	

		m_y_axis.setFont(m_font);		
		m_y_axis.setFillColor(sf::Color::Black);
		m_y_axis.setCharacterSize(24);
		m_y_axis.setRotation(-90.f);
		m_y_axis.setString("ADC value");
		m_y_axis.setPosition(sf::Vector2f(x + m_margin/4, y + h / 2 + m_y_axis.getLocalBounds().width / 2));
	}

	~Chart() {}

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
		target.draw(m_background, states);
		target.draw(m_chart_region, states);
		target.draw(m_x_axis);
		target.draw(m_y_axis);
		target.draw(m_title);
		target.draw(m_grid);
		for (const auto& curve : m_curves)
			target.draw(curve);
	}

	virtual void Handle(const sf::Event& event) {
		//  && m_chart_region.getGlobalBounds().contains(sf::Vector2f(event.mouseButton.x, event.mouseButton.y))
		if (event.type == sf::Event::MouseWheelScrolled) {
			if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {

				m_max_val -= event.mouseWheelScroll.delta * 50;

				std::cout << m_max_val << std::endl;
			}
		}
	}

	void AddCurve(int nPoints, const sf::Color& col) {
		m_curves.push_back(sf::VertexArray(sf::PrimitiveType::LineStrip, nPoints));
		auto& curve = m_curves[m_curves.size() - 1];
		for (int i = 0; i < nPoints; ++i) {
			curve[i].color = col;
			curve[i].position.x = static_cast<float>(m_chart_rect.left + i * m_chart_rect.width / (nPoints - 1));
		}
	}

	void CurveEditPoints(unsigned int curve_idx, unsigned int start, unsigned int size, const std::vector<float>& points) {
		if (curve_idx < m_curves.size()) {
			auto& curve = m_curves[curve_idx];
			float y_zero = m_chart_rect.top + m_chart_rect.height;
			for (int i = 0; i < size ; ++i, ++start) {
				curve[start].position.y = y_zero - (points[i] / m_max_val) * m_chart_rect.height;
			}
		}		
	}

	// n_lines - number of one type of lines (vertical or horizontal), there are same number of other lines
	void CreateGrid(int n_lines) {
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
	}

private:
	static constexpr int m_margin{ 20 };

	sf::RectangleShape	m_background;
	sf::RectangleShape	m_chart_region;
	sf::FloatRect		m_chart_rect;
	sf::VertexArray		m_outline;
	sf::VertexArray		m_axes;
	sf::VertexArray		m_grid;
	sf::Text			m_x_axis;
	sf::Text			m_y_axis;
	sf::Text			m_title;
	sf::Font			m_font;

	std::vector<sf::VertexArray> m_curves;
	float				m_max_val;
};

}
