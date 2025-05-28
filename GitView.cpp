#include "pch.h"

#include "GitView.h"

#include "FileIterator.h"
#include "RefIterator.h"
#include "RefTypeIterator.h"
#include "TopDirIterator.h"

#include "resource.h"
#include "utils.h"

#include <shlwapi.h>

#include <boost/property_tree/json_parser.hpp>

#include <algorithm>
#include <cwchar>
#include <sstream>

using namespace std;

#pragma warning(disable:4996)

GitView::GitView()
{
}

void GitView::init(int pluginNo,
                   tProgressProcW progressFunc, tLogProcW, tRequestProcW requestFunc)
{
	mPluginNo = pluginNo;
	mProgressFunc = progressFunc;
	mRequestFunc = requestFunc;
}

LineLogger GitView::log()
{
	return LineLogger(mLogFile);
}

bool GitView::loadSettings(const char defaultSettingsPath[MAX_PATH])
{
	return openSettingsFile(defaultSettingsPath) && readSettings();
}

bool GitView::openSettingsFile(const char defaultSettingsPath[MAX_PATH])
{
	//look for 'plugins/gitview.json' file first, then for 'gitview.json'
	const char dirSlash = '\\';
	size_t lastSlashPos = 0;
	for(size_t i = 0; i < MAX_PATH && defaultSettingsPath[i]; ++i)
	{
		if (dirSlash == defaultSettingsPath[i])
			lastSlashPos = i;
	}

	if('\0' == defaultSettingsPath[lastSlashPos])
	{
		mInitLog << L"invalid default ini location '" << str2wstr(defaultSettingsPath) << L"' - cannot derive location of gitview settings" << endl;
		return false;
	}

	char settingsSubpath[] = "plugins\\gitview.json";
	constexpr size_t subpathLen = DIM(settingsSubpath);

	char myIniPath[MAX_PATH + subpathLen];
	copy(defaultSettingsPath, defaultSettingsPath + lastSlashPos + 1, myIniPath);
	copy_n(settingsSubpath, subpathLen, myIniPath + lastSlashPos + 1);

	ifstream settingsFile(myIniPath);
	if (!settingsFile.is_open())
	{
		char iniFile[] = "gitview.json";
		copy_n(iniFile, DIM(iniFile), myIniPath + lastSlashPos + 1);
		settingsFile.open(myIniPath);
		if(!settingsFile.is_open())
		{
			mInitLog << L"Settings file " << str2wstr(myIniPath) << L" does not exist or is not accessible - cannot initialize gitview." << endl
			         << L"Settings file should be in TC settings folder or in 'gitview' subfolder.";
			return false;
		}
	}

	mSettingsFilePath = myIniPath;
	return true;
}

bool GitView::readSettings()
{
	namespace pt = boost::property_tree;
	pt::wptree settings;

	try {
		pt::json_parser::read_json(mSettingsFilePath, settings);
	}
	catch(std::exception& e) {
		mInitLog << L"GitView::readSettings: read_json failed: " << str2wstr(e.what()) << endl;
		return false;
	}

	try
	{
		mSettings.mLogLocation = settings.get<wstring>(L"debug.logLocation", L"");

		if(mSettings.mLogLocation.empty())
		{
			mInitLog << L"Logging is disabled (debug.logLocation setting is not set)." << endl;
		}
		else if(mLogFile.open(mSettings.mLogLocation, ios_base::out | ios_base::app), mLogFile.is_open())
		{
			mInitLog << L"Runtime logs will be available in " << mSettings.mLogLocation << L"." << endl;
		}
		else
		{
			mInitLog << L"Failed to create log file at " << mSettings.mLogLocation << L".\nInit done." << endl;
		}
	}
	catch(...)
	{
		mInitLog << L"Failed to create log file at " << mSettings.mLogLocation << L".\nInit done." << endl;
	}

	try
	{
		const auto& gitSettingsNode = settings.get_child(L"git client");

		Git::Settings& gitSettings = mSettings.mGitSettings;

		gitSettings.mGitPath = gitSettingsNode.get<wstring>(L"git", wstring());
		if(gitSettings.mGitPath.empty())
		{
			mInitLog << L"Error: path to git.exe is not set." << endl;
			log() << L"Error: path to git.exe is not set.";
			return false;
		}
		else if(!PathFileExistsW(gitSettings.mGitPath.c_str()))
		{
			mInitLog << L"Warning: file " << gitSettings.mGitPath << " doesn't exist." << endl;
			log() << L"Warning: file " << gitSettings.mGitPath << " doesn't exist.";
		}

		mInitLog << L"Init done.";

		gitSettings.mShowCurrentBranch = gitSettingsNode.get<bool>(L"showCurrentBranch", false);
		gitSettings.mTimeout = gitSettingsNode.get<unsigned>(L"timeoutMs", 1000);
		if(0 == gitSettings.mTimeout)
			gitSettings.mTimeout = 1000;
		else if(gitSettings.mTimeout > 10000)
			gitSettings.mTimeout = 10000;

		const auto& repos = settings.get_child(L"repos");
		for(const auto& item: repos)
		{
			const pt::wptree& repo = item.second;
			try
			{
				mRepos.add(repo.get<wstring>(L"name"),
				           repo.get<wstring>(L"workingDir"));
			}
			catch(const pt::ptree_bad_path& pathError)
			{
				log() << L"ERROR: setting repos." << str2wstr(pathError.path<pt::wptree::path_type>().dump()) << L" not found";
			}
			catch(const pt::ptree_bad_data& valueError)
			{
				log() << L"ERROR: settings error: " << str2wstr(valueError.what());
			}
		}
	}
	catch(const pt::ptree_bad_path& pathError)
	{
		log() << L"ERROR: setting " << str2wstr(pathError.path<pt::wptree::path_type>().dump()) << L" not found";
	}
	catch(const pt::ptree_bad_data& valueError)
	{
		log() << L"ERROR: settings error: " << str2wstr(valueError.what());
	}
	catch(const std::exception& e)
	{
		log() << L"ERROR: settings error: " << str2wstr(e.what());
	}
	catch(...)
	{
		log() << L"ERROR reading settings";
	}

	wstring wSettingsFile; wSettingsFile.reserve(mSettingsFilePath.size());
	copy(mSettingsFilePath.cbegin(), mSettingsFilePath.cend(), back_inserter(wSettingsFile));
	log() << L"Settings (" << wSettingsFile << "):"
	      << L"\n  git.path: " << mSettings.mGitSettings.mGitPath
	      << L"\n  git.timeout: " << mSettings.mGitSettings.mTimeout
			<< L"\n  git.showCurBranch: " << boolalpha << mSettings.mGitSettings.mShowCurrentBranch;

	return true;
}

const Repo* GitView::findRepo(const ItemKey& key) const
{
	const auto& repos = mRepos.repos();
	auto it = repos.find(key.repoName);

	return repos.end() == it ? 0 : &(it->second);
}

IFileIterator* GitView::createFileIterator(const WCHAR* path)
{
	ItemKey key(path);
	if(key.empty()) // at root level: list repositories
	{
		return createTopDirIterator();
	}
	else if(GitRef::Unknown == key.refType) // at repo level: create ref type iterator
	{
		return createRefTypeIterator();
	}
	
	else if(key.branch.empty()) // at ref type level: create GitRef iterator
	{
		return createRefIterator(key);
	}
	else // at branch/tag level: create file iterator
	{
		return createFileIterator(key);
	}
}


IFileIterator* GitView::createTopDirIterator()
{
	auto result = new TopDirIterator(mRepos.repos(), mInitLog);
	mFileIterators.insert(result);
	return result;
}

IFileIterator* GitView::createRefTypeIterator()
{
	auto result = new RefTypeIterator;
	mFileIterators.insert(result);
	return result;
}

IFileIterator* GitView::createRefIterator(const ItemKey& key)
{
	const Repo* repo = findRepo(key);
	if(!repo)
		return 0;

	StringList branches;
	OpStatus opStatus;
	Git git(mSettings.mGitSettings);
	switch(key.refType)
	{
		case GitRef::Branch:
			git.br(repo->workingDir, branches, opStatus);
		break;
		case GitRef::Tag:
			git.tags(repo->workingDir, branches, opStatus);
		break;
		default:
			opStatus.set(OpStatus::InternalError, L"unexpected reference type");
	}

	if(opStatus.isBad())
	{
		log() << opStatus.mDescr;
		return 0;
	}

	auto result = new RefIterator(branches);
	mFileIterators.insert(result);
	return result;
}

IFileIterator* GitView::createFileIterator(const ItemKey& key)
{
	const Repo* repo = findRepo(key);
	if(!repo)
		return 0;

	Entries entries;
	OpStatus opStatus;
	Git git(mSettings.mGitSettings);
	git.ls(repo->workingDir, key.branch, key.filePath, entries, opStatus);

	if(opStatus.isBad())
	{
		log() << opStatus.mDescr;
		return new ErrorFileItStub(opStatus.mDescr.c_str());
	}

	auto result = new FileIterator(entries);
	mFileIterators.insert(result);
	return result;
}

IFileIterator* GitView::getFileIterator(HANDLE fh)
{
	IFileIterator* fIt = (IFileIterator*)fh;
	auto it = mFileIterators.find(fIt);
	return it == mFileIterators.end() ? 0 : fIt;
}

void GitView::removeFileIterator(HANDLE fh)
{
	removeFileIterator(getFileIterator(fh));
}

void GitView::removeFileIterator(IFileIterator* fIt)
{
	if(fIt)
	{
		mFileIterators.erase(fIt);
		delete fIt;
	}
}

void GitView::saveFile(wchar_t* srcPath, wchar_t* destPath, OpStatus& saveStatus)
{
	saveStatus.clear();

	if(mProgressFunc)
		mProgressFunc(mPluginNo, srcPath, destPath, 0);

	ItemKey itemKey(srcPath);


	if(initLogName == itemKey.repoName && GitRef::Unknown == itemKey.refType) //is it init.log?
	{
		wofstream destFile(destPath);
		destFile << mInitLog.str();
		return;
	}

	const Repo* repo = findRepo(itemKey);
	if(!repo)
	{
		saveStatus.set(OpStatus::GitError, L"repo not found");
		return;
	}

	log() << L"FsGetFile: " << itemKey.branch << L":" << itemKey.filePath << L"  -->  " << destPath;

	Git git(mSettings.mGitSettings);
	git.saveFile(repo->workingDir, itemKey.branch.c_str(), itemKey.filePath.c_str(), destPath, saveStatus);

	if(mProgressFunc)
		mProgressFunc(mPluginNo, srcPath, destPath, 100);
}
