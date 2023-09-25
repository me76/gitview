#include "pch.h"

#include "GitView.h"

#include "FileIterator.h"
#include "RefIterator.h"
#include "RefTypeIterator.h"
#include "RepoNameIterator.h"
#include "resource.h"
#include "utils.h"

#include <boost/property_tree/json_parser.hpp>

#include <algorithm>
#include <cwchar>
#include <sstream>

using namespace std;

#pragma warning(disable:4996)

GitView::GitView()
{
	if(const char* homePath = getenv("USERPROFILE"))
	{
		mSettings.mFallbackLogPath.assign(homePath).append("\\gitview.log");
	}
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
	return LineLogger(logFile);
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
		ofstream fallbackLog(mSettings.mFallbackLogPath, ios_base::out | ios_base::app);
		fallbackLog << "invalid ini location '" << defaultSettingsPath << "' - cannot read the settings" << endl;
		return false;
	}

	char settingsSubpath[] = "plugins\\gitview.json";
	constexpr size_t subpathLen = DIM( settingsSubpath );

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
			ofstream fallbackLog(mSettings.mFallbackLogPath, ios_base::out | ios_base::app);
			fallbackLog << "Settings file " << myIniPath << " does not exist or is not accessible - cannot initialize gitview." << endl;
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
		ofstream fallbackLog(mSettings.mFallbackLogPath, ios_base::out | ios_base::app);
		fallbackLog << "GitView::readSettings: read_json failed: " << e.what() << endl;
		return false;
	}

	bool settingsRead = false;
	try
	{
		mSettings.mLogLocation = settings.get<wstring>(L"debug.logLocation", L"");
		if(!mSettings.mLogLocation.empty()
		   && (logFile.open(mSettings.mLogLocation, ios_base::out | ios_base::app), logFile.is_open()))
		{
			settingsRead = true;
		}
	}
	catch(...) { }

	if(!settingsRead)
	{
		ofstream fallbackLog(mSettings.mFallbackLogPath, ios_base::out | ios_base::app);
		fallbackLog << "GitView::readSettings: failed to load settings from " << (const char*) mSettings.mLogLocation.c_str() << endl;
	}

	try
	{
		Git::Settings& gitSettings = mSettings.mGitSettings;
		gitSettings.mShowCurrentBranch = settings.get<bool>(L"showCurrentBranch", false);

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
	if(!initialized())
	{
		mErrorFileIterator.setError(L"gitview is not initialized");
		return &mErrorFileIterator;
	}

	ItemKey key(path);
	if(key.empty()) // at root level: list repositories
	{
		return createRepoIterator();
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


IFileIterator* GitView::createRepoIterator()
{
	auto result = new RepoNameIterator(mRepos.repos());
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
	if(&mErrorFileIterator == fIt)
		return;

	if(fIt)
	{
		mFileIterators.erase(fIt);
		delete fIt;
	}
}

void GitView::saveFile(wchar_t* srcPath, wchar_t* destPath, OpStatus& saveStatus)
{
	saveStatus.clear();

	mProgressFunc(mPluginNo, srcPath, destPath, 0);

	ItemKey itemKey(srcPath);
	const Repo* repo = findRepo(itemKey);
	if(!repo)
	{
		saveStatus.set(OpStatus::GitError, L"repo not found");
		return;
	}

	log() << L"FsGetFile: " << itemKey.branch << L":" << itemKey.filePath << L"  -->  " << destPath;

	Git git(mSettings.mGitSettings);
	git.saveFile(repo->workingDir, itemKey.branch.c_str(), itemKey.filePath.c_str(), destPath, saveStatus);

	mProgressFunc(mPluginNo, srcPath, destPath, 100);
}
