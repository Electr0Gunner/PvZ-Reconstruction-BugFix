#ifndef __SETTINGSMANAGER_H__
#define __SETTINGSMANAGER_H__

#include <json.hpp>

using json = nlohmann::json;

namespace Sexy
{

	class SexyAppBase;

	class SettingsManager
	{
	public:
		SexyAppBase*			mApp;
		json					mSettingsJson;
	
		SettingsManager(SexyAppBase* theApp);
		virtual ~SettingsManager();

		json					GetSettings();
		void					SetSettings();

		bool					HasFailed();
	};

};

#endif //__XMLPARSER_H__
