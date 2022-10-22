#pragma once

#include "pch.h"

#include "DataStructs.h"

#include <istream>
#include <ostream>
#include <streambuf>
#include <string>

class Process
{
	class IOBuf: public std::basic_streambuf<wchar_t>
	{
	public:
		void setIoHandle(HANDLE ioHandle);

	protected:
		IOBuf(HANDLE ioHandle = 0);

		pos_type calcNewPos(std::streampos startPos, std::streamoff offset, std::ios_base::seekdir startFrom) const;
		void setGetPos(std::streampos pos);
		void setPutPos(std::streampos pos);

		bool handleAvailable() const;

	protected:
		HANDLE mIoHandle;

		static constexpr size_t mBufSize = 1024;
		wchar_t mBuf[mBufSize] = {0};
	};

	class InBuf: public IOBuf
	{
		typedef IOBuf Super;

	public:
		InBuf(HANDLE ioHandle = 0): Super(ioHandle) { }

		virtual pos_type seekoff(off_type offset, std::ios_base::seekdir startFrom,
		                         std::ios_base::openmode which) override;

		virtual std::streamsize showmanyc() override;
		virtual int_type underflow() override;
		virtual std::streamsize xsgetn(char_type* s, std::streamsize count) override;

		bool hasData() const; //data available for reading from buffer or pipe

	private:
		bool pump(int_type& pumpedChar); //pump from ioHandle into buffer
	};
	class OutBuf: public IOBuf
	{
		typedef IOBuf Super;

	public:
		OutBuf(HANDLE ioHandle = 0): Super(ioHandle) { }

		virtual pos_type seekoff(off_type offset, std::ios_base::seekdir startFrom,
		                         std::ios_base::openmode which) override;
		virtual int sync() override;

		virtual std::streamsize xsputn(const char_type* s, std::streamsize count) override;
		virtual int_type overflow(int_type c) override;
	};

public:
	Process();
	~Process();

	//strart 'cmd params' in 'workDir'
	//if redirection file paths are provided, redirect stdout/stderr to them instead of connecting mOut/mErr streams
	OpStatus run(const WCHAR* cmd, const WCHAR* workDir, const WCHAR* params,
	             const WCHAR* redirOutPath = 0, const WCHAR* redirErrPath = 0);

	OpStatus wait(unsigned timeout);

	const OpStatus& runStatus() const { return mRunStatus; }

public:
	std::wostream mIn;
	std::wistream mOut;
	std::wistream mErr;

protected:
	OutBuf mProcStdInBuf;
	InBuf  mProcStdOutBuf;
	InBuf  mProcStdErrBuf;

private:
	HANDLE mProcess = 0;
	HANDLE mProcStdIn = 0;
	HANDLE mProcStdOut = 0;
	HANDLE mProcStdErr = 0;

	OpStatus mRunStatus;
};