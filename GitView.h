#pragma once

#include "fsplugin.h"

#include "Git.h"
#include "IFileIterator.h"
#include "ErrorFileItStub.h"
#include "Repositories.h"
#include "utils.h"

#include <boost/property_tree/ptree.hpp>

#include <fstream>
#include <set>
#include <string>

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the GITVIEW_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
//  functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef GITVIEW_EXPORTS
#define GITVIEW_API __declspec(dllexport)
#else
#define GITVIEW_API __declspec(dllimport)
#endif

class GitView
{
public:
	GitView();

	void init(int pluginNo,
	          tProgressProcW progressFunc,
	          tLogProcW logFunc,
	          tRequestProcW requestFunc);

	LineLogger log();

	bool loadSettings(const char defaultSettingsPath[MAX_PATH]);

	bool initialized() const
	{
		return true;
	}

	IFileIterator* createFileIterator(const WCHAR* path); //based on 'path', delegate creation to one of methods below:
	IFileIterator* createRepoIterator(); //iterates over repo names
	IFileIterator* createRefTypeIterator(); //iteraters over 'branches', 'tags' directories
	IFileIterator* createRefIterator(const ItemKey& key); //iteraters over branch or tag names
	IFileIterator* createFileIterator(const ItemKey& key); //iteraters over files in a branch/tag

	IFileIterator* getFileIterator(HANDLE fh);
	void removeFileIterator(HANDLE fh);
	void removeFileIterator(IFileIterator* fIt);

	void saveFile(wchar_t* srcPath, wchar_t* destPath, OpStatus& saveStatus);

private:
	bool openSettingsFile(const char defaultSettingsPath[MAX_PATH]);
	bool readSettings();

	const Repo* findRepo(const ItemKey& key) const;

private:
	struct Settings
	{
		std::wstring mLogLocation;
		std::string mFallbackLogPath;
		Git::Settings mGitSettings;
	};

	int mPluginNo = 0;
	tProgressProcW mProgressFunc = 0;
	tRequestProcW mRequestFunc = 0;

	std::wofstream logFile;

	std::string mSettingsFilePath;
	Settings mSettings;

public:
	HANDLE mModule = NULL;

	Repositories mRepos;

	std::set<IFileIterator*> mFileIterators;
	ErrorFileItStub mErrorFileIterator;
};
