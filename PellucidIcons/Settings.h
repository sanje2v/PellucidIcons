#pragma once
#include <Windows.h>


class Settings
{
public:
#pragma region Enums
	enum class In
	{
		secs5,
		secs10,
		secs20,
		secs30,
		min1,
		mins2
	};

	enum class RestoreWhen
	{
		mousedMoved,
		mousedEntersQuarterRegionOnLeft,
		doubleClicked
	};

	enum class To
	{
		fullTransparency,
		semiTransparency
	};

	enum class Enabled
	{
		False,
		True
	};
#pragma endregion

#pragma region Functions
	static void ForceSettingsRefreshFromRegistry();

	static In getInSetting();
	static RestoreWhen getRestoreWhenSetting();
	static To getToSetting();
	static bool getIsEnabled();

	static void setInSetting(In setting);
	static void setRestoreWhenSetting(RestoreWhen setting);
	static void setToSetting(To setting);
	static void setIsEnabled(bool setting);

	static UINT convertInToMillisecs(In in);
#pragma endregion

private:
	// Constants
	static const WCHAR szSettingsKeyPath[];

	// Variables
	static In InSetting;
	static RestoreWhen RestoreWhenSetting;
	static To ToSetting;
	static bool IsEnabledSetting;

	static DWORD getSetting(LPWSTR szValueName);
	static void setSetting(LPWSTR szValueName, DWORD value);
};




