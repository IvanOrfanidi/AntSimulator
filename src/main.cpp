#include <SFML/Graphics.hpp>
#include <vector>
#include <list>
#include <fstream>
#include "colony.hpp"
#include "config.hpp"
#include "display_manager.hpp"
#include "distance_field_builder.hpp"
#include "racc.hpp"


void loadUserConf()
{
	std::ifstream conf_file("conf.txt");
	if (conf_file) {
		conf_file >> Conf::WIN_WIDTH;
		conf_file >> Conf::WIN_HEIGHT;
		conf_file >> Conf::ANTS_COUNT;
	}
	else {
		std::cout << "Couldn't find 'conf.txt', loading default" << std::endl;
	}
}


int main()
{
	Conf::loadTextures();
	loadUserConf();

	sf::ContextSettings settings;
	settings.antialiasingLevel = 4;
	sf::RenderWindow window(sf::VideoMode(Conf::WIN_WIDTH, Conf::WIN_HEIGHT), "AntSim", sf::Style::Fullscreen, settings);
	window.setFramerateLimit(60);

	World world(Conf::WORLD_WIDTH, Conf::WORLD_HEIGHT);
	Colony colony(300.0f, Conf::WORLD_HEIGHT * 0.5f, Conf::ANTS_COUNT/2);
	Colony colony2(Conf::WIN_WIDTH - 300.0f, Conf::WORLD_HEIGHT * 0.5f, Conf::ANTS_COUNT/2);
	ColonyRenderer renderer;

	for (uint32_t i(0); i < 64; ++i) {
		float angle = float(i) / 64.0f * (2.0f * PI);
		world.addMarker(colony.base.position + 16.0f * sf::Vector2f(cos(angle), sin(angle)), Mode::ToHome, 10.0f, true);
	}
	
	DisplayManager display_manager(window, window, world, colony);
	display_manager.setZoom(1.02f);

	sf::Vector2f last_clic;

	sf::Image food_map;
	if (food_map.loadFromFile("map.bmp")) {
		for (uint32_t x(0); x < food_map.getSize().x; ++x) {
			for (uint32_t y(0); y < food_map.getSize().y; ++y) {
				const sf::Vector2f position = float(world.map.cell_size) * sf::Vector2f(to<float>(x), to<float>(y));
				if (food_map.getPixel(x, y).r > 100) {
					world.addWall(position);
				}
				else if (food_map.getPixel(x, y).g > 100) {
					const float green = food_map.getPixel(x, y).g;
					world.addFoodAt(position.x, position.y, 4);
				}
			}
		}
	}

	DistanceFieldBuilder::computeDistance(world.map);

	display_manager.pause = true;
	display_manager.render_ants = true;
	world.renderer.draw_density = false;
	world.renderer.draw_markers = false;

	sf::Clock clock;
	RMean<float> fps(100);

	while (window.isOpen())
	{
		display_manager.processEvents();
		// Add food on clic
		if (display_manager.clic) {
			const sf::Vector2i mouse_position = sf::Mouse::getPosition(window);
			const sf::Vector2f world_position = display_manager.displayCoordToWorldCoord(sf::Vector2f(to<float>(mouse_position.x), to<float>(mouse_position.y)));
			const float clic_min_dist = 2.0f;
			if (getLength(world_position - last_clic) > clic_min_dist) {
				if (display_manager.wall_mode) {
					world.addWall(world_position);
				}
				else if (display_manager.remove_wall) {
					world.removeWall(world_position);
				}
				else {
					world.addFoodAt(world_position.x, world_position.y, 10);
				}
				last_clic = world_position;
			}
		}

		const float dt = 0.016f;

		if (!display_manager.pause) {
			colony.update(dt, world, colony2);
			colony2.update(dt, world, colony);
			//display_manager.colony_renderer.updatePopulation(colony, dt);
			world.update(dt);
		}

		window.clear(sf::Color::Black);
		display_manager.draw();
		sf::RenderStates states = display_manager.getStates();
		renderer.renderAnts(colony, window, states);
		renderer.render(colony, window, states);
		renderer.renderAnts(colony2, window, states);
		renderer.render(colony2, window, states);
		window.display();

		fps.addValue(1.0f / clock.restart().asSeconds());
	}

	// Free textures
	Conf::freeTextures();

	return 0;
}

#if defined(_WIN32)
#include <windows.h>
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline,
                     int cmdshow) {
  return main();
}
#endif