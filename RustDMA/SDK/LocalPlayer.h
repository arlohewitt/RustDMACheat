#pragma once

class LocalPlayer
{
	/*
	"Address": 54566200,
      "Name": "LocalPlayer_TypeInfo",
      "Signature": "LocalPlayer_c*"
	*/
	uint64_t Class = 0x3C5BD20; // 3C7 5438
	//Dump.cs / DummyDLL
	uint64_t StaticField = 0xB8;// Static Padding To Access Static Fields
	uint64_t BasePlayerBackingField = 0x0; // private static BasePlayer <Entity>k__BackingField;
	int64_t BasePlayerValue; // the baseplayer value
public:

	LocalPlayer();
	~LocalPlayer();
	uint64_t GetBasePlayer();
	uint64_t GetEyePos();
	uint64_t GetEye_C();
	void UpdateBasePlayer(VMMDLL_SCATTER_HANDLE);
	bool IsLocalPlayerValid();
};
