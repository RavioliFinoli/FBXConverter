#include "Manager.h"


int main(int argc, char **argv)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	std::string file = "C:\\FBXConverter\\FBXConverterProject1\\FBXConverterProject\\P1_idle.fbx";

	argc > 1
		? file = std::string(argv[1])
		: file = file;

	Converter converter(file.c_str());
	converter.convertFileToCustomFormat();

	SetConsoleTextAttribute(hConsole, 2);
	std::cout << "Export finished."<< std::endl;

	std::getchar();
	return 0;
}
