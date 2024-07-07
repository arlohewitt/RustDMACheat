#include "Pch.h"
#include "LocalPlayer.h"
#include "Globals.h"
#include <tuple>

LocalPlayer::LocalPlayer()
{
	printf("[LocalPlayer] Initialized\n");
	Class = TargetProcess.Read<uint64_t>(TargetProcess.GetBaseAddress(LIT("GameAssembly.dll")) + Class); // Get Class Start Address
	printf("[LocalPlayer] LocalPlayer: 0x%llX\n", Class);
	this->StaticField = TargetProcess.Read<uint64_t>(Class + StaticField); // Set Static Padding
	printf("[LocalPlayer] Static Fields: 0x%llX\n", StaticField);
	this->BasePlayerValue = TargetProcess.Read<uint64_t>(StaticField + BasePlayerBackingField); // Set BasePlayer Backing Field
	printf("[LocalPlayer] Base Player: 0x%llX\n", BasePlayerValue);
	ULONG_PTR Eyes = TargetProcess.Read<ULONG_PTR>(BasePlayerValue + 0xA90);
	ULONG_PTR Eyes_C = TargetProcess.Read<ULONG_PTR>(Eyes);
	ULONG_PTR Eyes_padded = TargetProcess.Read<ULONG_PTR>(Eyes_C + 0xb8);
}

uint64_t LocalPlayer::GetBasePlayer()
{
	return BasePlayerValue;
}

uint64_t LocalPlayer::GetEyePos() {
	uint64_t Eyes = TargetProcess.Read<uint64_t>(BasePlayerValue + 0xA90);
	uint64_t Eyes_C = TargetProcess.Read<uint64_t>(Eyes);
	uint64_t Eyes_padded = TargetProcess.Read<uint64_t>(Eyes_C + 0xB8);

	return Eyes_padded;
}

uint64_t LocalPlayer::GetEye_C() {
	uint64_t Eyes = TargetProcess.Read<uint64_t>(BasePlayerValue + 0xA90);

	return Eyes;
}

void LocalPlayer::UpdateBasePlayer(VMMDLL_SCATTER_HANDLE handle)
{
	TargetProcess.AddScatterReadRequest(handle, StaticField + BasePlayerBackingField,reinterpret_cast<void*>(&BasePlayerValue),sizeof(uint64_t));
}

bool LocalPlayer::IsLocalPlayerValid()
{
	return BasePlayerValue != 0;
}

LocalPlayer::~LocalPlayer()
{
}