#include "Settings.h"


// Static constants
const WCHAR Settings::szSettingsKeyPath[] = L"SOFTWARE\\PellucidIcons\\Settings";

// Static variables
Settings::In Settings::InSetting = Settings::In();
Settings::RestoreWhen Settings::RestoreWhenSetting = Settings::RestoreWhen();
Settings::To Settings::ToSetting = Settings::To();
bool Settings::IsEnabledSetting = true;


void Settings::ForceSettingsRefreshFromRegistry()
{
	InSetting = static_cast<In>(getSetting(L"In"));
	RestoreWhenSetting = static_cast<RestoreWhen>(getSetting(L"RestoreWhen"));
	ToSetting = static_cast<To>(getSetting(L"To"));
	IsEnabledSetting = (getSetting(L"Enabled") > FALSE);
}

Settings::In Settings::getInSetting()
{
	return InSetting;
}

Settings::RestoreWhen Settings::getRestoreWhenSetting()
{
	return RestoreWhenSetting;
}

Settings::To Settings::getToSetting()
{
	return ToSetting;
}

bool Settings::getIsEnabled()
{
	return IsEnabledSetting;
}

DWORD Settings::getSetting(LPWSTR szValueName)
{
	HKEY hkeyPellucidSettings;

	if (RegOpenKeyEx(HKEY_CURRENT_USER,
						szSettingsKeyPath,
						0,
						KEY_READ,
						&hkeyPellucidSettings) == ERROR_SUCCESS)
	{
		DWORD Type;
		DWORD Data;
		DWORD cbData;

		if (RegQueryValueEx(hkeyPellucidSettings,
							szValueName,
							NULL,
							&Type,
							(LPBYTE)&Data,
							&cbData) == ERROR_SUCCESS)
		{
			if (Type == REG_DWORD && cbData == sizeof(DWORD))
			{
				return Data;
			}
		}

		RegCloseKey(hkeyPellucidSettings);
	}

	return 0;	// Default setting
}


void Settings::setInSetting(In setting)
{
	setSetting(L"In", static_cast<DWORD>(setting));
	InSetting = setting;
}

void Settings::setRestoreWhenSetting(RestoreWhen setting)
{
	setSetting(L"RestoreWhen", static_cast<DWORD>(setting));
	RestoreWhenSetting = setting;
}

void Settings::setToSetting(To setting)
{
	setSetting(L"To", static_cast<DWORD>(setting));
	ToSetting = setting;
}

void Settings::setIsEnabled(bool setting)
{
	setSetting(L"Enabled", (setting ? 1 : 0));
	IsEnabledSetting = setting;
}

void Settings::setSetting(LPWSTR szValueName, DWORD value)
{
	HKEY hkeyPellucidSettings;

	// Create key if it doesn't exists
	// NOTE: If both 'Pellucid' and its subkey 'Settings' doesn't exist, the 'RegCreateKeyEx()' function creates both
	if (RegCreateKeyEx(HKEY_CURRENT_USER,
						szSettingsKeyPath,
						NULL,
						NULL,
						REG_OPTION_NON_VOLATILE,
						KEY_READ | KEY_WRITE,
						NULL,
						&hkeyPellucidSettings, NULL) != ERROR_SUCCESS)
	{
		// TODO: Log error
		return;
	}

	RegSetValueEx(hkeyPellucidSettings, szValueName, NULL, REG_DWORD, (const BYTE *)&value, sizeof(value));

	RegCloseKey(hkeyPellucidSettings);
}

UINT Settings::convertInToMillisecs(Settings::In in)
{
	switch (in)
	{
	case Settings::In::secs5: return 5000;
	case Settings::In::secs10: return 10000;
	case Settings::In::secs20: return 20000;
	case Settings::In::secs30: return 30000;
	case Settings::In::min1: return 60000;
	case Settings::In::mins2: return 120000;
	default:	// TODO: Add error reporting
		break;
	}

	return 5000;	// Should not reach here
}