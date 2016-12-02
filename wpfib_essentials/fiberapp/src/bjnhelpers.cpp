#include "string.h"
#include <map>
#include "talk/base/logging_libjingle.h"
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <curl/curl.h>
#include "bjnhelpers.h"
#ifdef FB_WIN
#include "talk/base/win32regkey.h"
#include <windows.h>
#include <WinInet.h>
#include <WinCrypt.h>
#include <Shlwapi.h>
#include <atlbase.h>
#include <atlconv.h>

#pragma comment (lib,"crypt32.lib")

#undef BOOLAPI
#undef SECURITY_FLAG_IGNORE_CERT_DATE_INVALID
#undef SECURITY_FLAG_IGNORE_CERT_CN_INVALID

#define URL_COMPONENTS URL_COMPONENTS_ANOTHER
#define URL_COMPONENTSA URL_COMPONENTSA_ANOTHER
#define URL_COMPONENTSW URL_COMPONENTSW_ANOTHER

#define LPURL_COMPONENTS LPURL_COMPONENTS_ANOTHER
#define LPURL_COMPONENTSA LPURL_COMPONENTS_ANOTHER
#define LPURL_COMPONENTSW LPURL_COMPONENTS_ANOTHER

#define INTERNET_SCHEME INTERNET_SCHEME_ANOTHER
#define LPINTERNET_SCHEME LPINTERNET_SCHEME_ANOTHER

#define HTTP_VERSION_INFO HTTP_VERSION_INFO_ANOTHER
#define LPHTTP_VERSION_INFO LPHTTP_VERSION_INFO_ANOTHER

#include <winhttp.h>

#undef URL_COMPONENTS
#undef URL_COMPONENTSA
#undef URL_COMPONENTSW

#undef LPURL_COMPONENTS
#undef LPURL_COMPONENTSA
#undef LPURL_COMPONENTSW

#undef INTERNET_SCHEME
#undef LPINTERNET_SCHEME

#undef HTTP_VERSION_INFO
#undef LPHTTP_VERSION_INFO

#include <wchar.h>
#endif

#ifndef FB_WIN
#include <execinfo.h>
#include <signal.h>
#include <syslog.h>
#include <stdlib.h>
#endif

#ifdef FB_X11
#include "talk/base/linux.h"
#include <unistd.h>
#include <limits.h>
#endif

#ifdef FB_MACOSX
#include "math.h"
#include "stdio.h"
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#include <mach-o/dyld.h>
/* Any change here should be reflected in pjsua.h */
#define HTTP_PROXY_MAX_DOMAIN_LENGTH 256
#define HTTP_PROXY_MAX_UNAME_LENGTH 128
#define HTTP_PROXY_MAX_PWD_LENGTH 128

#define BJN_CERT_ID     "Blue Jeans Network, Inc. (HE4P42JBGN)"
#define BJN_CERT_SHA1   "8A 40 51 37 9A D4 FF 4D 44 DA 75 04 E2 38 31 10 EF 77 C9 2D"
#endif

#ifdef WIN32
static wchar_t *strVal = L"Uninstall";
#define BJN_CERT_ID         "Blue Jeans Network"
#define BJN_CERT_SHA1       "C7 2C EC D5 9C F9 E0 42 24 01 6A 65 17 62 51 82 DB 1F 83 36"
#define BJN_CERT_SHA1_NEW   "A5 32 97 1B 97 F7 AA 99 92 25 7D 9B AA 63 8B 24 FC B4 12 0B"
#endif

#define CONFIG_XML          "fiberapp.config"
#define EFFECT_FILE         "d3d9_effect.fx"
#define AUTO_UPDATER_EXE    "bjnupdateplugin.exe"
#define DOWNLOAD_DIRECTORY  "downloads"
#define PCAP_DIRECTORY      "pcaps"
#define MAC_PLUGIN_PATH     "/Library/Internet Plug-Ins"
#define MAC_DIAG_REPORT     "/Library/Logs/DiagnosticReports/"
#define LINUX_RPM_PLUGIN_PATH   "/usr/lib64/mozilla/plugins"
#define LINUX_DEB_PLUGIN_PATH   "/usr/lib/mozilla/plugins"
#define PLUGIN_STR          "plugin"
#define MAC_RESOURNCES_DIR  "Contents/Resources/"
#define MAC_EXEC_DIR        "/Contents/MacOS"

#define OLD_COMPANY_NAME    "BlueJeans"
#define OLD_APP_NAME        "Skinny"

#define ENDPOINT_NAME       "Browser"

#define APP_NAME_LINUX      "bluejeans-bin"

#define FBSTRING_DirectoryName	"BlueJeans"
#define FBSTRING_PluginLogDir	"logs"
#define FBSTRING_MasterPluginFilename "Unused"


static const char vSiteList[][50] = { "bluejeans.com", "bluejeansnet.com", "bluejeansdev.com" };
std::vector<std::string> vsiteList(vSiteList, vSiteList + sizeof(vSiteList) / sizeof(vSiteList[0]));

static const char invalidSiteList[][50] = { "bluejeans.com.", "bluejeansnet.com.", "bluejeansdev.com." };
std::vector<std::string> invsiteList(invalidSiteList, invalidSiteList + sizeof(invalidSiteList) / sizeof(invalidSiteList[0]));
static std::string guid_;

namespace BJN
{
	std::string proxyHostStr;
	std::string proxyPortStr;
	std::string proxyAuthUsername;
	std::string proxyAuthPasswordDONOTPRINT;
	static std::string pluginFileSystemPath;

#ifdef FB_WIN
	struct CertInfoPlugin //to store certificate info of plugin-installers
	{
		HCERTSTORE hStore;
		HCRYPTMSG hMsg;
		PCMSG_SIGNER_INFO pSignerInfo;
		PCCERT_CONTEXT pCertContext;
		CHAR* ownerName;
		BYTE *thumbPrint; //SHA1 hash

		CertInfoPlugin()
		{
			hStore = NULL;
			hMsg = NULL;
			pSignerInfo = NULL;
			pCertContext = NULL;
			ownerName = NULL;
			thumbPrint = NULL;
		}
		~CertInfoPlugin()
		{
			if (thumbPrint)
			{
				delete[] thumbPrint;
				thumbPrint = NULL;
			}
			if (ownerName)
			{
				delete[] ownerName;
				ownerName = NULL;
			}
			if (pCertContext)
			{
				CertFreeCertificateContext(pCertContext);
				pCertContext = NULL;
			}
			if (pSignerInfo)
			{
				delete[] pSignerInfo;
				pSignerInfo = NULL;
			}
			if (hStore)
			{
				CertCloseStore(hStore, 0);
				hStore = NULL;
			}
			if (hMsg)
			{
				CryptMsgClose(hMsg);
				hMsg = NULL;
			}
		}
	};
#endif
#ifndef FB_WIN
	void handler(int sig) {
		void *array[10];
		size_t size;
		std::string pidPath = BJN::getBjnLogfilePath() + "skinnyPluginPid";
		::unlink(pidPath.c_str());

		// get void*'s for all entries on the stack
		size = backtrace(array, 10);

		// print out all the frames to stderr
		syslog(LOG_ERR, "Bluejeans Error: signal %d\n", sig);
		LOG(LS_ERROR) << "Error: signal " << sig;
		talk_base::LogMessage::clearLog();
		char** funcNames = backtrace_symbols(array, size);
		for (int i = 0; i < size; i++) {
			syslog(LOG_ERR, "%s\n", funcNames[i]);
			LOG(LS_ERROR) << funcNames[i];
		}
		free(funcNames);
		signal(sig, SIG_DFL);
		kill(getpid(), sig);
	}

	void registerSignalhandler()
	{
		signal(SIGQUIT, handler);
		signal(SIGILL, handler);
		signal(SIGTRAP, handler);
		signal(SIGABRT, handler);
		signal(SIGFPE, handler);
		signal(SIGBUS, handler);
		signal(SIGSEGV, handler);
		signal(SIGSYS, handler);
	}
#endif 


	std::string getBjnLogDirName()
	{
		return (LOGS_DIRECTORY);
	}
	std::string getMasterName()
	{
		return ("");
	}

	void quoted_file_path(std::string &fileName)
	{
		fileName = "\"" + fileName + "\"";
	}

#ifdef FB_WIN
	bool verifyBinaryCertificate(const std::string &pkgPath)
	{
		bool bSuccess = false;
		CertInfoPlugin certInfoPlugin;
		DWORD _size = 0;
		CERT_INFO CertInfo;
		string thumbPrintStr;
		WCHAR szFileName[MAX_PATH];

		mbstowcs(szFileName, pkgPath.c_str(), MAX_PATH); //CryptQueryObject requires unicode file path

		if (!CryptQueryObject(CERT_QUERY_OBJECT_FILE, szFileName, CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
			CERT_QUERY_FORMAT_FLAG_BINARY, 0, NULL, NULL, NULL, &certInfoPlugin.hStore,
			&certInfoPlugin.hMsg, NULL))
			return false;

		if (!CryptMsgGetParam(certInfoPlugin.hMsg, CMSG_SIGNER_INFO_PARAM, NULL, NULL, &_size))
			return false;

		certInfoPlugin.pSignerInfo = new(nothrow)CMSG_SIGNER_INFO[_size];
		if (!certInfoPlugin.pSignerInfo || !CryptMsgGetParam(certInfoPlugin.hMsg, CMSG_SIGNER_INFO_PARAM, NULL,
			(PVOID)certInfoPlugin.pSignerInfo, &_size))
			return false;

		CertInfo.Issuer = certInfoPlugin.pSignerInfo->Issuer;
		CertInfo.SerialNumber = certInfoPlugin.pSignerInfo->SerialNumber;
		certInfoPlugin.pCertContext = CertFindCertificateInStore(certInfoPlugin.hStore,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, NULL,
			CERT_FIND_SUBJECT_CERT, (PVOID)&CertInfo, NULL);
		if (!certInfoPlugin.pCertContext)
			return false;
		_size = CertGetNameStringA(certInfoPlugin.pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, 0, 0, 0);
		if (!_size)
			return false;
		//Retrieving the owner of the certificate
		certInfoPlugin.ownerName = new(nothrow)CHAR[_size];
		if (!certInfoPlugin.ownerName ||
			!CertGetNameStringA(certInfoPlugin.pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE,
			NULL, NULL, certInfoPlugin.ownerName, _size))
			return false;

		if (strcmp(certInfoPlugin.ownerName, BJN_CERT_ID) != 0) // Owner name is not matching
			return false;

		if (!CryptHashCertificate(0, 0, 0, certInfoPlugin.pCertContext->pbCertEncoded,
			certInfoPlugin.pCertContext->cbCertEncoded, 0, &_size))
			return false;
		//Retrieving the SHA1 hash
		certInfoPlugin.thumbPrint = new(nothrow)BYTE[_size];
		if (!certInfoPlugin.thumbPrint || !CryptHashCertificate(0, 0, 0, certInfoPlugin.pCertContext->pbCertEncoded,
			certInfoPlugin.pCertContext->cbCertEncoded,
			certInfoPlugin.thumbPrint, &_size))
			return false;

		char buf[4] = "";
		for (DWORD n = 0; n < _size; n++)
		{
			sprintf_s(buf, sizeof(buf), "%02X ", certInfoPlugin.thumbPrint[n]);
			thumbPrintStr.append(buf);
		}
		if ((0 == thumbPrintStr.compare(0, thumbPrintStr.size() - 1, BJN_CERT_SHA1))
			|| (0 == thumbPrintStr.compare(0, thumbPrintStr.size() - 1, BJN_CERT_SHA1_NEW)))
		{
			bSuccess = true;
		}

		return bSuccess;
	}
#endif

#ifdef FB_MACOSX
	bool verifyBinaryCertificate(const std::string &pkgPath)
	{
		bool foundId = false;
		bool foundSHA1 = false;
		bool ret = false;
		char buff[512] = "";
		FILE *in = NULL;
		std::string command = "pkgutil --check-signature " + pkgPath;

		if (!(in = popen(command.c_str(), "r"))) {
			return ret;
		}

		while (fgets(buff, sizeof(buff), in) != NULL) {
			if (strstr(buff, BJN_CERT_ID)) {
				foundId = true;
			}
			if (strstr(buff, BJN_CERT_SHA1)) {
				foundSHA1 = true;
			}
			if (foundId && foundSHA1) {
				ret = true;
				break;
			}
		}
		pclose(in);
		return ret;
	}

	bool verifyAppCertificate(const std::string& appPath)
	{
		bool foundId = false;
		bool ret = false;
		char buff[PATH_MAX] = "";
		FILE *in = NULL;
		// command to launch codesign app, 2>&1 is required to redirect the stderr to stdout
		// looks like codesign is outputting contents on stderr rather than stdout!
		std::string command = "codesign -vv -d \"" + appPath + "\" 2>&1";

		if (!(in = popen(command.c_str(), "r"))) {
			LOG(LS_INFO) << "Mac app name command: popen failed";
			return ret;
		}

		while (fgets(buff, PATH_MAX, in) != NULL) {
			if (strstr(buff, BJN_CERT_ID)) {
				foundId = true;
			}
			if (foundId) {
				ret = true;
				break;
			}
		}
		pclose(in);
		return ret;
	}

	std::string getMacPluginBasePath()
	{
		return FB::System::getHomeDirPath() + "/" + MAC_PLUGIN_PATH;
	}
	std::string getDiagReportPath()
	{
		std::string path = FB::System::getHomeDirPath() + "/" + MAC_DIAG_REPORT;
		for (unsigned i = 0; i < path.length(); i++)
		{
			if (path.at(i) == ' ')
			{
				path.insert(i, "\\");
				++i;
			}
		}
		return path;
	}
#endif

#ifdef FB_X11
	std::string getLinuxPluginBasePath()
	{
		if (talk_base::IsRedHat())
			return LINUX_RPM_PLUGIN_PATH;
		else
			return LINUX_DEB_PLUGIN_PATH;
	}
#endif
	std::string getLocalAppDataPath(std::string param)
	{
		return getenv("LOCALAPPDATA");
	}

	std::string getBjnLocalAppCompanyDir()
	{
#ifdef FB_WIN
		return getLocalAppDataPath("") + "\\" + FBSTRING_DirectoryName;
#elif FB_MACOSX
		return FB::System::getLocalAppDataPath("") + "/" + FBSTRING_DirectoryName;
#elif FB_X11
		return FB::System::getLocalAppDataPath("") + "/.config/" + FBSTRING_DirectoryName;
#endif
	}
#ifdef FB_WIN
	std::string getBjnCarmelAppCompanyDir()
	{
		return getLocalAppDataPath("") + FBSTRING_DirectoryName;
	}
#endif

	void setPluginPath(std::string _path)
	{
		pluginFileSystemPath = _path;
	}

	std::string getBjnAppDataPath()
	{

#ifdef FB_WIN
		TCHAR buffer[MAX_PATH];
		GetModuleFileName(NULL, buffer, MAX_PATH);
		CT2CA pszPath(buffer);
		std::string path(pszPath);
		std::string::size_type pos = path.find_last_of("\\/");
		return path.substr(0, pos);

		//		assert(!pluginFileSystemPath.empty());
//		std::string pluginPath = pluginFileSystemPath;
//		std::size_t index = pluginPath.rfind("\\");
//		if (index != std::string::npos)
//		{
//			pluginPath = pluginPath.substr(0, index);
//		}
//		else
//		{
//			LOG(LS_ERROR) << "Invalid plugin path: " << pluginPath;
//		}
//		return pluginPath;

#elif FB_MACOSX

		assert(!pluginFileSystemPath.empty());
		std::string pluginPath = pluginFileSystemPath;
		std::size_t index = pluginPath.find(MAC_EXEC_DIR);
		if (index != std::string::npos)
		{
			pluginPath = pluginPath.substr(0, index);
		}
		else
		{
			LOG(LS_ERROR) << "Invalid plugin path: " << pluginPath;
		}
		return pluginPath;

#elif FB_X11
		return getLinuxPluginBasePath() + "/" + "." + getMasterName();
#endif
	}


	std::string getBjnLocalAppDataPath()
	{
#ifdef FB_WIN
		return getBjnLocalAppCompanyDir() + "\\";
#else
		return getBjnLocalAppCompanyDir() + "/" + FBSTRING_PluginLogDir;
#endif
	}


	std::string getBjnConfigFile()
	{
		std::string path;
#ifdef FB_MACOSX
		path = getBjnAppDataPath() + "/" + MAC_RESOURNCES_DIR + CONFIG_XML;
#elif FB_WIN
		path = getBjnAppDataPath() + "\\" + CONFIG_DIRECTORY + "\\" + CONFIG_XML;
#elif FB_X11
		path = getBjnAppDataPath() + "/" + CONFIG_DIRECTORY + "/" + CONFIG_XML;
#endif
		return path;
	}

	std::string getBjnConfigPath()
	{
		//return FB::System::getTempPath();
		std::string path;
#ifdef FB_MACOSX
		path = getBjnAppDataPath() + "/" + MAC_RESOURNCES_DIR;
#elif FB_WIN
		path = getBjnAppDataPath() + "\\" + CONFIG_DIRECTORY ;
#elif FB_X11
		path = getBjnAppDataPath() + "/" + CONFIG_DIRECTORY;
#endif
		return path;
	}


	std::string getBjnEffectFile()
	{
		std::string path;
#ifdef FB_WIN
		path = getBjnAppDataPath() + "\\" + CONFIG_DIRECTORY + "\\" + EFFECT_FILE;
#endif
		return path;
	}

	std::string getBjnLogfilePath()
	{
		std::string path;
#ifdef FB_WIN
		path = getBjnLocalAppDataPath() + "\\Prism\\" + LOGS_DIRECTORY;
#else
		path = getBjnLocalAppDataPath() + "/" + LOGS_DIRECTORY;
#endif
		return path;
	}

	std::string getBjnLogfilePath(bool withGuid)
	{
#ifdef FB_WIN
		return getBjnLogfilePath() + "\\" + guid_ + "_";
#else
		return getBjnLogfilePath() + "/" + guid_ + "_";
#endif
	}

	void setGuid(std::string guid)
	{
		guid_ = guid;
	}

	std::string getGuid()
	{
		return guid_;
	}

	int getBjnLogDirectoryLevel()
	{
		return LOGS_DIRECTORY_LEVEL;
	}


	std::string getBjnDownloadPath()
	{
		std::string path;
#ifdef FB_WIN
		path = getBjnLocalAppDataPath() + "\\" + DOWNLOAD_DIRECTORY;
#else
		path = getBjnLocalAppDataPath() + "/" + DOWNLOAD_DIRECTORY;
#endif
		return path;
	}


	std::string getBjnPcapPath()
	{
		std::string path;
#ifdef FB_WIN
		path = getBjnLocalAppDataPath() + "\\" + PCAP_DIRECTORY + "\\";
#else
		path = getBjnLocalAppDataPath() + "/" + PCAP_DIRECTORY + "/";
#endif
		if (!(boost::filesystem::exists(path)))
		{
			boost::filesystem::create_directory(path);
		}
		return path;
	}


	std::string getBjnAutoUpdaterEXE()
	{
		std::string path;
#if FB_WIN
		path = getBjnAppDataPath() + "\\" + AUTO_UPDATER_EXE;
#endif
		return path;
	}


	std::string getBjnOldLocalAppDataPath()
	{
		std::string path;
#ifdef FB_WIN
		path = getLocalAppDataPath("") + "\\" + OLD_COMPANY_NAME;
#else 
		path = getLocalAppDataPath("") + "/" + OLD_COMPANY_NAME;
#endif
		return path;
	}

	bool hasEnding(std::string const &fullString, std::string const &ending)
	{
		if (fullString.length() >= ending.length()) {
			return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
		}
		else {
			return false;
		}
	}

	bool checkForValidSite(std::string location)
	{
		bool  isValidList = false;
		bool  isInValidList = false;
		bool  allowSite = true;
		std::string actualLocation;
		std::string tempStr;

		for (unsigned int i = 0; i<vsiteList.size(); i++)
		{
			if (location.find(vsiteList[i]) != std::string::npos) {
				tempStr.assign(location.begin() + location.find(vsiteList[i]), location.end());
				actualLocation.assign(tempStr.begin(), tempStr.begin() + tempStr.find_first_of('/'));
				if (hasEnding(actualLocation, vsiteList[i]))
				{
					isValidList = true;
					break;
				}
				tempStr.clear();
				actualLocation.clear();
			}
		}
		for (unsigned int j = 0; j<invsiteList.size(); j++)
		{
			if (location.find(invsiteList[j]) != std::string::npos)
			{
				isInValidList = true;
				break;
			}
		}
		if ((false == isInValidList) && (true == isValidList))
		{
			allowSite = true;
		}
		else
		{
			allowSite = false;
		}
		return allowSite;
	}

	bool checkForBlueJeansApp()
	{
		bool bRet = false;
#ifdef FB_WIN
		char szFileName[MAX_PATH + 1];
		DWORD dwRet = GetModuleFileNameA(NULL, szFileName, MAX_PATH);
		if (0 != dwRet) {
			if (false == verifyBinaryCertificate(szFileName)) {
				LOG(LS_INFO) << "Win app name:" << szFileName << " failed to verify application signature.";
			}
			else {
				bRet = true;
				LOG(LS_INFO) << "Win app name:" << szFileName << " signature verification success.";
			}
		}
		else {
			LOG(LS_WARNING) << "GetModuleFileName failed: " << GetLastError();
		}
#elif FB_MACOSX
		char szExePath[PATH_MAX];
		uint32_t len = sizeof(szExePath);
		if (_NSGetExecutablePath(szExePath, &len) != 0) {
			szExePath[0] = '\0'; // buffer too small (!)
		}
		else {
			// resolve symlinks, ., .. if possible
			char *szCanonicalPath = realpath(szExePath, NULL);
			if (szCanonicalPath != NULL) {
				if (verifyAppCertificate(std::string(szCanonicalPath))) {
					bRet = true;
					LOG(LS_INFO) << "Mac app name:" << szExePath << " signature verification success.";
				}
				else {
					LOG(LS_INFO) << "Mac app name:" << szCanonicalPath << " failed to verify signature.";
				}
				free(szCanonicalPath);
			}
			else {
				// Some issue!
				LOG(LS_WARNING) << "realpath API failed, err	:" << errno;
				szExePath[0] = '\0'; // buffer too small (!)
			}
		}
#elif FB_X11
		char szExePath[PATH_MAX];
		ssize_t nLen = ::readlink("/proc/self/exe", szExePath, sizeof(szExePath));
		if (-1 == nLen || sizeof(szExePath) == nLen) {
			nLen = 0;
		}
		szExePath[nLen] = '\0';
		if (boost::filesystem::path(szExePath).filename().string() == APP_NAME_LINUX) {
			bRet = true;
		}
		LOG(LS_INFO) << "Linux app name:" << szExePath << ";bRet:" << bRet;
#endif
		return bRet;
	}

#ifdef FB_WIN
	DWORD autoProxyDetect(WINHTTP_AUTOPROXY_OPTIONS AutoProxyOptions,
		HINTERNET session_handle,
		wchar_t *buf,
		WINHTTP_PROXY_INFO &ProxyInfo)
	{
		// Use DHCP and DNS-based auto-detection.
		AutoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP |
			WINHTTP_AUTO_DETECT_TYPE_DNS_A;

		// If obtaining the PAC script requires NTLM/Negotiate
		// authentication, then automatically supply the client
		// domain credentials.
		AutoProxyOptions.fAutoLogonIfChallenged = FALSE;

		if (!WinHttpGetProxyForUrl(session_handle,
			buf,
			&AutoProxyOptions,
			&ProxyInfo))
		{
			if (GetLastError() == ERROR_WINHTTP_LOGIN_FAILURE) {
				AutoProxyOptions.fAutoLogonIfChallenged = TRUE;
				if (!WinHttpGetProxyForUrl(session_handle,
					buf, &AutoProxyOptions,
					&ProxyInfo)) {
					DWORD errorCode = GetLastError();
					LOG(LS_ERROR) << "WinHttpGetProxyForUrl Error: " << errorCode;
				}
			}
			else {
				DWORD errorCode = GetLastError();
				LOG(LS_ERROR) << "WinHttpGetProxyForUrl Error: " << errorCode;
			}
		}
		return GetLastError();
	}

	typedef BOOL(CALLBACK *pfnInternetInitializeAutoProxyDll)(
		DWORD dwVersion,
		LPSTR lpszDownloadedTempFile,
		LPSTR lpszMime,
		AutoProxyHelperFunctions* lpAutoProxyCallbacks,
		LPAUTO_PROXY_SCRIPT_BUFFER lpAutoProxyScriptBuffer
		);

	typedef BOOL(CALLBACK *pfnInternetGetProxyInfo)(
		LPCSTR lpszUrl,
		DWORD dwUrlLength,
		LPSTR lpszUrlHostName,
		DWORD dwUrlHostNameLength,
		LPSTR* lplpszProxyHostName,
		LPDWORD lpdwProxyHostNameLength
		);

	typedef BOOL(CALLBACK *pfnInternetDeInitializeAutoProxyDll)(
		LPSTR lpszMime,
		DWORD dwReserved
		);

	/* We look up all of these dynamically since we don't have the import libs */
	HMODULE   hJSProxyDLL = NULL;
	pfnInternetInitializeAutoProxyDll   pInternetInitAutoProxyDll;
	pfnInternetDeInitializeAutoProxyDll pInternetDeInitAutoProxyDll;
	pfnInternetGetProxyInfo             pInternetGetProxyInfo;

	bool winInetInit(void)
	{

		if (!hJSProxyDLL) {
			hJSProxyDLL = LoadLibrary(TEXT("jsproxy.dll"));

			pInternetInitAutoProxyDll = (pfnInternetInitializeAutoProxyDll)GetProcAddress(hJSProxyDLL, "InternetInitializeAutoProxyDll");
			pInternetDeInitAutoProxyDll = (pfnInternetDeInitializeAutoProxyDll)GetProcAddress(hJSProxyDLL, "InternetDeInitializeAutoProxyDll");
			pInternetGetProxyInfo = (pfnInternetGetProxyInfo)GetProcAddress(hJSProxyDLL, "InternetGetProxyInfo");

			if (!pInternetInitAutoProxyDll || !pInternetDeInitAutoProxyDll || !pInternetGetProxyInfo){
				LOG(LS_ERROR) << "Failed to lookup required functions";
				return false;
			}
		}
		return true;
	}

	char * winInetProxyServerForUrl(char *url, LPWSTR pacFile)
	{
		char hostname[MAX_PATH];
		char buffer[MAX_PATH];
		LPSTR proxyInfo = NULL;
		DWORD proxyLength = 0;
		PARSEDURL pu;
		pu.cbSize = sizeof(pu);

		if (!pacFile) {
			LOG(LS_WARNING) << "Pac file provided is not valid";
			return NULL;
		}

		HRESULT hr = ParseURL(pacFile, &pu);
		if (SUCCEEDED(hr)) {
			if (pu.nScheme != URL_SCHEME_FILE) {
				LOG(LS_ERROR) << "Not a local file location";
				return NULL;
			}
		}

		wcstombs(buffer, pu.pszSuffix, pu.cchSuffix);
		buffer[pu.cchSuffix] = '\0';
		if (!winInetInit()) return NULL;

		PARSEDURLA pua;
		pua.cbSize = sizeof(pua);
		hr = ParseURLA(url, &pua);
		if (SUCCEEDED(hr)) {
			//For http protocoal suffix starts with //
			memcpy(hostname, pua.pszSuffix + 2, pua.cchSuffix - 2);
			hostname[pua.cchSuffix - 2] = '\0';
		}
		else {
			LOG(LS_ERROR) << "Can't parse url to get host name" << url;
			return NULL;
		}

		/* Initialize autoproxy functions */
		if (!pInternetInitAutoProxyDll(0, buffer, NULL, NULL, NULL)) {
			LOG(LS_ERROR) << "InternetInitializeAutoProxyDll failed " << GetLastError();
			return NULL;
		}

		/* get proxy information */
		if (!pInternetGetProxyInfo(url, strlen(url),
			hostname, strlen(hostname),
			&proxyInfo, &proxyLength)) {
			LOG(LS_ERROR) << "InternetGetProxyInfo failed " << GetLastError();
			return NULL;
		}

		/* close autoproxy */
		if (!pInternetDeInitAutoProxyDll(NULL, 0)) {
			LOG(LS_ERROR) << "InternetDeInitAutoProxyDll failed " << GetLastError();
		}
		return proxyInfo;
	}

#endif

#ifdef FB_MACOSX
	static void getPassword(const char *server, const int port, char *username, char *pwd_DONT_PRINT)
	{
		UInt32 pwLength = 0;
		SecKeychainItemRef item;
		void *password = NULL;

		OSStatus err = SecKeychainFindInternetPassword(NULL, (UInt32)strlen(server), server, 0,
			NULL, (UInt32)strlen(username), username, 0, NULL,
			port, kSecProtocolTypeAny, kSecAuthenticationTypeAny,
			&pwLength, &password, &item);

		if (err == errSecSuccess) {
			if (pwLength > 0) {
				/* TODO: Windows and non-Windows. Currently this is just OSX */
				snprintf(pwd_DONT_PRINT, pwLength + 1, "%s", (char *)password);
			}
		}
		else {
			CFStringRef error = SecCopyErrorMessageString(err, NULL);
			LOG(LS_INFO) << "Proxy SecKeychainFindInternetPassword ERROR: " << CFStringGetCStringPtr(error,
				kCFStringEncodingASCII);
		}
	}

	static const char *extractString(CFStringRef someString)
	{
		return (CFStringGetCStringPtr(someString, kCFStringEncodingMacRoman));
	}

	static void matchUserName(const void *value, char *username)
	{
		CFDictionaryRef dict = (CFDictionaryRef)value;
		CFStringRef acct = (CFStringRef)CFDictionaryGetValue(dict, kSecAttrAccount);
		strncpy(username, extractString(acct), strlen(extractString(acct)));
	}

	static void getUsername(const char *server, const int port, char *username)
	{
		{
			CFStringRef cfServerStr = CFStringCreateWithCString(NULL, server,
				kCFStringEncodingMacRoman);
			CFNumberRef cfPortNum = CFNumberCreate(NULL, kCFNumberSInt32Type, (void *)&port);

			/*Find username*/
			CFMutableDictionaryRef query = CFDictionaryCreateMutable(kCFAllocatorDefault, 3,
				&kCFTypeDictionaryKeyCallBacks,
				&kCFTypeDictionaryValueCallBacks);

			CFDictionaryAddValue(query, kSecReturnAttributes, kCFBooleanTrue);
			CFDictionaryAddValue(query, kSecMatchLimit, kSecMatchLimitOne);
			CFDictionaryAddValue(query, kSecClass, kSecClassInternetPassword);
			CFDictionaryAddValue(query, kSecAttrServer, cfServerStr);
			CFDictionaryAddValue(query, kSecAttrPort, cfPortNum);

			CFArrayRef result = nil;

			SecItemCopyMatching(query, (CFTypeRef*)&result);
			if (result != NULL) {
				matchUserName(result, username);
			}
			else {
				username[0] = '\0';
			}
		}
	}

	static bool getCredentials(const char *server, const int port,
		char *username, char *pwd_DONT_PRINT)
	{
		bool retCode = true;

		LOG(LS_INFO) << "Proxy: Looking for credentials for server "
			<< server << " and port " << port;

		getUsername(server, port, username);

		if (username[0] == '\0') {
			/* No username. Now we prompt the user for credentials */
			LOG(LS_INFO) << "Proxy: Credentials required but could not read it from keychain. Let's ask the user";
			retCode = false;
		}
		else {
			getPassword(server, port, username, pwd_DONT_PRINT);
			if (pwd_DONT_PRINT[0] == '\0') {
				retCode = false;
			}
		}
		return retCode;
	}

	static void setCredentials(const char *server, const int port,
		const char *username, const char *pwd)
	{
		OSStatus err;

		err = SecKeychainAddInternetPassword(NULL,
			strlen(server), server,
			0, NULL,
			strlen(username), username,
			0, NULL,
			port,
			kSecProtocolTypeHTTPS,
			kSecAuthenticationTypeAny,
			strlen(pwd), pwd,
			NULL);

		if (err == errSecSuccess) {
			LOG(LS_INFO) << "Proxy: Successfully wrote credentials into keychain";
		}
		else if (err == errSecDuplicateItem) {
			UInt32 pwLength = 0;
			void *pwBytes;
			SecKeychainItemRef item = NULL;

			LOG(LS_INFO) << "Proxy: Entry exists. Attempting to update";
			err = SecKeychainFindInternetPassword(NULL, strlen(server), server, 0, NULL,
				(UInt32)strlen(username), username, 0, NULL,
				port, kSecProtocolTypeAny, kSecAuthenticationTypeAny,
				&pwLength, &pwBytes, &item);
			if (err == errSecSuccess) {
				if (item) {
					if (SecKeychainItemModifyAttributesAndData(item, NULL, strlen(pwd), (void *)pwd)
						== errSecSuccess) {
						LOG(LS_INFO) << "Proxy: Successfully updated keychain entry";
						CFRelease(item);
					}
				}
			}
		}
		else {
			CFStringRef error = SecCopyErrorMessageString(err, NULL);
			LOG(LS_INFO) << "Proxy: SecKeychainAddInternetPassword ERROR: " <<
				CFStringGetCStringPtr(error, kCFStringEncodingMacRoman);
		}
	}
#endif

	static CURL *initCurlHandleAndParams(std::string url, std::string proxyStr)
	{
		CURL *curl_handle = curl_easy_init();

		if (curl_handle) {
			curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl_handle, CURLOPT_PROXY, proxyStr.c_str());
			curl_easy_setopt(curl_handle, CURLOPT_HTTPPROXYTUNNEL, 1);
			curl_easy_setopt(curl_handle, CURLOPT_CONNECT_ONLY, 1);
		}

		return curl_handle;
	}

	static bool isProxyAuthEnabled(std::string url, std::string proxyStr)
	{
		/* We attempt a connect through the discovered proxy and see
		* if the proxy expects authentication - 407 response
		*/
		CURL *curl_handle = NULL;
		bool retCode = false;
		long response = -1;

		curl_handle = initCurlHandleAndParams(url, proxyStr);

		if (curl_handle) {
			if (curl_easy_perform(curl_handle) != 0) {
				curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CONNECTCODE, &response);
				if (response == 407) {
					LOG(LS_INFO) << "For " << url << " Proxy sent 407 response. We will need auth credentials.";
					retCode = true;
				}
				else {
					LOG(LS_INFO) << "For " << url << " Proxy sent Non-407 response: " << response;
				}
			}
			else {
				LOG(LS_INFO) << "For " << url << " Proxy doesn't require authentication";
			}

			curl_easy_cleanup(curl_handle);
		}
		return retCode;
	}

	bool isProxyAuthCredentialsValid(std::string url, std::string proxyStr,
		std::string &proxyAuthStr, bool saveToKeychain, const char *username,
		const char *passwordDONOTPRINT)
	{
		CURL *curl_handle = NULL;
		bool retCode = false;
		long response = -1;

		LOG(LS_INFO) << __FUNCTION__;

		curl_handle = initCurlHandleAndParams(url, proxyStr);

		if (curl_handle) {
			curl_easy_setopt(curl_handle, CURLOPT_PROXYAUTH, CURLAUTH_NTLM);
			curl_easy_setopt(curl_handle, CURLOPT_PROXYUSERPWD, proxyAuthStr.c_str());

			if (curl_easy_perform(curl_handle) != 0) {
				curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CONNECTCODE, &response);
				if (response == 200) {
					LOG(LS_INFO) << "For " << url << " Proxy sent 200 response.";
					retCode = true;
				}
				else {
					LOG(LS_INFO) << "For " << url << " Proxy sent error response: " << response;
				}
			}
			else {
				LOG(LS_INFO) << "For " << url << " Proxy doesn't require authentication";
				retCode = true;
			}

			curl_easy_cleanup(curl_handle);

#ifdef FB_MACOSX
			if ((retCode == true) && (saveToKeychain == true)) {
				if (username != NULL && passwordDONOTPRINT != NULL) {
					LOG(LS_INFO) << "Proxy: Attempting to set credentials in keychain";
					setCredentials(proxyHostStr.c_str(), atoi(proxyPortStr.c_str()),
						username, passwordDONOTPRINT);
				}
			}
#endif
		}
		return retCode;
	}

	bool detectProxyAuthSettings(const BrowserHostPtr host, std::string &proxyHostStrRef,
		std::string &proxyPortStrRef, std::string &proxyAuthStrRef)
	{
#ifdef FB_MACOSX
		/* Find host */
		char username[HTTP_PROXY_MAX_DOMAIN_LENGTH + HTTP_PROXY_MAX_UNAME_LENGTH],
			pwd_DONT_PRINT[HTTP_PROXY_MAX_PWD_LENGTH];
		bool retCode = false;
		memset(username, 0, HTTP_PROXY_MAX_DOMAIN_LENGTH + HTTP_PROXY_MAX_UNAME_LENGTH);
		memset(pwd_DONT_PRINT, 0, HTTP_PROXY_MAX_PWD_LENGTH);

		if (getCredentials(proxyHostStrRef.c_str(), atoi(proxyPortStrRef.c_str()), username, pwd_DONT_PRINT) == true) {
			proxyAuthStrRef.assign(username);
			proxyAuthStrRef.append(":");
			proxyAuthStrRef.append(pwd_DONT_PRINT);
			retCode = true;
		}
		else {
			retCode = false;
		}

		return retCode;
#else
		return false;
#endif
	}

	bool detectProxySettings(std::string url, const BrowserHostPtr host,
		std::string &proxyStr, std::string &proxyInfoStr,
		std::string &proxyAuthStr)
	{
		bool isProxyAuthReq = false;
		return (detectProxySettings(url, host, proxyStr, proxyInfoStr, proxyAuthStr, isProxyAuthReq));
	}

	bool detectProxySettings(std::string url, const BrowserHostPtr host,
		std::string &proxyStr, std::string &proxyInfoStr,
		std::string &proxyAuthStr, bool &isProxyAuthReq)
	{
		size_t colpos = 0, endofhostpos = 0;
		std::string hostaddr;
		if (url.find("sip") != std::string::npos)
		{
			if (url.find("sips") || url.find("tls"))
			{
				hostaddr.append("https://");
			}
			else
			{
				hostaddr.append("http://");
			}
			if ((colpos = url.find('@')) != std::string::npos)
			{
				colpos++;
				if ((endofhostpos = url.find(';', colpos)) != std::string::npos)
				{
					hostaddr.append(url.substr(colpos, (endofhostpos - colpos)));
				}
				else
				{
					hostaddr.append(url.substr(colpos));
				}
			}
			else
			{
				hostaddr.clear();
			}
		}
		else {
			hostaddr = url;
		}
#if 0
		//AN - Come back and revisit this later
		if (!hostaddr.empty())
		{
			std::map<std::string, std::string>settingsMap;
			if (host->DetectProxySettings(settingsMap, hostaddr.c_str()))
			{
				LOG(LS_INFO) << "Proxy Settings through Firebreath";
				for (std::map<std::string, std::string>::iterator it = settingsMap.begin(); it != settingsMap.end(); ++it)
				{
					if (!(it->first).compare("hostname"))
					{
						proxyStr.assign(it->second);
						proxyHostStr.assign(it->second);
					}
					else if (!(it->first).compare("port"))
					{
						proxyStr.append(":");
						proxyStr.append((it->second));
						proxyPortStr.assign(it->second);
					}
					else if (!(it->first).compare("type") && proxyInfoStr.find((it->second).append(":")) == std::string::npos)
					{
						proxyInfoStr.append("[");
						proxyInfoStr.append(it->second);
						proxyInfoStr.append("1]/");
					}
				}
				if (!proxyStr.empty())
				{
					LOG(LS_INFO) << "Proxy url (FB) " << proxyStr;
					isProxyAuthReq = isProxyAuthEnabled(hostaddr, proxyStr);
					if (isProxyAuthReq == true) {
						/* proxyAuthStr might contain the username and password in clear-text.
						* So, DONT PRINT IT
						*/
						if (detectProxyAuthSettings(host, proxyHostStr, proxyPortStr, proxyAuthStr) == true) {
							if (proxyAuthStr.length() > 0) {
								if (isProxyAuthCredentialsValid(hostaddr, proxyStr, proxyAuthStr, false, NULL, NULL) == false) {
									proxyAuthStr.erase();
								}
							}
						}
						else {
							proxyAuthStr.erase();
						}
					}
					return true;
				}
			}
		}
#endif
		return false;
	}

#ifdef FB_WIN

	bool detectWinProxySettings(std::string url, const BrowserHostPtr host,
		std::string &proxyStr, std::string &proxyInfoStr,
		std::string &proxyAuthStr, bool &isProxyAuthReq)
	{
		LOG(LS_INFO) << __FUNCTION__ << " " << url;
		LOG(LS_INFO) << "Proxy Settings through WinHTTP (Chrome model)";
		char buffer[255] = { 0 };

		WINHTTP_AUTOPROXY_OPTIONS  AutoProxyOptions = { 0 };
		WINHTTP_PROXY_INFO         ProxyInfo = { 0 };
		LPWSTR                     proxyConfig = { 0 };
		WINHTTP_CURRENT_USER_IE_PROXY_CONFIG MyProxyConfig;
		DWORD errorCode;
		if (!WinHttpGetIEProxyConfigForCurrentUser(&MyProxyConfig))
		{
			LOG(LS_INFO) << "WinHttpGetIEProxyConfigForCurrentUser failed with the following error number: " << GetLastError() << endl;
		}
		else
		{
			//no error so check the proxy settings and free any strings
			LOG(LS_INFO) << "Auto Detect is: " << MyProxyConfig.fAutoDetect << endl;
			if (NULL != MyProxyConfig.lpszAutoConfigUrl)
			{
				proxyConfig = MyProxyConfig.lpszAutoConfigUrl;
			}
			else if (NULL != MyProxyConfig.lpszProxy)
			{
				proxyConfig = MyProxyConfig.lpszProxy;
			}
			else if (NULL != MyProxyConfig.lpszProxyBypass)
			{
				proxyConfig = MyProxyConfig.lpszProxyBypass;
			}
		}
		HINTERNET session_handle_ = WinHttpOpen(NULL,
			WINHTTP_ACCESS_TYPE_NO_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0);

		//no error so check the proxy settings and free any strings
		wchar_t *buf = new wchar_t[url.size() + 1];
		mbstowcs(buf, url.c_str(), url.size() + 1);

		// Use auto-detection because the Proxy
		// Auto-Config URL is not known.
		if (MyProxyConfig.fAutoDetect)
		{
			AutoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
			errorCode = autoProxyDetect(AutoProxyOptions, session_handle_, buf, ProxyInfo);
		}
		else if (MyProxyConfig.lpszAutoConfigUrl)
		{
			AutoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
			AutoProxyOptions.lpszAutoConfigUrl = proxyConfig;
			errorCode = autoProxyDetect(AutoProxyOptions, session_handle_, buf, ProxyInfo);
		}
		else if (MyProxyConfig.lpszProxy)
		{
			if (wcstombs(buffer, MyProxyConfig.lpszProxy, sizeof(buffer)) == sizeof(buffer))
				buffer[254] = '\0';
		}

		if (errorCode == ERROR_WINHTTP_UNRECOGNIZED_SCHEME) {
			char *buf = winInetProxyServerForUrl((char *)url.c_str(), proxyConfig);
			if (buf) {
				memcpy(buffer, buf, strlen(buf));
				GlobalFree(buf);
			}
		}
		else if (ProxyInfo.lpszProxy) {
			if (wcstombs(buffer, ProxyInfo.lpszProxy, sizeof(buffer)) == sizeof(buffer))
				buffer[254] = '\0';
		}

		if (strlen(buffer))
		{
			int offset = 0;
			if (strnicmp(buffer, "PROXY ", strlen("PROXY ")) == 0) {
				offset = strlen("PROXY ");
			}
			proxyStr.assign(buffer + offset);
			LOG(LS_INFO) << "proxy url is " << proxyStr << " access type " << ProxyInfo.dwAccessType;
		}
		else {
			if (session_handle_) {
				WinHttpCloseHandle(session_handle_);
				session_handle_ = NULL;
			}
			return false;
		}

		if (session_handle_)
		{
			WinHttpCloseHandle(session_handle_);
			session_handle_ = NULL;
		}

		if (ProxyInfo.lpszProxy) GlobalFree(ProxyInfo.lpszProxy);
		if (ProxyInfo.lpszProxyBypass) GlobalFree(ProxyInfo.lpszProxyBypass);
		if (MyProxyConfig.lpszAutoConfigUrl) GlobalFree(MyProxyConfig.lpszAutoConfigUrl);
		if (MyProxyConfig.lpszProxy) GlobalFree(MyProxyConfig.lpszProxy);
		if (MyProxyConfig.lpszProxyBypass) GlobalFree(MyProxyConfig.lpszProxyBypass);
		delete[]buf;

		if (proxyStr.find_first_not_of(' ') != std::string::npos)
		{
			LOG(LS_INFO) << "Proxy url " << proxyStr;
			/* TODO: Windows equivalent implementation pending */
			//detectProxyAuthSettings(host, proxyStr, proxyAuthStr);
			return true;
		}
		else
		{
			LOG(LS_INFO) << "Null Proxy String... It's no use...";
			return false;
		}
		LOG(LS_ERROR) << "Proxy could not be detected";
		return false;
	}

#endif

	std::string getBjnEndpointName()
	{
		return ENDPOINT_NAME;
	}

#ifdef FB_WIN
	std::string getPluginNameForColor(const std::string& name, PluginColor color)
	{
		// We have 3 plugin colors:
		// RED   - rbjn...
		// GREEN - gbjn...
		// BLUE  = bjn...
		// We convert the input name to BLUE an then add color back
		std::string baseName;
		std::string newName;

		if (name.substr(0, 4) == "rbjn")
		{
			// RED - remove the first character
			baseName = name.substr(1, name.length() - 1);
		}
		else if (name.substr(0, 4) == "gbjn")
		{
			// GREEN  - remove the first character
			baseName = name.substr(1, name.length() - 1);
		}
		else
		{
			// assume BLUE don't modify the string
			baseName = name;
		}

		// now add the specified color
		switch (color)
		{
		case kPluginColorRed:
			newName = "r";
			newName += baseName;
			break;
		case kPluginColorGreen:
			newName = "g";
			newName += baseName;
			break;
		case kPluginColorBlue:
			newName = baseName;
			break;
		default:
			LOG(LS_ERROR) << "Invalid plugin color " << color;
			assert(false);
			break;
		}

		return newName;
	}

	void getRegKeyForMaster(wchar_t* wkey, size_t size, PluginColor color)
	{
#ifdef ENABLE_PER_MACHINE_INSTALL
		std::string key = "HKLM\\Software\\Blue Jeans\\";
#else
		std::string key = "HKCU\\Software\\Blue Jeans\\";
#endif
		key += getPluginNameForColor(FBSTRING_MasterPluginFilename, color);
		mbstowcs(wkey, key.c_str(), size);
	}

	void getRegKey(wchar_t* wkey, size_t size, const std::string& pluginName)
	{
#ifdef ENABLE_PER_MACHINE_INSTALL
		std::string key = "HKLM\\Software\\Blue Jeans\\";
#else
		std::string key = "HKCU\\Software\\Blue Jeans\\";
#endif
		key = key + pluginName;
		mbstowcs(wkey, key.c_str(), size);
	}
#endif

	std::string getVersion(const std::string& pluginName)
	{
#ifdef FB_WIN
#if 0
		std::wstring wversion;
		wchar_t key[128];
		BJN::getRegKey(key, 128, pluginName);
		talk_base::RegKey::GetValue(key, strVal, &wversion);
		std::string str(CW2A(wversion.c_str()));
		return str;
#endif
		return "";
#else
		return FBSTRING_PLUGIN_VERSION;
#endif
	}


	PluginColor getPluginColorFromName(const std::string& pluginName)
	{
		PluginColor color = kPluginColorInvalid;

		// we compare the first kPluginBaseNameCharsToComp chars of the pluginName
		// with the first kPluginBaseNameCharsToComp chars of our known plugin names
		// this compare is case insensitive
		// first check the the pluginName is at least long enough
		if (pluginName.length() >= kPluginBaseNameCharsToComp)
		{
			std::string comp1 = pluginName.substr(0, kPluginBaseNameCharsToComp);
			if (boost::iequals(comp1, std::string(kPluginBaseNameRed, kPluginBaseNameCharsToComp)))
			{
				color = kPluginColorRed;
			}
			else if (boost::iequals(comp1, std::string(kPluginBaseNameGreen, kPluginBaseNameCharsToComp)))
			{
				color = kPluginColorGreen;
			}
			else if (boost::iequals(comp1, std::string(kPluginBaseNameBlue, kPluginBaseNameCharsToComp)))
			{
				color = kPluginColorBlue;
			}
		}

		if (color == kPluginColorInvalid)
		{
			LOG(LS_ERROR) << "Invalid plugin color " << pluginName;
		}
		assert(color != kPluginColorInvalid);

		return color;
	}

	std::string getMasterVersion()
	{
#ifdef FB_WIN
		std::wstring wversion;
		wchar_t key[128];
		BJN::getRegKey(key, 128, FBSTRING_MasterPluginFilename);
		talk_base::RegKey::GetValue(key, strVal, &wversion);
		std::string str(wversion.begin(), wversion.end());
		return str;
#elif FB_MACOSX
		FILE *fp;
		char str[50];
		std::string version;
		std::string cmd("ls ");
		std::string basePath(BJN::getMacPluginBasePath());
		BJN::quoted_file_path(basePath);
		std::string grepVer;

		grepVer = std::string("| grep -i ") + FBSTRING_MasterPluginName + " | cut -d '_' -f2 | cut -d '.' -f1-4";

		cmd += basePath + grepVer;

		LOG(LS_INFO) << "Command is " << cmd;
		fp = popen(cmd.c_str(), "r");
		if (fp == NULL)
		{
			LOG(LS_INFO) << "Failed to get master plugin version";

		}
		else
		{
			if (fgets(str, 50, fp) != NULL)
			{
				LOG(LS_INFO) << "Master plugin version is " << str;
			}
			version = str;
		}
		pclose(fp);
		return version;
#elif FB_X11
		FILE *fp;
		char str[50];
		std::string version;
		std::string cmd("ls ");
		std::string basePath(BJN::getLinuxPluginBasePath());
		BJN::quoted_file_path(basePath);
		std::string grepVer;
		if (0 == strcmp("bjninstallplugin", FBSTRING_PluginName))
		{
			LOG(LS_INFO) << "non Red plugin" << cmd;
			grepVer = "| grep -i bjnplugin | cut -d '_' -f2 | cut -d '.' -f1-4";
		}
		else
		{
			LOG(LS_INFO) << "Red plugin" << cmd;
			grepVer = "| grep -i rbjnplugin | cut -d '_' -f2 | cut -d '.' -f1-4";
		}
		cmd += basePath + grepVer;

		LOG(LS_INFO) << "Command is " << cmd;
		fp = popen(cmd.c_str(), "r");
		if (fp == NULL)
		{
			LOG(LS_INFO) << "Failed to get master plugin version";

		}
		else
		{
			if (fgets(str, 50, fp) != NULL)
			{
				LOG(LS_INFO) << "Master plugin version is " << str;
			}
			version = str;
		}
		pclose(fp);
		return version;
#endif

	}

#ifdef FB_WIN
	std::string getMasterVersionForColor(PluginColor color)
	{
		std::wstring wversion;
		wchar_t key[128];
		BJN::getRegKeyForMaster(key, 128, color);
		talk_base::RegKey::GetValue(key, strVal, &wversion);
		std::string str(wversion.begin(), wversion.end());
		return str;
	}
#endif

	std::string getBjnCurrentLogFileName()
	{
		std::string fullFileName(getBjnLogfilePath());
		typedef std::multimap<std::time_t, boost::filesystem::wpath> result_set_t;
		result_set_t result_set;
		result_set_t::iterator iter;
		result_set_t::reverse_iterator rev_iter;
		boost::filesystem::wrecursive_directory_iterator dir_iter(fullFileName), end_iter;

		for (; dir_iter != end_iter; ++dir_iter)
		{
			if (boost::filesystem::is_regular_file(dir_iter->status()))
			{
				result_set.insert(result_set_t::value_type(boost::filesystem::last_write_time(dir_iter->path()), *dir_iter));
			}
		}
#ifdef FB_WIN
		fullFileName.append("\\");
#else
		fullFileName.append("/");
#endif
		rev_iter = result_set.rbegin();
		return fullFileName.append((rev_iter->second).filename().string());
	}

	std::string getBjnPackageType()
	{
		// return the linux package type if known
		// empty string on non Linux builds
		std::string packageType = "";

#ifdef FB_X11
		if (talk_base::IsDebian())
		{
			packageType = "debian";
		}
		else if (talk_base::IsRedHat())
		{
			packageType = "redhat";
		}
#endif

		return packageType;
	}

#ifdef FB_MACOSX
	//On OSX > 10.8.4 has added isfinitel which is not present on older system like 10.6.8.
	//Plugin fails to load because of missing symbol of __isfinitel when compiled on 10.8.x 
	// Therefore we defined our own __isfinitel function.

	extern "C" {
		int __isfinitel(long double x)
		{
			x = __builtin_fabsl(x);
			return x < __builtin_infl();
		}
	}
#endif 

#ifdef FB_WIN
	void refreshIEElevationPolicy()
	{
		// if we are hosted in IE then we want to get IE to refresh it's reg setting
		// as the installer may have updated some of them. Call the IE protected mode
		// API if it exists: HRESULT __stdcall IERefreshElevationPolicy();
		typedef HRESULT(__stdcall *IE_REFRESH_ELEVATION_POLICY_FUNC)();
		// this will return NULL if the host browser isn't IE as ieframe.dll won't be loaded
		IE_REFRESH_ELEVATION_POLICY_FUNC ieFunc = (IE_REFRESH_ELEVATION_POLICY_FUNC)
			GetProcAddress(GetModuleHandle(L"ieframe.dll"), "IERefreshElevationPolicy");
		if (ieFunc)
		{
			HRESULT ieHR = ieFunc();
			LOG(LS_INFO) << "IERefreshElevationPolicy() returned: " << ieHR;
		}
	}
#endif

	std::string getInstallType()
	{
		// return the install type of the plugin on Windows (user or admin)
		// return an empty string on non Windows builds
		std::string installType = "";
#ifdef FB_WIN
		// althoug the admin and user plugin are functionally identical
		// we have to compile them slightly differently as some of the
		// Firebreath macros are different, so we just use a ifdef here
#ifdef ENABLE_PER_MACHINE_INSTALL
		installType = "admin";
#else
		installType = "user";
#endif

#endif
		return installType;
	}

#ifdef ENABLE_CONFIG_PATH_TEST_API
	void getPluginConfigPaths(std::map<std::string, std::string>& pathMap, const std::string& pluginName)
	{
		// this is a test/debug API to make sure that the path are
		// correct when adding new build configs (64bit, admin etc.)
		pathMap["BJN:getVersion"] = getVersion(pluginName);
		pathMap["BJN:getMasterVersion"] = getMasterVersion();
		pathMap["BJN:getBjnAutoUpdaterEXE"] = getBjnAutoUpdaterEXE();
		pathMap["BJN:getBjnDownloadPath"] = getBjnDownloadPath();
		pathMap["BJN:getBjnOldLocalAppDataPath"] = getBjnOldLocalAppDataPath();
		pathMap["BJN:getBjnPcapPath"] = getBjnPcapPath();
		pathMap["BJN:getBjnDownloadPath"] = getBjnDownloadPath();
		pathMap["BJN:getBjnLogfilePath"] = getBjnLogfilePath();
		pathMap["BJN:getBjnLogDirName"] = getBjnLogDirName();
		pathMap["BJN:getMasterName"] = getMasterName();
		pathMap["BJN:getBjnLocalAppCompanyDir"] = getBjnLocalAppCompanyDir();
		pathMap["BJN:getBjnAppDataPath"] = getBjnAppDataPath();
		pathMap["BJN:getBjnLocalAppDataPath"] = getBjnLocalAppDataPath();
		pathMap["BJN:getBjnConfigFile"] = getBjnConfigFile();
		pathMap["BJN:getBjnConfigPath"] = getBjnConfigPath();
		pathMap["BJN:getBjnEffectFile"] = getBjnEffectFile();
#ifdef FB_WIN
		pathMap["BJN:getMasterVersionForColor(RED)"] = getMasterVersionForColor(kPluginColorRed);
		pathMap["BJN:getMasterVersionForColor(GREEN)"] = getMasterVersionForColor(kPluginColorGreen);
		pathMap["BJN:getMasterVersionForColor(BLUE)"] = getMasterVersionForColor(kPluginColorBlue);
#endif
#ifdef FB_MACOSX
		pathMap["BJN:getMacPluginBasePath"] = getMacPluginBasePath();
		pathMap["BJN:getDiagReportPath"] = getDiagReportPath();
#endif
#ifdef FB_X11
		pathMap["BJN:getLinuxPluginBasePath"] = getLinuxPluginBasePath();
#endif
	}
#endif // ENABLE_CONFIG_PATH_TEST_API

}