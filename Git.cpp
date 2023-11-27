#include "pch.h"

#include "Git.h"

#include <git2.h>

#include <cassert>
#include <fstream>
#include <memory>
#include <sstream>
#include <vector>

using namespace std;

namespace {

string asUtf8(const wstring& s)
{
	string result;
	if(!s.empty())
	{
		result.resize(2 * s.size());
		int resultSize = WideCharToMultiByte(CP_UTF8, 0 /*conversion flags - using default*/, s.c_str(), s.size(), &result.front(), result.size(), 0, 0);

		if(resultSize > 0)
		{
			result.resize(resultSize);
		}
	}

	return result;
}

wstring asWStr(const char* utf8Str)
{
	size_t strLen = strlen(utf8Str);
	wstring result(strLen, ' ');
	MultiByteToWideChar(CP_UTF8, 0 /*conversion flags - using default*/, utf8Str, -1, &result.front(), strLen);

	return result;
}

typedef std::shared_ptr<git_repository> GitRepoPtr;

GitRepoPtr getRepo(const std::wstring& workingDir, OpStatus& opStatus)
{
	git_repository* repo;
	if(opStatus.mErrorCode = git_repository_open(&repo, asUtf8(workingDir).c_str()))
	{
		opStatus.mStatus = OpStatus::GitError;
		opStatus.mDescr.assign(L"failed to open git repo at ").append(workingDir);
		return GitRepoPtr();
	}

	return GitRepoPtr(repo, git_repository_free);
}

typedef std::shared_ptr<git_branch_iterator> GitBranchIt;

GitBranchIt getFirstLocalBranch(GitRepoPtr repo, OpStatus& opStatus)
{
	git_branch_iterator* branchIt = 0;
	if(opStatus.mErrorCode = git_branch_iterator_new(&branchIt, repo.get(), GIT_BRANCH_LOCAL))
	{
		opStatus.mStatus = OpStatus::GitError;
		opStatus.mDescr.assign(L"failed to read git branches at ").append(L"workingDir");
		return GitBranchIt();
	}

	return GitBranchIt(branchIt, git_branch_iterator_free);
}

} //local namespace

Git::Git(const Settings& settings):
	mSettings(settings)
{
}

bool Git::initialized() const
{
	return true;
}

bool Git::br(const std::wstring& workingDir,
             StringList& branches, OpStatus& opStatus)
{
	GitRepoPtr repo = getRepo(workingDir, opStatus);
	if(!repo)
	{
		return false;
	}

	GitBranchIt branchIt = getFirstLocalBranch(repo, opStatus);
	if(!branchIt)
	{
		return false;
	}

	git_reference* branch = 0;
	git_branch_t branchType;
	for(bool hasMore = true; hasMore; )
	{
		switch(git_branch_next(&branch, &branchType, branchIt.get()))
		{
			case 0:
				if(GIT_BRANCH_LOCAL == branchType)
				{
					const char* brName;
					if(git_branch_name(&brName, branch) == 0)
					{
						branches.emplace_back(asWStr(brName));
					};
				}
				break;
			case GIT_ITEROVER:
				hasMore = false;
				break;
			default:
				;
		}
	}

	return true;
}

bool Git::tags(const std::wstring& workingDir,
               StringList& tags, OpStatus& opStatus)
{
	GitRepoPtr repo = getRepo(workingDir, opStatus);
	if(!repo)
	{
		return false;
	}

	git_strarray tagNames;
	git_tag_list(&tagNames, repo.get());
	for(size_t i = 0; i < tagNames.count; ++i)
	{
		tags.push_back(asWStr(tagNames.strings[i]));
	}
	git_strarray_dispose(&tagNames);

	return true;
}

bool Git::ls(const std::wstring& workingDir,
             const wstring& gitRef, const wstring& subDir,
             Entries& fileData, OpStatus& opStatus)
{
	if(!initialized())
		return false;

	GitRepoPtr repo = getRepo(workingDir, opStatus);
	if(!repo)
	{
		return false;
	}

	git_index* index;
	if(git_repository_index(&index, repo.get()) < 0)
	{
		opStatus.set(OpStatus::GitError, L"gitlib2: couldn't open repository index");
		return false;
	}

	git_oid treeId;
	if(git_index_write_tree(&treeId, index) < 0)
	{
		opStatus.set(OpStatus::GitError, L"gitlib2: couldn't convert index to treeId");
		return false;
	}

	git_index_free(index);

	git_tree* root;
	if(git_tree_lookup(&root, repo.get(), &treeId) < 0)
	{
		opStatus.set(OpStatus::GitError, L"gitlib2: couldn't get tree from treeId");
		return false;
	}

	git_tree* targetTree = root;

	if(!subDir.empty())
	{
		git_tree_entry* entry;
		if(git_tree_entry_bypath(&entry, root, asUtf8(subDir).c_str()))
		{
			opStatus.set(OpStatus::GitError, L"gitlib2: couldn't get tree entry for " + subDir);
			return false;
		}

		if(git_tree_entry_type(entry) != GIT_OBJECT_TREE)
		{
			opStatus.set(OpStatus::InternalError, subDir + L" wasn't a subdirectory in repo " + workingDir);
			return false;
		}

		if(git_tree_lookup(&targetTree, repo.get(), git_tree_entry_id(entry)))
		{
			opStatus.set(OpStatus::GitError, L"gitlib2: couldn't find tree id for " + subDir);
			return false;
		}
	}

	struct Payload
	{
		Entries& fileEntries;
		const wstring& subDir;
	}
	payload = {fileData, subDir};

	auto traverseCallback = [](const char* root, const git_tree_entry* entry, void* payload) -> int
	{
		//return 1 to skip the entry, 0 to include it in traversal, and -1 to terminate traversal

		if(!root)
			return 1;

		Payload* paramsAndResults = static_cast<Payload*>(payload);

		if(root && *root) //list only top-level entries
			return 1;

		Entries& fileEntries = paramsAndResults->fileEntries;
		const wstring& subDir = paramsAndResults->subDir;

		wstring fileName = asWStr(git_tree_entry_name(entry));
		switch(git_tree_entry_type(entry))
		{
			case GIT_OBJECT_TREE:
				fileEntries.emplace_back(fileName, Entry::Directory);
			break;
			case GIT_OBJECT_BLOB:
				fileEntries.emplace_back(fileName, Entry::File);
			break;
			default:
				return 1;
		}

		return 0;
	};
	git_tree_walk(targetTree, GIT_TREEWALK_PRE, traverseCallback, &payload);

	return true;
}

void Git::saveFile(const wstring& workingDir, const wchar_t* ref,
                   const wchar_t* srcPath, const wchar_t* destPath,
                   OpStatus& opStatus)
{
	GitRepoPtr repo = getRepo(workingDir, opStatus);
	if(!repo)
	{
		wofstream(destPath) << L"no git repo in " << workingDir;
		opStatus.set(OpStatus::GitError, L"not a git repo");
		return;
	}

	wstring revPath;
	revPath.assign(ref).append(L":").append(srcPath);

	git_object* gitObject = nullptr;
	if(git_revparse_single(&gitObject, repo.get(), asUtf8(revPath).c_str()))
	{
		wofstream(destPath) << L"rev " << revPath << L" not found in repo " << workingDir;
		opStatus.set(OpStatus::GitError, L"rev not found in repo");
		return;
	}

	if(git_object_type(gitObject) != GIT_OBJECT_BLOB)
	{
		opStatus.set(OpStatus::GitError, L"not a file");
		git_object_free(gitObject);
		return;
	}

	git_blob* blob = (git_blob*)gitObject;

	auto file = CreateFile(destPath, GENERIC_WRITE, 0 /*sharing*/, NULL /*&secAttrs*/,
	                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL /*hTemplateFile*/);
	if(INVALID_HANDLE_VALUE == file)
	{
		wstringstream ss;
		ss << L"couldn't create file " << destPath << L": error " << GetLastError();
		opStatus.set(OpStatus::SystemError, ss.str());
		return;
	}

	DWORD dataSize = (DWORD)git_blob_rawsize(blob), bytesWritten = 0;
	if(!WriteFile(file, git_blob_rawcontent( blob ), dataSize, &bytesWritten, NULL /*&overlapped*/) || bytesWritten < dataSize)
	{
		opStatus.set(OpStatus::SystemError, L"couldn't save all content");
	}

	git_blob_free(blob);

	CloseHandle(file);
}
