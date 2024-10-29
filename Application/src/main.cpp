//> main
#include <vk_engine.h>

int main(int argc, char* argv[])
{
	SE::Engine& engine = SE::Engine::getInstance();

	engine.run();

	engine.cleanup();
	return 0;
}
//< main