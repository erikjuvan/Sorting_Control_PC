#include <random>
#include <chrono>
#include <iostream>

#include <SFML/Graphics.hpp>

#include "Helpers.hpp"
#include "chart.hpp"

int main(int argc, char *argv[])
{
	std::default_random_engine dre;
	std::uniform_int_distribution<int> di(0, 30);    
	dre.seed(std::chrono::system_clock::now().time_since_epoch().count());

	sf::RenderWindow window(sf::VideoMode(1100, 700), "Sorting Control");

	while (window.isOpen()) {
		// check all the window's events that were triggered since the last iteration of the loop
		sf::Event event;
		while (window.pollEvent(event))
		{
			// "close requested" event: we close the window
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// clear the window with black color
		window.clear(sf::Color::Black);

		// draw everything here...
		// window.draw(...);

		// end the current frame
		window.display();
	}

	return 0;
}