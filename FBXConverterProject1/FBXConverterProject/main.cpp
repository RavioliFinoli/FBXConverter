#include "Manager.h"


int main()
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	Converter converter("test.fbx");
	converter.convertFileToCustomFormat();
	std::cout <<        "DONE"<< std::endl;


	std::getchar();
	return 0;
}
