#ifndef __BJNHELPERS_H_
#define __BJNHELPERS_H_
#include <iostream>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
using namespace std;

// uncomment this to add the getPluginConfigPaths() API to the
// master and install plugin so that and change in the Firebreath
// plugin configuration macros can be developer tested
//#define ENABLE_CONFIG_PATH_TEST_API

#define CONFIG_DIRECTORY     "Resources"
#define LOGS_DIRECTORY      "logs"
#define LOGS_DIRECTORY_LEVEL 1      // Depth from local Appdata company directory

#define LOG_CLEANUP_DURATION  604800   // 7 days (7 * 24 * 60 *60)

#define XP_MAJOR_VERSION 5
#define XP_MINOR_VERSION 1

#define BrowserHostPtr void*

namespace BJN
{
	enum PluginColor {
		kPluginColorInvalid,
		kPluginColorRed,
		kPluginColorGreen,
		kPluginColorBlue,
	};

	const char kPluginBaseNameRed[] = "rbjnplugin";
	const char kPluginBaseNameGreen[] = "gbjnplugin";
	const char kPluginBaseNameBlue[] = "bjnplugin";
	// this is the length of the shortest string above
	const size_t kPluginBaseNameCharsToComp = 9;

	void quoted_file_path(std::string &fileName);
	std::string getBjnAppDataPath();
	std::string getBjnLocalAppDataPath();
	std::string getBjnPcapPath();
	std::string getBjnConfigFile();
	std::string getBjnConfigPath();
	std::string getBjnLogDirName();
	std::string getBjnLocalAppCompanyDir();
	std::string getBjnCarmelAppCompanyDir();
	std::string getBjnLogfilePath();
	std::string getBjnLogfilePath(bool withGuid);
	void setGuid(std::string guid);
	std::string getGuid();
	int getBjnLogDirectoryLevel();
	std::string getBjnDownloadPath();
	std::string getBjnAutoUpdaterEXE();
	std::string getBjnOldLocalAppDataPath();
	std::string getBjnEffectFile();
	std::string getBjnEndpointName();
	std::string getVersion(const std::string& pluginName);
	std::string getMasterVersion();
	std::string getBjnCurrentLogFileName();
	std::string getBjnPackageType();
	PluginColor getPluginColorFromName(const std::string& pluginName);
	void setPluginPath(std::string _path);
	std::string getInstallType();

	bool hasEnding(std::string const &fullString, std::string const &ending);
	bool checkForValidSite(std::string location);
	bool checkForBlueJeansApp();
	bool detectProxySettings(std::string url, const BrowserHostPtr host,
		std::string &proxyStr, std::string &proxyInfoStr,
		std::string &proxyAuthStr, bool &isProxyAuthReq);
	bool detectProxySettings(std::string url, const BrowserHostPtr host,
		std::string &proxyStr, std::string &proxyInfoStr,
		std::string &proxyAuthStr);
	bool detectProxyAuthSettings(const BrowserHostPtr host, std::string &proxyHostStrRef,
		std::string &proxyPortStrRef, std::string &proxyAuthStrRef);
	bool isProxyAuthCredentialsValid(std::string url, std::string proxyStr, std::string &proxyAuthStr,
		bool saveToKeychain, const char *username, const char *pwd);
#ifdef FB_WIN
	std::string getMasterVersionForColor(PluginColor color);
	void getRegKey(wchar_t *wkey, size_t size, const std::string& pluginName);
	void getRegKeyForMaster(wchar_t *wkey, size_t size, PluginColor color);
	bool detectWinProxySettings(std::string url, const BrowserHostPtr host,
		std::string &proxyStr, std::string &proxyInfoStr,
		std::string &proxyAuthStr, bool &isProxyAuthReq);
	void refreshIEElevationPolicy();
	bool verifyBinaryCertificate(const std::string &pkgPath);
#else
	void handler(int sig);
	void registerSignalhandler();
#endif    

#ifdef FB_MACOSX
	std::string getMacPluginBasePath();
	std::string getDiagReportPath();
	bool verifyBinaryCertificate(const std::string &pkgPath);
#endif
#ifdef FB_X11
	std::string getLinuxPluginBasePath();
#endif

#ifdef ENABLE_CONFIG_PATH_TEST_API
	void getPluginConfigPaths(std::map<std::string, std::string>& pathMap, const std::string& pluginName);
#endif
}
#endif  // __BJNHELPERS_H_