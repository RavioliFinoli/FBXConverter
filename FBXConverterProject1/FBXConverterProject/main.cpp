#include "Manager.h"


int main()
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	Converter converter("test.fbx");
	converter.convertFileToCustomFormat();

	SetConsoleTextAttribute(hConsole, 2);
	std::cout << "Export finished."<< std::endl;


	std::getchar();
	return 0;
}
