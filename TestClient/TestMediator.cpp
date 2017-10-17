#include "TestMediator.h"
#include <iomanip>
#include <fcntl.h>
#include <corecrt_io.h>

bool TestMediator::InitEvents()
{
	_stopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if (_stopEvent == nullptr)
	{
		CONSOLE_DEBUG_OUTPUT << _T("Failed to create _stopEvent. Error: ") << GetLastError() << ENDL;
		return false;
	}
	return true;
}
bool TestMediator::InitLib()
{
	auto initErr = SccTcpMediator::STMStartupLocal();
	if (initErr) {
		CONSOLE_DEBUG_OUTPUT << _T("Failed to initialize SccTcpMediator. Error: ") << initErr << ENDL;
		return false;
	}
	return true;
}

void TestMediator::PrintMenu() const
{
	system("cls");

	CONSOLE_DEBUG_OUTPUT << _T("\tCONTROLS:\n") << ENDL;

	CONSOLE_DEBUG_OUTPUT << _T("F1\tPing") << ENDL;
	CONSOLE_DEBUG_OUTPUT << _T("F2\tDLL version") << ENDL;
	CONSOLE_DEBUG_OUTPUT << _T("F3\tPin configuration") << ENDL;
	CONSOLE_DEBUG_OUTPUT << _T("F4\tSubscribed to device events") << ENDL;
	CONSOLE_DEBUG_OUTPUT << _T("F5\tReset Buffer") << ENDL;
	CONSOLE_DEBUG_OUTPUT << _T("F6\tChannels status") << ENDL;
	CONSOLE_DEBUG_OUTPUT << _T("F7\tSimulate forced connection") << ENDL;
	CONSOLE_DEBUG_OUTPUT << _T("F8\tSimulate release") << ENDL;



	CONSOLE_DEBUG_OUTPUT << _T("↑\tCreate sending thread") << ENDL;
	CONSOLE_DEBUG_OUTPUT << _T("↓\tRemove sending thread") << ENDL;
	CONSOLE_DEBUG_OUTPUT << _T("→\tCreate recieving thread") << ENDL;
	CONSOLE_DEBUG_OUTPUT << _T("←\tRemove recieving thread") << ENDL;

	CONSOLE_DEBUG_OUTPUT << ENDL;
}

bool TestMediator::IsActiveConsoleWindow() const
{
	return _consoleWindowHandle == GetForegroundWindow();
}

TestMediator::TestMediator() : _stopEvent(nullptr)
{
	_consoleWindowHandle = GetConsoleWindow();
}

void TestMediator::Start()
{
	auto result = _setmode(_fileno(stdout), _O_U16TEXT);
	if (result == -1) {
		CONSOLE_DEBUG_OUTPUT << "Cannot set mode _O_U16TEXT" << ENDL;
		return;
	}
	if (!InitEvents() || !InitLib())
		return;
	auto pos = 0;
	while (!SccTcpMediator::IsOnline())
	{
		if (++pos == ARINC_429_CHANNELS)
			pos = 0;
		system("cls");
		CONSOLE_DEBUG_OUTPUT << _T("\rEstablishing connection") << STD setfill(_T('.')) << STD setw(pos) << "" << STD setfill(_T(' '));
		Sleep(500);
	}

	PrintMenu();
	while (WaitForSingleObject(_stopEvent, 0) != WAIT_OBJECT_0)
	{
		Sleep(100);
		if (GetAsyncKeyState(VK_F1) && IsActiveConsoleWindow())
		{
			PrintMenu();
			Ping();
			continue;
		}
		if (GetAsyncKeyState(VK_F2) && IsActiveConsoleWindow())
		{
			PrintMenu();
			GetDllVersion();
			continue;
		}
		if (GetAsyncKeyState(VK_F3) && IsActiveConsoleWindow())
		{
			PrintMenu();
			GetPinConf();
			continue;
		}
		if (GetAsyncKeyState(VK_F4) && IsActiveConsoleWindow())
		{
			PrintMenu();
			SubscribeToEvents();
			continue;
		}
		if (GetAsyncKeyState(VK_F5) && IsActiveConsoleWindow())
		{
			PrintMenu();
			ResetBuffer();
			continue;
		}
		if (GetAsyncKeyState(VK_F6) && IsActiveConsoleWindow())
		{
			PrintMenu();
			ShowChannelsStates();
			continue;
		}
		if (GetAsyncKeyState(VK_F7) && IsActiveConsoleWindow())
		{
			PrintMenu();
			ConnectForced();
			continue;
		}
		if (GetAsyncKeyState(VK_F8) && IsActiveConsoleWindow())
		{
			PrintMenu();
			Release429();
			continue;
		}
		if (GetAsyncKeyState(VK_UP) && IsActiveConsoleWindow())
		{
			PrintMenu();
			AddSender429();
			continue;
		}
		if (GetAsyncKeyState(VK_DOWN) && IsActiveConsoleWindow())
		{
			PrintMenu();
			RemoveSender();
			continue;
		}
		if (GetAsyncKeyState(VK_RIGHT) && IsActiveConsoleWindow())
		{
			PrintMenu();
			AddListener429();
			continue;
		}
		if (GetAsyncKeyState(VK_LEFT) && IsActiveConsoleWindow())
		{
			PrintMenu();
			RemoveListener429();
			continue;
		}
		if (GetAsyncKeyState(VK_ESCAPE) && IsActiveConsoleWindow())
		{
			Stop();
			break;
		}
	}
}

void TestMediator::Stop()
{
	SetEvent(_stopEvent);
	for (auto && h_event : _sendEvents)
		SetEvent(h_event);

	for (auto && h_event : _recEvents)
		SetEvent(h_event);

	for (auto &&thread : _threads)
	{
		if (thread.joinable())
			thread.join();
	}
}

TestMediator::~TestMediator()
{
	Stop();
	for (auto && h_event : _sendEvents)
		CloseHandle(h_event);
	for (auto && h_event : _recEvents)
		CloseHandle(h_event);
}
