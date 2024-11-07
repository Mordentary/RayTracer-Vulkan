//> main
#include <core/engine.hpp>

uint32_t WINDOW_WIDTH = 2560, WINDOW_HEIGHT = 1440;
int main(int argc, char* argv[])
{
	SE::Engine& engine = SE::Engine::getInstance();

	engine.init(WINDOW_WIDTH, WINDOW_HEIGHT);
	engine.run();
	engine.shutdown();
	return 0;
}
//< main