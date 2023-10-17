#include "PreCompile.h"
#include "TestClient.h"

int main()
{
	std::wstring optionFile = L"";
	TestClient client(optionFile);

	while (true)
	{
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
		{
			break;
		}
	}

	return 0;
}