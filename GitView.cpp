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

GitView::GitView()
{
}

void GitView::init(int pluginNo,
                   tProgressProcW progressFunc, tLogProcW logFunc, tRequestProcW requestFunc)
{
	mPluginNo = pluginNo;
	mProgressFunc = progressFunc;
	mLogFunc = logFunc;
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
	//use 'defaultSettingsPath' as fallback option
	const char dirSlash = '\\';
	size_t lastSlashPos = MAX_PATH;
	for (size_t i = 0; i < MAX_PATH; ++i)
	{
		if (dirSlash == defaultSettingsPath[i])
			lastSlashPos = i;
	}

	if (MAX_PATH == lastSlashPos)
	{
		log() << L"invalid ini location - cannot read the settings";
		return false;
	}

	char myIniPath[MAX_PATH + 50];
	char* iniFilePath = myIniPath;
	copy(defaultSettingsPath, defaultSettingsPath + lastSlashPos + 1, myIniPath);

	char subpath[] = "plugins\\gitview.json";
	copy_n(subpath, DIM(subpath), myIniPath + lastSlashPos + 1);
	ifstream settingsFile(myIniPath);
	if (!settingsFile.is_open())
	{
		char iniFile[] = "gitview.json";
		copy_n(iniFile, DIM(iniFile), myIniPath + lastSlashPos + 1);
		settingsFile.open(myIniPath);
		if (!settingsFile.is_open())
		{
			log() << L"Settings file does not exist or is not accessible - cannot initialize gitview.";
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
	catch(...) { return false; }

	try
	{
		mSettings.mLogLocation = settings.get<wstring>(L"debug.logLocation", L"");
		if(!mSettings.mLogLocation.empty())
		{
			logFile.open(mSettings.mLogLocation, ios_base::out | ios_base::app);
			log() << L"\n\t=== starting gitview plugin ===\n";
		}
	}
	catch(...) { }

	try
	{
		const auto& gitSettingsNode = settings.get_child(L"git client");

		Git::Settings& gitSettings = mSettings.mGitSettings;

		gitSettings.mGitPath = gitSettingsNode.get<wstring>(L"git", wstring());
		if(gitSettings.mGitPath.empty())
			return false;

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
				           repo.get<wstring>(L"url"),
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

	log() << L"Settings:"
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
	if(!initialized())
	{
		mErrorFileIterator.setError(L"no git exe");
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
