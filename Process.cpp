#include "pch.h"

#include "Process.h"

#include "GitView.h"
#include "utils.h"

#include <algorithm>
#include <cassert>
#include <cwchar>
#include <vector>

using namespace std;

extern GitView gview;

Process::IOBuf::IOBuf(HANDLE ioHandle)
{
	setIoHandle(ioHandle);
}

void Process::IOBuf::setIoHandle(HANDLE ioHandle)
{
	mIoHandle = ioHandle;
	setg(mBuf, mBuf, mBuf);
	setp(mBuf, mBuf, mBuf + mBufSize);
}

Process::IOBuf::pos_type Process::IOBuf::calcNewPos(streampos startPos, streamoff offset, ios_base::seekdir startFrom) const
{
	streampos result = 0;
	switch(startFrom)
	{
		case ios_base::beg:
			result = offset;
		case ios_base::cur:
			result = startPos + offset;
		case ios_base::end:
			result = mBufSize - abs(offset);
		default:
			return streampos(-1);
	}

	return result >= 0 && result < (int)mBufSize ? result : pos_type(streampos(-1));
}

void Process::IOBuf::setGetPos(std::streampos pos)
{
	auto bufEnd = egptr();
	assert(pos >= 0 && pos <= bufEnd - mBuf);
	setg(mBuf, mBuf + pos, bufEnd);
}

void Process::IOBuf::setPutPos(std::streampos pos)
{
	auto bufEnd = epptr();
	assert(pos >= 0 && pos < bufEnd - mBuf);
	setp(mBuf, mBuf + pos, bufEnd);
}

bool Process::IOBuf::handleAvailable() const
{
	return mIoHandle && mIoHandle != INVALID_HANDLE_VALUE;
}

streampos Process::InBuf::seekoff(off_type offset, ios_base::seekdir startFrom,
                                  ios_base::openmode which)
{
	if((which & ios_base::in) == 0)
		return streampos(off_type(-1));

	streampos newPos = calcNewPos(gptr() - mBuf, offset, startFrom);
	setGetPos(newPos);
	return newPos;
}

streamsize Process::InBuf::showmanyc()
{
	return gptr() ? egptr() - gptr() : 0;
}

Process::IOBuf::int_type Process::InBuf::underflow()
{
	/*Ensure that at least one character is available in the input area by updating the pointers to the input area (if needed) and reading more data in from the input sequence (if applicable). Returns the value of that character (converted to int_type with Traits::to_int_type(c)) on success or Traits::eof() on failure.

	May update gptr, egptr and eback pointers to define the location of newly loaded data (if any). On failure, ensure that either gptr() == nullptr or gptr() == egptr.

	Return the value of the character pointed to by the get pointer after the call on success.*/

	if(showmanyc() > 0)
		return traits_type::to_int_type(*gptr());

	int_type nextChar;
	if(!handleAvailable() || !pump(nextChar))
	{
		setGetPos(egptr() - mBuf); // "on failure, ensure that either gptr == nullptr or gptr == egptr"
		return traits_type::eof();
	}

	return nextChar;
}

streamsize Process::InBuf::xsgetn(char_type* s, streamsize count)
{
/* Read 'count' characters from the input sequence into 's'. The characters are read as if by repeated calls to sbumpc(). That is, if less than count characters are immediately available, the function calls uflow() to provide more until Traits::eof() is returned.*/
	auto remainingCharCount = count;
	while(remainingCharCount > 0 && underflow() != traits_type::eof())
	{
		auto availableSpace = min(showmanyc(), remainingCharCount);
		assert(availableSpace > 0);

		copy_n(gptr(), availableSpace, s);
		gbump(availableSpace);

		remainingCharCount -= availableSpace;
		s += availableSpace;
	}
	return count - remainingCharCount;
}

bool Process::InBuf::pump(int_type& pumpedChar)
{
	if(gptr() < egptr())
	{
		pumpedChar = *gptr();
		return true; //still have readable data
	}

	ptrdiff_t pumpSize = (mBuf + mBufSize - egptr());
	//pumpSize is treated in bytes - this results in pumping half amount of available space,
	//and leaves us enough space to expand characters when converting them to wide chars

	if(pumpSize <= 0) //no space available at buffer end
	{
		//shift the buffer to left and preserve last 64 bytes (move last 64 bytes to beginning and set gptr() to 64th byte)
		constexpr size_t charsToKeep = 64;
		memmove(mBuf, mBuf + mBufSize - charsToKeep, charsToKeep);
		setg(mBuf, mBuf + charsToKeep, mBuf + charsToKeep);

		pumpSize = (mBufSize - charsToKeep);
	}

	//https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile
	DWORD bytesRead = 0;
	char buf[mBufSize];
	if(!ReadFile(mIoHandle, buf, (DWORD)pumpSize, &bytesRead, 0 /*&overlapped*/))
		return false;

	size_t charsRead = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED,
                                          buf, bytesRead,
	                                       gptr(), mBufSize);
	if(!charsRead)
	{
		DWORD winApiError = GetLastError();
		gview.log() << "MultiByteToWideChar.Error: " << winApiError;
		return false;
	}

	//size_t charsRead = bytesRead / 2;
	setg(mBuf, gptr(), gptr() + charsRead);

	pumpedChar = *gptr();
	return true;
}

bool Process::InBuf::hasData() const
{
	if(gptr() && gptr() < egptr())
		return true;

	DWORD availableBytes = 0;
	if(!PeekNamedPipe(mIoHandle, 0, 0, 0, &availableBytes, 0))
	{
		auto winApiError = GetLastError();
		gview.log() << "PeekNamedPipe.Error: " << winApiError;
		return false;
	}

	return availableBytes > 0;
}

streampos Process::OutBuf::seekoff(off_type offset, ios_base::seekdir startFrom,
                                   ios_base::openmode which)
{
	if((which & ios_base::out) == 0)
		return streampos(off_type(-1));

	streampos newPos = calcNewPos(gptr() - mBuf, offset, startFrom);
	setPutPos(newPos);
	return newPos;
}

int Process::OutBuf::sync()
{
	if(!handleAvailable())
		return -1;

	ptrdiff_t dumpSize = (pptr() - pbase()) * 2;
	if(0 <= dumpSize)
		return 0;

	DWORD bytesWritten = 0;
	return !WriteFile(mIoHandle, pptr(), (DWORD)dumpSize, &bytesWritten, 0 /*&overlapped*/) || bytesWritten < dumpSize
	       ? -1 : 0;
}

streamsize Process::OutBuf::xsputn(const char_type* s, streamsize count)
{
/*Write 'count' characters from 's' to the output sequence. The characters are written as if by repeated calls to sputc(). Writing stops when either count characters are written or a call to sputc() would have returned Traits::eof().
If the put area becomes full (pptr() == epptr()), this function may call overflow(), or achieve the effect of calling overflow() by some other, unspecified, means.*/
	auto remainingCharCount = count;
	while(remainingCharCount > 0 && overflow(traits_type::eof()) != traits_type::eof())
	{
		auto availableSpace = min(epptr() - pptr(), remainingCharCount);
		assert(availableSpace > 0);

		copy_n(s, availableSpace, pptr());
		pbump(availableSpace);

		remainingCharCount -= availableSpace;
		s += availableSpace;
	}
	return count - remainingCharCount;
}

Process::IOBuf::int_type Process::OutBuf::overflow(int_type c)
{
	assert(epptr() - pbase() == mBufSize);

	/*Ensure that there is space at the put area for at least one character by saving some initial subsequence of characters starting at pbase() to the output sequence and updating the pointers to the put area (if needed). If 'c' is not Traits::eof() (i.e. Traits::eq_int_type(ch, Traits::eof()) != true), it is either put to the put area or directly saved to the output sequence.

	May update pptr, epptr and pbase pointers to define the location to write more data. On failure, ensure that either pptr() == nullptr or pptr() == epptr.*/
	if(pptr() == epptr())
	{
		if(sync() == -1)
		{
			setPutPos(mBufSize); // "on failure, ensure that either pptr == nullptr or pptr == epptr"
			return traits_type::eof();
		}

		setPutPos(0);
	}

	if(!traits_type::eq_int_type(c, traits_type::eof()))
		*pptr() = c;

	return traits_type::not_eof('_');
}

Process::Process():
	mIn(&mProcStdInBuf),
	mOut(&mProcStdOutBuf),
	mErr(&mProcStdErrBuf)
{
}

Process::~Process()
{
	if(mProcStdIn)  CloseHandle(mProcStdIn);
	if(mProcStdOut) CloseHandle(mProcStdOut);
	if(mProcStdErr) CloseHandle(mProcStdErr);

	if(mProcess) CloseHandle(mProcess);
}

namespace {

class IoHandleGuard
{
public:
	IoHandleGuard(size_t reservedSize, bool nullifyOnClose):
		mNullifyOnClose(nullifyOnClose)
	{
		mHandles.reserve(reservedSize);
	}

	IoHandleGuard(const initializer_list<HANDLE*>& handles, bool nullifyOnClose):
		mHandles(handles),
		mNullifyOnClose(nullifyOnClose)
	{
	}

	IoHandleGuard(const IoHandleGuard&) = delete;
	IoHandleGuard& operator=(const IoHandleGuard&) = delete;

	~IoHandleGuard() { closeHandles(mNullifyOnClose); }

	void closeHandles(bool andSetToNull)
	{
		for(HANDLE* h : mHandles)
			if(h && *h && *h != INVALID_HANDLE_VALUE)
			{
				CloseHandle(*h);
				if(andSetToNull)
					*h = 0;
			}
	}

	void reset()
	{
		mHandles.clear();
	}

private:
	vector<HANDLE*> mHandles;
	bool mNullifyOnClose;
};

class StartupInfo: public STARTUPINFOW
{
public:
	StartupInfo():
		mHandleGuard({&hStdInput, &hStdOutput, &hStdError}, false)
	{
		ZeroMemory((STARTUPINFOW*)this, sizeof(STARTUPINFOW));
		cb = sizeof(STARTUPINFO);
	}

private:
	IoHandleGuard mHandleGuard;
};

DWORD connectChildOutput(HANDLE& childOutHandle, HANDLE& pipeInHandle, const WCHAR* redirFilePath = 0)
{
	bool redirToFile = redirFilePath && *redirFilePath;

	SECURITY_ATTRIBUTES secAttrs;
	secAttrs.nLength = sizeof(secAttrs);
	secAttrs.bInheritHandle = TRUE;
	secAttrs.lpSecurityDescriptor = 0;

	if(redirToFile)
	{
		childOutHandle = CreateFileW(redirFilePath,
		                             GENERIC_WRITE, 0, &secAttrs,
		                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if(INVALID_HANDLE_VALUE == childOutHandle)
		{
			auto winApiError = GetLastError();
			gview.log() << "CreateFile.Error: " << winApiError;
			return winApiError;
		}
	}
	else
	{
		if(!CreatePipe(&pipeInHandle, &childOutHandle, &secAttrs, 0))
		{
			auto winApiError = GetLastError();
			gview.log() << "CreatePipe.Error: " << winApiError;
			return winApiError;
		}

		SetHandleInformation(pipeInHandle, HANDLE_FLAG_INHERIT, 0);
	}

	return ERROR_SUCCESS;
}

} //local namespace

OpStatus Process::run(const WCHAR* cmd, const WCHAR* workDir, const WCHAR* params,
                      const WCHAR* redirOutPath, const WCHAR* redirErrPath)
{
	StartupInfo startInfo;
	startInfo.dwFlags = STARTF_FORCEONFEEDBACK | STARTF_USESTDHANDLES;

	DWORD winApiError = 0;

	IoHandleGuard pipeHandleGuard({&mProcStdIn, &mProcStdOut, &mProcStdErr}, true);

	bool redirOutToFile = redirOutPath && *redirOutPath,
	     redirErrToFile = redirErrPath && *redirErrPath;

	winApiError = connectChildOutput(startInfo.hStdOutput, mProcStdOut, redirOutPath);
	if(winApiError != ERROR_SUCCESS)
		return OpStatus(OpStatus::SystemError, L"cannot read from git", winApiError);

	winApiError = connectChildOutput(startInfo.hStdError, mProcStdErr, redirErrPath);
	if(winApiError != ERROR_SUCCESS)
		return OpStatus(OpStatus::SystemError, L"cannot read from git", winApiError);

	PROCESS_INFORMATION procInfo;
	if(!CreateProcessW(cmd, (WCHAR*)params,
	                   0, 0, TRUE, //no security attributed; inherit handles
	                   NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
	                   NULL,
	                   workDir,
	                   &startInfo,
	                   &procInfo))
	{
		winApiError = GetLastError();
		gview.log() << "CreateProcess.Error: " << winApiError;

		mIn.setstate(basic_ios<wchar_t>::eofbit);
		mOut.setstate(basic_ios<wchar_t>::eofbit);
		mErr.setstate(basic_ios<wchar_t>::eofbit);

		return OpStatus(OpStatus::SystemError, L"git launch failed", winApiError);
	};

	mProcStdInBuf.setIoHandle(mProcStdIn);
	if(!redirOutToFile)  mProcStdOutBuf.setIoHandle(mProcStdOut);
	if(!redirErrToFile)  mProcStdErrBuf.setIoHandle(mProcStdErr);

	mIn.clear();
	mOut.clear();
	mErr.clear();

	if(procInfo.hThread) CloseHandle(procInfo.hThread);

	mProcess = procInfo.hProcess;

	pipeHandleGuard.reset();

	return OpStatus();
}

OpStatus Process::wait(unsigned timeout)
{
	OpStatus result;
	switch(WaitForSingleObject(mProcess, timeout * 1000))
	{
		case WAIT_OBJECT_0:
		break;
		case WAIT_TIMEOUT:
			result.mStatus = OpStatus::GitError;
			result.mDescr = L"git timed out";
		break;
		default:
			result.mStatus = OpStatus::Unexpected;
			result.mDescr = L"unknown error";
	};
	return result;
}