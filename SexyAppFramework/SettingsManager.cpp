#include "SettingsManager.h"
#include "SexyAppBase.h"
#include <iostream>
#include <fstream>

using namespace Sexy;

SettingsManager::SettingsManager(SexyAppBase* theApp)
{
    mApp = theApp;
}

SettingsManager::~SettingsManager()
{
}

json SettingsManager::GetSettings()
{
    json theSettings;
    std::ifstream file("settings.json");
    if (file.is_open())
        file >> theSettings;
    else
    {
        SetSettings();
        theSettings = GetSettings();
    }
    return theSettings;
}

void SettingsManager::SetSettings()
{
    json settings;
    settings["MusicVolume"] = mApp->mMusicVolume;
    settings["SoundVolume"] = mApp->mSfxVolume;
    settings["3DAccelerated"] = mApp->Is3DAccelerated();
    settings["FullScreen"] = !mApp->mIsWindowed;

    std::ofstream file("settings.json");
    if (file.is_open()) {
        file << settings.dump(4);
    }
    else {
        mApp->Popup("Failed to save settings");
    }
}

bool SettingsManager::HasFailed()
{
	return false;
}
