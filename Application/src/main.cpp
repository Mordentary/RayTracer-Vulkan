//> main
#include <vk_engine.h>

int main(int argc, char* argv[])
{
	SE::Engine engine;

	engine.init();	
	
	engine.run();	

	engine.cleanup();	

	return 0;
}
//< main
