#include "pch.h"
#include "Globals.h"
#include "memory.h"
#include "OcclusionCulling.h"
#include <cmath>
#include "MainCamera.h"
#include "ConvarGraphics.h"
#include "ConvarAdmin.h"
#include "ConsoleSystem.h"
#include "LocalPlayer.h"
#include "BaseNetworkable.h"
#include "BasePlayer.h"
#include "TODSky.h"
#include "BaseProjectile.h"
#include "CheatFunction.h"
#include "Init.h"
#include "GUI.h"
#include "Configinstance.h"
#include <dwmapi.h>
#include <windows.h>
std::shared_ptr<BasePlayer> BaseLocalPlayer = nullptr;
std::shared_ptr<MainCamera> Camera = nullptr;
std::shared_ptr<ConsoleSystem> Console = nullptr;
std::shared_ptr<TODSky> Sky = nullptr;
ULONG_PTR Eyes_C;
ULONG_PTR Eyes;
std::thread Thread;
bool ThreadRunning = false;

void debugcamera();

struct Quaternion {
	float x, y, z, w;
};

void StartDebugCam() {
	ThreadRunning = true;
	Thread = std::thread(debugcamera);
}

void StopDebugCam() {
	ThreadRunning = false;
	Sleep(10);
	if (Thread.joinable()) {
		Thread.join();
	}
}

double GetYawRad(const Quaternion& q) {
	return atan2(2 * q.y * q.w - 2 * q.x * q.z, 1 - 2 * q.y * q.y - 2 * q.z * q.z);
}

Vector3 GetForwardDirection(const Quaternion& quaternion) {
	return {
		2 * (quaternion.x * quaternion.z + quaternion.w * quaternion.y),
		2 * (quaternion.y * quaternion.z - quaternion.w * quaternion.x),
		1 - 2 * (quaternion.x * quaternion.x - quaternion.y * quaternion.y)
	};
}

Vector3 RotateY(const Vector3& vector, double yaw) {
	double s = sin(yaw);
	double c = cos(yaw);

	return {
		static_cast<float>(vector.x * c - vector.z * s),
		vector.y,
		static_cast<float>(vector.x * s + vector.z * c)
	};
}
// each time we reinitialize localplayer
void PerServerVariables()
{
	std::shared_ptr <LocalPlayer> localplayer = std::make_shared <LocalPlayer>();
	auto handle = TargetProcess.CreateScatterHandle();
	BaseLocalPlayer = std::make_shared <BasePlayer>(localplayer->GetBasePlayer(),handle);
	Eyes = localplayer->GetEyePos();
	Eyes_C = localplayer->GetEye_C();
	TargetProcess.ExecuteReadScatter(handle);
	TargetProcess.CloseScatterHandle(handle);
	BaseLocalPlayer->InitializePlayerList();
	handle = TargetProcess.CreateScatterHandle();
	BaseLocalPlayer->CacheStage1(handle);
	TargetProcess.ExecuteReadScatter(handle);
	TargetProcess.CloseScatterHandle(handle);
	Camera = std::make_shared <MainCamera>();
	Sky = std::make_shared<TODSky>();

	if (!TargetProcess.GetKeyboard()->InitKeyboard())
	{
		std::cout << "Failed to initialize keyboard hotkeys through kernel." << std::endl;
	}
}
void SetupCvars()
{

	std::shared_ptr<OcclusionCulling> occlusionculling = std::make_shared<OcclusionCulling>();
	if (ConfigInstance.Misc.AdminESP)
	{
		occlusionculling->WriteDebugSettings(DebugFilter::Dynamic);
		occlusionculling->WriteLayerMask(131072);
	}
	else
	{
		occlusionculling->WriteDebugSettings(DebugFilter::Off);
		occlusionculling->WriteLayerMask(0);
	}
	std::shared_ptr<ConvarAdmin> convaradmin = std::make_shared<ConvarAdmin>();
	if (ConfigInstance.Misc.RemoveWaterEffect)
		convaradmin->ClearVisionInWater(true);
	if (ConfigInstance.Misc.ChangeTime)
		convaradmin->SetAdminTime(ConfigInstance.Misc.Time);
	else
		convaradmin->SetAdminTime(-1);
	std::shared_ptr<ConvarGraphics> graphics = std::make_shared<ConvarGraphics>();
	if (ConfigInstance.Misc.ChangeFov)
		graphics->WriteFOV(ConfigInstance.Misc.Fov);

}
std::shared_ptr<CheatFunction> CachePlayers = std::make_shared<CheatFunction>(2000, []() {
		BaseLocalPlayer->CachePlayers();
	});
std::shared_ptr<CheatFunction> UpdateMovement = std::make_shared<CheatFunction>(38, []() {
	if (ConfigInstance.Misc.SpiderMan)
	{
		auto handle = TargetProcess.CreateScatterHandle();
		BaseLocalPlayer->GetBaseMovement()->WriteGroundAngleNew(handle, 0.f);
		BaseLocalPlayer->GetBaseMovement()->WriteMaxAngleWalking(handle, 100.f);
		BaseLocalPlayer->GetBaseMovement()->WriteGroundAngle(handle, 0.f);
		TargetProcess.ExecuteScatterWrite(handle);
		TargetProcess.CloseScatterHandle(handle);
		
	}
	});
std::shared_ptr<CheatFunction> UpdateLocalPlayer = std::make_shared<CheatFunction>(300, []() {

	if (ConfigInstance.Misc.NoRecoil)
	{
		BaseLocalPlayer->SetupBeltContainerList();
	}

	auto handle = TargetProcess.CreateScatterHandle();
	BaseLocalPlayer->UpdateActiveItemID(handle);
	BaseLocalPlayer->UpdateActiveFlag(handle);
	TargetProcess.ExecuteReadScatter(handle);
	TargetProcess.CloseScatterHandle(handle);

	if(ConfigInstance.Misc.NoRecoil)
	{
		std::shared_ptr <Item> helditem = BaseLocalPlayer->GetActiveItem();
		if (helditem != nullptr)
		{
			std::shared_ptr <BaseProjectile> weapon = helditem->GetBaseProjectile();
			if (weapon->IsValidWeapon())
			{
				handle = TargetProcess.CreateScatterHandle();
				weapon->WriteRecoilPitch(handle,helditem->GetItemID(),ConfigInstance.Misc.RecoilX);
				weapon->WriteRecoilYaw(handle,helditem->GetItemID(), ConfigInstance.Misc.RecoilY);
				TargetProcess.ExecuteScatterWrite(handle);
				TargetProcess.CloseScatterHandle(handle);
				
			}

		}

	}
	
	if (ConfigInstance.Misc.AdminFlag && !ThreadRunning) {
		StartDebugCam();
	}
	else if (!ConfigInstance.Misc.AdminFlag && ThreadRunning) {
		StopDebugCam();
	}

	//if (ConfigInstance.Misc.AdminFlag)
	//{
	//	if ((BaseLocalPlayer->GetActiveFlag() & (int)4) != (int)4)
	//	{
	//		if (Console == nullptr)
	//		{
	//			Console = std::make_shared<ConsoleSystem>();
	//
	//		}
	//		BaseLocalPlayer->WriteActiveFlag(BaseLocalPlayer->GetActiveFlag() + 4);
	//	}
	//}
	});
std::shared_ptr<CheatFunction> SkyManager = std::make_shared<CheatFunction>(7, []() {
	auto handle = TargetProcess.CreateScatterHandle();
	if (ConfigInstance.Misc.BrightNights)
	{
		Sky->WriteNightLightIntensity(handle, 25.0f);
		Sky->WriteNightAmbientMultiplier(handle, 4.0f);
	}

		if (ConfigInstance.Misc.BrightCaves)
		{
			Sky->WriteDayAmbientMultiplier(handle, 2.0f);

		}
		TargetProcess.ExecuteScatterWrite(handle);
		TargetProcess.CloseScatterHandle(handle);
	

	});

void Caching()
{
	if (BaseLocalPlayer->GetPlayerListSize() == 0)
		return;
	CachePlayers->Execute();
	UpdateLocalPlayer->Execute();
	SkyManager->Execute();
	UpdateMovement->Execute();
}
void Intialize()
{
	PerServerVariables();
	while (!BaseLocalPlayer->IsPlayerValid())
	{
		Sleep(4000);

		Intialize(); // wait till localplayer is valid.
	}
	SetupCvars();
	CachePlayers->Execute();
	
}
void main()
{
	if (!TargetProcess.Init("RustClient.exe"))
	{
		printf("Failed to initialize process\n");
		return;
	}
	uint64_t base = TargetProcess.GetBaseAddress("GameAssembly.dll");
	TargetProcess.FixCr3();


	Intialize();
}


void debugcamera() {

	double previousYaw = 0.0;
	int moveCam;
	Vector3 targetmovement{ 0.0f , 1.5f, 0.0f };
	float camSpeed = 0.00015f;
	float camSpeedMultiplier = 5;
	float camDrag = 0.99f;
	bool camFlyToLook = true;
	bool camFast = false;
	Vector3 camVelocity = { 0.0f, 0.0f, 0.0f };
	Vector3 forward = { 0.0f, 0.0f, 1.0f };
	Vector3 right = { 1.0f, 0.0f, 0.0f };
	Vector3 up = { 0.0f, 1.0f, 0.0f };

	std::chrono::steady_clock::time_point startTime, endTime;
	float deltaTime = 0.0f;

	startTime = std::chrono::steady_clock::now();


	while(ThreadRunning)
	{
		endTime = std::chrono::steady_clock::now();

		std::chrono::duration<float, std::milli> duration = endTime - startTime;
		deltaTime = duration.count();

		startTime = std::chrono::steady_clock::now();

		Quaternion currentRotation = TargetProcess.Read<Quaternion>(Eyes_C + 0x4C);

		double currentYaw = GetYawRad(currentRotation);

		double deltaRotation = currentYaw - previousYaw;

		previousYaw = currentYaw;

		if (deltaRotation != 0)
			targetmovement = RotateY(targetmovement, deltaRotation);

		if (TargetProcess.GetKeyboard()->IsKeyDown(0x52)) // R = reset viewpoint
		{
			camVelocity = Vector3();
			targetmovement = Vector3();
		}

		if (TargetProcess.GetKeyboard()->IsKeyDown(0x51)) { // Q
			camFlyToLook = !camFlyToLook; // camera goes where you are facing toggle
		}

		if (TargetProcess.GetKeyboard()->IsKeyDown(0xA0)) { // left shift faster movment
			camFast = true;
		}
		else {
			camFast = false;
		}

		moveCam = 0;

		if (TargetProcess.GetKeyboard()->IsKeyDown(0x57)) // W forwards
		{
			camVelocity += forward;
			moveCam = 1;
		}

		if (TargetProcess.GetKeyboard()->IsKeyDown(0x53)) // S backwards
		{
			camVelocity -= forward;
			moveCam = -1;
		}

		if (TargetProcess.GetKeyboard()->IsKeyDown(0x41)) { // A left
			camVelocity -= right;
		}
		if (TargetProcess.GetKeyboard()->IsKeyDown(0x44)) { // D right
			camVelocity += right;
		}

		if (camFlyToLook)
		{
			camVelocity.y += GetForwardDirection(currentRotation).y * moveCam;
		}
		else
		{
			if (TargetProcess.GetKeyboard()->IsKeyDown(0xA2)) { // left ctrl go down
				camVelocity -= up;
			}
			if (TargetProcess.GetKeyboard()->IsKeyDown(0x20)) { // spacebar go up
				camVelocity += up;
			}
		}


		if (camFast)
			targetmovement += camVelocity * deltaTime * camSpeed * camSpeedMultiplier;
		else
			targetmovement += camVelocity * deltaTime * camSpeed;

		camVelocity *= camDrag;

		TargetProcess.Write<Vector3>(Eyes, targetmovement); //move our eyes to the calculated value
	}
	TargetProcess.Write<Vector3>(Eyes, { 0.0f , 1.5f, 0.0f }); //restore out eyes to their proper position
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	InputWndProc(hWnd, message, wParam, lParam);
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc;
	AllocConsole();
	FILE* fDummy;
	freopen_s(&fDummy, LIT("CONIN$"), LIT("r"), stdin);
	freopen_s(&fDummy, LIT("CONOUT$"), LIT("w"), stderr);
	freopen_s(&fDummy, LIT("CONOUT$"), LIT("w"), stdout);
	printf(LIT("Debugging Window:\n"));
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	main();
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = L"GUI Framework";
	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(WS_EX_APPWINDOW, wc.lpszClassName, L"GUI Framework",
		WS_POPUP,
		0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return -1;


	SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 255, LWA_ALPHA);

	ShowWindow(hWnd, nCmdShow);

	InitD2D(hWnd);
	CreateGUI();
	MSG msg;
	SetProcessDPIAware();
	SetInput();
	while (TRUE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				break;
		}
		Caching();
		RenderFrame();

	}
	CleanD2D();
	return msg.wParam;
}
