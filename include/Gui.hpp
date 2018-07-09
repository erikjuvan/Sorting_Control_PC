#pragma once

#include <SFML\Graphics.hpp>

namespace gui {

class Object : public sf::Drawable {
public:
	virtual void Handle(const sf::Event& event) = 0;
};

class Button : public Object {
	typedef void(*fptr)();

public:
	Button(const char* name, int x, int y, int w = 90, int h = 40, const char* font_name = "arial.ttf", int character_size = 24) : m_idle_shape(sf::Vector2f(static_cast<float>(w), static_cast<float>(h))), m_pressed_shape(m_idle_shape), m_pressed(false) {

		m_idle_shape.setPosition(static_cast<float>(x), static_cast<float>(y));
		m_idle_shape.setFillColor(sf::Color(250, 250, 250, 255));

		m_pressed_shape.setPosition(static_cast<float>(x), static_cast<float>(y));
		m_pressed_shape.setFillColor(sf::Color(200, 200, 200, 255));

		m_active_shape = &m_idle_shape;

		m_font.loadFromFile(font_name);
		m_text.setFont(m_font);
		m_text.setString(name);
		m_text.setCharacterSize(character_size);
		sf::FloatRect text_pos = m_text.getGlobalBounds();
		m_text.setPosition(x - text_pos.left + w / 2 - text_pos.width / 2, y - text_pos.top + h / 2 - text_pos.height / 2);
		m_text.setFillColor(sf::Color(0, 0, 0));
	}

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
		target.draw(*m_active_shape, states);
		target.draw(m_text, states);
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
	Label() {}

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {

	}
};

class Textbox : public Object {
public:
	Textbox() {}

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {

	}
};

class Chart : public Object {
public:
	Chart(int x, int y, int w, int h) : m_x(x), m_y(y), m_w(w), m_h(h),
		m_background(sf::Vector2f(m_w, m_h)), m_axis(sf::PrimitiveType::LineStrip, 3) {

		m_background.setPosition(m_x, m_y);
		m_axis[0].position = sf::Vector2f(m_x + m_margin, m_y + m_margin);
		m_axis[1].position = sf::Vector2f(m_x + m_margin, m_y + m_h - m_margin);
		m_axis[2].position = sf::Vector2f(m_x + m_w - m_margin, m_y + m_h - m_margin);
		m_axis[0].color = m_axis[1].color = m_axis[2].color = sf::Color::Black;

		InitGrid();
	}

	~Chart() {}

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
		target.draw(m_background, states);
		target.draw(m_axis, states);
		target.draw(m_grid, states);
	}

	virtual void Handle(const sf::Event& event) {

	}

	void InitGrid() {
		m_grid = sf::VertexArray(sf::PrimitiveType::Lines, 16);
		int w = m_w - 2 * m_margin;
		int h = m_h - 2 * m_margin;

		// Vertical Lines
		for (int i = 0; i < 4; ++i) {
			m_grid[2 * i].position = sf::Vector2f(m_x + i * w / 4, m_y);
			m_grid[2 * i].color = sf::Color(128, 128, 128, 100);
			m_grid[2 * i + 1].position = sf::Vector2f(m_x + i * w / 4, m_y + m_h);
			m_grid[2 * i + 1].color = sf::Color(128, 128, 128, 100);
		}
		// HorizontalLines
		for (int i = 4; i < 8; ++i) {
			m_grid[2 * i].position = sf::Vector2f(m_x, m_y + (i - 4) * h / 4);
			m_grid[2 * i].color = sf::Color(128, 128, 128, 100);
			m_grid[2 * i + 1].position = sf::Vector2f(m_x + m_w, m_y + (i - 4)* h / 4);
			m_grid[2 * i + 1].color = sf::Color(128, 128, 128, 100);
		}
	}

private:
	static constexpr int m_margin{ 20 };
	int m_x, m_y, m_w, m_h;

	sf::RectangleShape	m_background;
	sf::VertexArray		m_axis;
	sf::VertexArray		m_grid;
};

}
