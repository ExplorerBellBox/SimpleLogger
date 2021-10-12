#pragma once

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#if defined(_MSC_VER) || (__GNUC__ >= 8)
#include <filesystem>
#else
#include <experimental/filesystem>
#endif
#include <regex>
#include <list>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <cassert>
#include <stdexcept>

#ifdef _WIN32
#define E_PATH_SEPARATOR  '\\'
#else
#define E_PATH_SEPARATOR  '/'
#endif

#define E_MAYBE_UNUSED    [[maybe_unused]]
#define E_NODISCARD       [[nodiscard]]

// log level
#define E_DEBUG  Simple::Logger::eDebug
#define E_INFO   Simple::Logger::eInfo
#define E_WARN   Simple::Logger::eWarn
#define E_ERROR  Simple::Logger::eError

// std color
#ifdef _WIN32
#define E_STD_COLOR_BLACK   (0)
#define E_STD_COLOR_RED     (FOREGROUND_RED)
#define E_STD_COLOR_GREEN   (FOREGROUND_GREEN)
#define E_STD_COLOR_YELLOW  (FOREGROUND_RED | FOREGROUND_GREEN)
#define E_STD_COLOR_BLUE    (FOREGROUND_BLUE)
#define E_STD_COLOR_PURPLE  (FOREGROUND_RED | FOREGROUND_BLUE)
#define E_STD_COLOR_CYAN    (FOREGROUND_GREEN | FOREGROUND_BLUE)
#define E_STD_COLOR_WHITE   (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#else
#define E_STD_COLOR_BLACK   "\033[30m"
#define E_STD_COLOR_RED     "\033[31m"
#define E_STD_COLOR_GREEN   "\033[32m"
#define E_STD_COLOR_YELLOW  "\033[33m"
#define E_STD_COLOR_BLUE    "\033[34m"
#define E_STD_COLOR_PURPLE  "\033[35m"
#define E_STD_COLOR_CYAN    "\033[36m"
#define E_STD_COLOR_WHITE   "\033[37m"
#endif

// source code position
#define E_LOG_POS  __FILE__, __LINE__, __FUNCTION__

// get logger inst
#define E_loggerInst  Simple::Logger::Inst()

// log methods
#define E_StdLog(_trace, _level, ...)   E_loggerInst.StdLog(E_LOG_POS, _level, _trace, __VA_ARGS__)
#define E_FileLog(_trace, _level, ...)  E_loggerInst.FileLog(E_LOG_POS, _level, _trace, __VA_ARGS__)
#define E_StdLogDiy(_level, ...)        E_loggerInst.StdLogDiy(_level, __VA_ARGS__)
#define E_FileLogDiy(_level, ...)       E_loggerInst.FileLogDiy(_level, __VA_ARGS__)

// useful log methods
#define E_Debug(_trace, ...)  E_FileLog(_trace, E_DEBUG, __VA_ARGS__)
#define E_Info(_trace, ...)   E_FileLog(_trace, E_INFO, __VA_ARGS__)
#define E_Warn(_trace, ...)   E_FileLog(_trace, E_WARN, __VA_ARGS__)
#define E_Error(_trace, ...)  E_FileLog(_trace, E_ERROR, __VA_ARGS__)
#define E_DiyDebug(...)       E_FileLogDiy(E_DEBUG, __VA_ARGS__)
#define E_DiyInfo(...)        E_FileLogDiy(E_INFO, __VA_ARGS__)
#define E_DiyWarn(...)        E_FileLogDiy(E_WARN, __VA_ARGS__)
#define E_DiyError(...)       E_FileLogDiy(E_ERROR, __VA_ARGS__)

// array helper
#define E_CountOf(_array)  sizeof(*Simple::CountOfHelper(_array))
#define E_ByteOf(_array)   sizeof(*Simple::ByteOfHelper(_array))

namespace Simple
{

template<typename T, size_t cnt>
inline
char
(*CountOfHelper(T (&)[cnt]))[cnt];

template<typename T, size_t cnt>
inline
char
(*ByteOfHelper(T (&)[cnt]))[sizeof(T) * cnt ];

/**
 * @brief
 * @note singleton class, keep singleton object during whole progress living time
 */
#ifdef _MSC_VER
class Logger final
#else
class __attribute__((visibility("default"), aligned(8))) Logger final
#endif
{
private:
	using Mutex = std::mutex;
	using SafeLock = std::unique_lock<Mutex>;
	using Condition = std::condition_variable;
	using LogQueue = std::list<std::string>;
	using FileQueue = std::list<std::string>;
	using ThreadPtr = std::shared_ptr<std::thread>;

public:
	// level
	enum : unsigned int { eDebug, eInfo, eWarn, eError, eCnt };

	static constexpr auto s_kFileByteDefault = size_t{1024} * 1024 * 5;     // 5MB
	static constexpr auto s_kFileByteAllowMax = size_t{1024} * 1024 * 1024; // 1GB
	static constexpr auto s_kFileByteAllowMin = size_t{1024} * 1;           // 1KB
	static constexpr auto s_kFileCntDefault = size_t{100};
	static constexpr auto s_kFileCntAllowMax = size_t{1000};
	static constexpr auto s_kFileCntAllowMin = size_t{1};
	static constexpr auto s_kFileStorePathDefault = "./Logs";

	static
	Logger &
	Inst()
	{
		static Logger _inst;
		return _inst;
	}

	~Logger()
	noexcept
	{
		StopFileLog();
		delete[] m_stdColor;
		delete[] m_strLevel;
	}

	Logger(const Logger &) = delete;

	Logger &
	operator=(const Logger &) = delete;

	E_MAYBE_UNUSED
	void
#ifdef _WIN32
	ConfigStd(unsigned int _recordLevel = E_INFO, bool _useColor = true,
			  const WORD(&_color)[Logger::eCnt] =
				  {E_STD_COLOR_WHITE, E_STD_COLOR_GREEN, E_STD_COLOR_YELLOW, E_STD_COLOR_RED})
#else
	ConfigStd(unsigned int _recordLevel = E_INFO, bool _useColor = true,
			  char const *(&_color)[Logger::eCnt] = (char const *[Logger::eCnt])
				  {E_STD_COLOR_WHITE, E_STD_COLOR_GREEN, E_STD_COLOR_YELLOW, E_STD_COLOR_RED})
#endif
	{
		SafeLock _sl(m_mutex);
		assert(!m_bLogStd); // should not call twice
		if (m_bLogStd)
		{
			return;
		}
		m_bLogStd = true;
		m_levelStd = (_recordLevel > E_ERROR) ? E_ERROR : _recordLevel;
		m_bColorStd = _useColor;
		if (m_bColorStd)
		{
#ifdef _WIN32
			m_stdColor = new WORD[Logger::eCnt]{_color[E_DEBUG], _color[E_INFO], _color[E_WARN], _color[E_ERROR]};
#else
			m_stdColor = new char const *[Logger::eCnt];
			m_stdColor[E_DEBUG] = (nullptr == _color[E_DEBUG]) ? E_STD_COLOR_WHITE : _color[E_DEBUG];
			m_stdColor[E_INFO] = (nullptr == _color[E_INFO]) ? E_STD_COLOR_GREEN : _color[E_INFO];
			m_stdColor[E_WARN] = (nullptr == _color[E_WARN]) ? E_STD_COLOR_YELLOW : _color[E_WARN];
			m_stdColor[E_ERROR] = (nullptr == _color[E_ERROR]) ? E_STD_COLOR_RED : _color[E_ERROR];
#endif
		}
	}

	E_MAYBE_UNUSED
	void
	ConfigFile(unsigned int _recordLevel = E_INFO, const std::string &_storeDirectory = Logger::s_kFileStorePathDefault,
			   size_t _byteMax = Logger::s_kFileByteDefault, size_t _cntMax = Logger::s_kFileCntDefault)
	{
		SafeLock _sl(m_mutex);
		assert(!m_bLogFile); // should not call twice
		if (m_bLogFile)
		{
			return;
		}
		m_levelFile = (_recordLevel > E_ERROR) ? E_ERROR : _recordLevel;
		m_strDir = EnsurePath(_storeDirectory);
		m_strName = GetExeName();
#define E_ENSURE_RANGE(_v, _max, _min)  (((_v) > (_max)) ? (_max) : (((_v) < (_min)) ? (_min) : (_v)))
		m_byteMax = E_ENSURE_RANGE(_byteMax, Logger::s_kFileByteAllowMax, Logger::s_kFileByteAllowMin);
		m_cntMax = E_ENSURE_RANGE(_cntMax, Logger::s_kFileCntAllowMax, Logger::s_kFileCntAllowMin);
#undef E_ENSURE_RANGE
		M_StdLog(E_LOG_POS, E_INFO, "log files were stored in (", m_strDir, "), prefix (", m_strName,
				 "), max size (", GetByteSizeString(m_byteMax, 1), "), max count (", m_cntMax, ")");
		ListExistLogFiles();
		RemoveOldLogFiles();
		m_bLogFile = true;
		m_bStop = false;
		// start the write file thread
		m_ptrWriteThread = std::make_shared<std::thread>(&Logger::WriteThread, this);
		const auto _interval = std::chrono::milliseconds{16};
		do
		{
			std::this_thread::sleep_for(_interval); // take a break, wait for thread start
		}
		while (!m_bWriteThreadAlive && !m_bStop);
	}

	E_MAYBE_UNUSED inline
	void
	ConfigAlwaysMarkSourceCodePosition()
	{
		SafeLock _sl(m_mutex);
		assert(!m_bAlwaysMarkSourceCodePosition); // should not call twice
		m_bAlwaysMarkSourceCodePosition = true;
	}

	template<typename ... Tn>
	E_MAYBE_UNUSED inline
	void
	StdLogDiy(unsigned int _level, const Tn &... tn)
	{
		if (m_bLogStd)
		{
			SafeLock _sl(m_mutex);
			PrintStdLog(Format(tn...), _level);
		}
	}

	template<typename ... Tn>
	E_MAYBE_UNUSED inline
	void
	StdLog(const char *__restrict _file, unsigned int _line, const char *__restrict _func,
		   unsigned int _level, const char *__restrict _trace, const Tn &... _tn)
	{
		assert(_file && _func);
		if (NeedRecordStd(_level))
		{
			SafeLock _sl(m_mutex);
			PrintStdLog(M_Format(_file, _line, _func, _level, _trace, _tn...), _level);
		}
	}

	template<typename ... Tn>
	E_MAYBE_UNUSED inline
	void
	StdLog(const char *__restrict _file, unsigned int _line, const char *__restrict _func,
		   unsigned int _level, const std::string &_trace, const Tn &... _tn)
	{
		return StdLog(_file, _line, _func, _level, _trace.c_str(), _tn...);
	}

	template<typename ... Tn>
	E_MAYBE_UNUSED inline
	void
	FileLogDiy(unsigned int _level, const Tn &... tn)
	{
		if (m_bLogFile && m_bWriteThreadAlive)
		{
			SafeLock _sl(m_mutex);
			auto _content = Format(tn...);
			if (m_bLogStd)
			{
				PrintStdLog(_content, _level);
			}
			m_queueLog.emplace_back(std::forward<std::string>(_content));
			m_cond.notify_one();
		}
		else if (m_bLogStd)
		{
			SafeLock _sl(m_mutex);
			PrintStdLog(Format(tn...), _level);
		}
	}

	template<typename ... Tn>
	E_MAYBE_UNUSED
	void
	FileLog(const char *__restrict _file, unsigned int _line, const char *__restrict _func,
			unsigned int _level, const char *__restrict _trace, const Tn &... _tn)
	{
		assert(_file && _func);
		if (NeedRecordFile(_level))
		{
			SafeLock _sl(m_mutex);
			auto strLog = M_Format(_file, _line, _func, _level, _trace, _tn...);
			if (NeedRecordStd(_level))
			{
				PrintStdLog(strLog, _level);
			}
			m_queueLog.emplace_back(std::forward<std::string>(strLog));
			m_cond.notify_one();
		}
		else if (NeedRecordStd(_level))
		{
			SafeLock _sl(m_mutex);
			PrintStdLog(M_Format(_file, _line, _func, _level, _trace, _tn...), _level);
		}
	}

	template<typename ... Tn>
	E_MAYBE_UNUSED inline
	void
	FileLog(const char *__restrict _file, unsigned int _line, const char *__restrict _func,
			unsigned int _level, const std::string &_trace, const Tn &... _tn)
	{
		return FileLog(_file, _line, _func, _level, _trace.c_str(), _tn...);
	}

	E_NODISCARD inline
	bool
	NeedRecordStd(unsigned int _level) const { return m_bLogStd && (_level >= m_levelStd); }

	E_NODISCARD inline
	bool
	NeedRecordFile(unsigned int _level) const { return m_bLogFile && m_bWriteThreadAlive && (_level >= m_levelFile); }

	[[maybe_unused, nodiscard]] inline
	bool
	NeedRecord(unsigned int _level) const { return NeedRecordStd(_level) || NeedRecordFile(_level); }

private:
	Logger() :
		m_strLevel(new (char const *[Logger::eCnt]){"Debug", "Info", "Warn", "Error"}),
		m_bAlwaysMarkSourceCodePosition(false),
		m_bLogStd(false), m_bColorStd(false), m_levelStd(E_INFO), m_stdColor(nullptr),
		m_bLogFile(false), m_bWriteThreadAlive(false), m_levelFile(E_INFO), m_writeErrorCnt(0),
		m_byteMax(Logger::s_kFileByteDefault), m_cntMax(Logger::s_kFileCntDefault), m_bStop(false) {}

	void
	StopFileLog()
	{
		if (m_ptrWriteThread)
		{
			{
				SafeLock _sl(m_mutex);
				m_bLogFile = false;
				m_bStop = true;
				m_cond.notify_all();
			}
			m_cond.notify_all();
			if (m_ptrWriteThread->joinable())
			{
				m_ptrWriteThread->join();
			}
			m_ptrWriteThread.reset();
		}
	}

	/**
	 * @example 2021-01-25 15:30:00.123 [Debug] it is a debug information
	 * @example 2021-01-25 15:30:00.345 [Info] it is a normal information
	 * @example 2021-01-25 15:30:00.567 [Warn] it is a warning information	[directories/source.cpp, 125, test_logger]
	 * @example 2021-01-25 15:30:00.789 [Error] it is an error information	[directories/source.cpp, 125, test_logger]
	 */
	template<typename ... Tn>
	E_NODISCARD
	std::string
	M_Format(const char *__restrict _file, unsigned int _line, const char *__restrict _func,
			 unsigned int _level, const char *__restrict _trace, const Tn &... tn)
	{
		assert(_level < Logger::eCnt);
		std::stringstream _ss;
		_ss.setf(std::ios::fixed);
		_ss.precision(3); // for float and double numbers
		_ss << GetTimestampForLogContent() << " [" << m_strLevel[_level] << "] ";
		if (_trace && ('\0' != _trace[0]))
		{
			_ss << "trace=" << _trace << " | ";
		}
		FormatHelper(_ss, tn...);
		if ((_level > E_INFO) || m_bAlwaysMarkSourceCodePosition)
		{
			_ss << "\t[" << _file << ", " << _line << ", " << _func << ']';
		}
		return _ss.str();
	}

	template<typename ... Tn>
	inline
	void
	M_StdLog(const char *__restrict _file, unsigned int _line, const char *__restrict _func,
			 unsigned int _level, const Tn &... tn)
	{
		PrintStdLog(M_Format(_file, _line, _func, _level, "logger", tn...), _level);
	}

	E_NODISCARD inline
	std::string
	MakeLogFileName()
	{
		return Format(m_strDir, E_PATH_SEPARATOR, m_strName, '_', GetTimestampForLogFileName(), ".log");
	}

	E_MAYBE_UNUSED
	void
	WriteThread()
	{
		assert(m_bLogFile);
		m_bWriteThreadAlive = true;
		static constexpr auto _maxInterval = std::chrono::seconds{1};
		auto _file = MakeLogFileName();
		size_t _byte = 0;
		LogQueue _logs;
		while (!m_bStop)
		{
			{
				SafeLock _sl(m_mutex);
				if (m_bStop)
				{
					break;
				}
				m_cond.wait_for(_sl, _maxInterval);
				if (!m_queueLog.empty())
				{
					std::swap(_logs, m_queueLog); // get all logs in queue
				}
			}

			if (!_logs.empty())
			{
				m_writeErrorCnt = 0;
				if (!WriteLogs(_logs, _file, _byte) || !_logs.empty())
				{
					M_StdLog(E_LOG_POS, E_WARN, "wrote log file errors, drop count ", _logs.size());
					_logs.clear();
				}
			}
		}

		m_bWriteThreadAlive = false;
		// write final logs
		{
			SafeLock _sl(m_mutex);
			if (!m_queueLog.empty())
			{
				std::swap(_logs, m_queueLog); // get all logs in queue
			}
		}

		if (!_logs.empty())
		{
			m_writeErrorCnt = 0;
			if (!WriteLogs(_logs, _file, _byte) || !_logs.empty())
			{
				M_StdLog(E_LOG_POS, E_WARN, "wrote log file errors, drop count ", _logs.size());
				_logs.clear();
			}
		}
	}

	E_NODISCARD
	bool
	WriteFile(LogQueue &_logs, const std::string &_file, size_t &_byte)
	{
		assert(!_file.empty());
		std::ofstream _ofs;
		_ofs.open(_file, std::ios_base::app | std::ios_base::out);
		if (!_ofs.good())
		{
			M_StdLog(E_LOG_POS, E_WARN, "open log file (", _file, ") failed");
			return false;
		}

		auto _pos = _ofs.tellp();
		if (std::ofstream::pos_type{-1} != _pos)
		{
			_byte = static_cast<size_t>(_pos);
		}

		while (!_logs.empty())
		{
			// check IO status
			if (!_ofs.good())
			{
				_ofs.clear(); // try ensure the file IO is always valid
				_ofs.flush();
				if (!_ofs.good())
				{
					M_StdLog(E_LOG_POS, E_WARN, "write log file (", _file, ") failed, bad IO");
					return false;
				}
			}
			// write log once
			_ofs << _logs.front() << '\n';
			// check file size
			if ((_pos = _ofs.tellp()) == std::ofstream::pos_type{-1})
			{
				_ofs.clear();
				_ofs.flush();
				if ((_pos = _ofs.tellp()) == std::ofstream::pos_type{-1})
				{
					M_StdLog(E_LOG_POS, E_WARN, "write log file (", _file, ") failed, bad IO");
					return false;
				}
			}
			// check whether was really written
			if (static_cast<size_t>(_pos) > _byte)
			{
				_logs.pop_front();
			}
			_byte = static_cast<size_t>(_pos);
			if (_byte >= m_byteMax)
			{
				return true;
			}
		}

		return true;
	}

	E_NODISCARD
	bool
	WriteLogs(LogQueue &_logs, std::string &_file, size_t &_byte)
	{
		auto _ok = WriteFile(_logs, _file, _byte);
		if (_ok && _logs.empty() && (_byte < m_byteMax))
		{
			return true;
		}

		if (!_ok)
		{
			if (++m_writeErrorCnt > 5)
			{
				return false;
			}
		}

		if (_byte)
		{
			m_queueFile.push_back(_file);
			RemoveOldLogFiles();
		}
		else
		{
			M_StdLog(E_LOG_POS, E_INFO, "try remove empty log file (", _file, ")");
			remove(_file.c_str());
		}
		// open new file
		_file = MakeLogFileName();
		// append info to last file
		if (_byte)
		{
			std::ofstream _ofs;
			_ofs.open(m_queueFile.back(), std::ios_base::app | std::ios_base::out);
			if (_ofs.good())
			{
				_ofs << "**************** See next logs in " << _file << " ****************" << std::endl;
			}
		}
		// append info to current file
		_logs.push_front(Format("**************** See previous logs in ", m_queueFile.back(), " ****************"));
		// continue write
		_byte = 0;
		return WriteLogs(_logs, _file, _byte);
	}

	void
	ListExistLogFiles()
	{
		m_queueFile.clear();
		try
		{
			FileQueue _queue;
			/// \warning the follow line runs error with gcc 4.8, so gcc 7.5 above was needed
			std::regex reg{m_strName + R"+(_\d{8}_\d{6}_\d{3}\.log)+"};
#if defined(_MSC_VER) || (__GNUC__ >= 8)
			for (const auto &item: std::filesystem::directory_iterator{m_strDir})
#else
			for (const auto &item: std::experimental::filesystem::directory_iterator{m_strDir})
#endif
			{
				auto _name = item.path().filename().string();
#if defined(_MSC_VER) || (__GNUC__ >= 8)
				if (item.is_regular_file() && std::regex_match(_name, reg))
#else
				if ((std::experimental::filesystem::file_type::regular == item.symlink_status().type())
					&& std::regex_match(_name, reg))
#endif
				{
					_queue.emplace_back(_name);
				}
			}

			if (!_queue.empty())
			{
				/// \brief the name was created by time, so sort by name equal to sort by file create time
				_queue.sort();
				for (const auto &item: _queue)
				{
					m_queueFile.emplace_back(Format(m_strDir, E_PATH_SEPARATOR, item));
				}
			}
		}
		catch (...)
		{
			M_StdLog(E_LOG_POS, E_WARN, "list log files in (", m_strDir, ") exception");
		}
	}

	void
	RemoveOldLogFiles()
	{
		if (m_queueFile.size() <= m_cntMax)
		{
			return;
		}
		const auto _cntReduce = m_queueFile.size() - m_cntMax;
		for (size_t i = 0; i < _cntReduce; ++i)
		{
			const auto &_file = m_queueFile.front();
			if (remove(_file.c_str()) == 0)
			{
				M_StdLog(E_LOG_POS, E_INFO, "remove log file (", _file, ") success");
			}
			else
			{
				M_StdLog(E_LOG_POS, E_WARN, "remove log file (", _file, ") failed");
			}
			m_queueFile.pop_front();
		}
	}

	inline
	void
	PrintStdLog(const std::string &_log, unsigned int _level)
	{
		if (m_bColorStd)
		{
#ifdef _WIN32
			auto _h = GetStdHandle(STD_OUTPUT_HANDLE);
			CONSOLE_SCREEN_BUFFER_INFO _old{};
			GetConsoleScreenBufferInfo(_h, std::addressof(_old));
			SetConsoleTextAttribute(_h, m_stdColor[_level]);
			std::cout << _log << '\n';
			SetConsoleTextAttribute(_h, _old.wAttributes);
			OutputDebugStringA((_log + "\r\n").c_str());
#else
			std::cout << m_stdColor[_level] << _log << "\033[0m" << '\n';
#endif
		}
		else
		{
			std::cout << _log << '\n';
#ifdef _WIN32
			OutputDebugStringA((_log + "\r\n").c_str());
#endif
		}
	}

	template<typename T=size_t>
	E_NODISCARD
	std::string
	GetByteSizeString(T _byte, int _precision = 3)
	{
		assert(std::is_integral<T>::value);
		const char *_p = nullptr;
		double _d = 1.0 * _byte;
#define E_BYTE_SCALE  (1024.0)
		if (_d < E_BYTE_SCALE)
		{
			_p = "B";
		}
		else if ((_d /= E_BYTE_SCALE) < E_BYTE_SCALE)
		{
			_p = "KB";
		}
		else if ((_d /= E_BYTE_SCALE) < E_BYTE_SCALE)
		{
			_p = "MB";
		}
		else if ((_d /= E_BYTE_SCALE) < E_BYTE_SCALE)
		{
			_p = "GB";
		}
		else if ((_d /= E_BYTE_SCALE) < E_BYTE_SCALE)
		{
			_p = "TB";
		}
		else
		{
			_d /= E_BYTE_SCALE;
			_p = "PB";
		}
#undef E_BYTE_SCALE
		return Format(std::setprecision(_precision), _d, _p);
	}

	E_NODISCARD static
	char
	(&GetTimestampForLogFileName())[32]
	{
		// note, this function would always be used with mutex, so the follow 4 arguments can be static
		static struct tm _t{};
		static char _timestamp[32] = {0};
		static uint64_t _milliSeconds = 0;
		static time_t _seconds = 0;

		_milliSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		_seconds = static_cast<time_t>(_milliSeconds / uint64_t{1000});
#ifdef _WIN32
		localtime_s(std::addressof(_t), std::addressof(_seconds));
#else
		localtime_r(std::addressof(_seconds), std::addressof(_t));
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
		snprintf(_timestamp, E_ByteOf(_timestamp), "%04d%02d%02d_%02d%02d%02d_%03d",
				 _t.tm_year + 1900, _t.tm_mon + 1, _t.tm_mday,
				 _t.tm_hour, _t.tm_min, _t.tm_sec, static_cast<int>(_milliSeconds % uint64_t{1000}));
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
		return _timestamp;
	}

	E_NODISCARD static
	auto
	GetTimestampForLogContent() -> char (&)[32]
	{
		// note, this function would always be used with mutex, so the follow 4 arguments can be static
		static struct tm _t{};
		static char _timestamp[32] = {0};
		static uint64_t _milliSeconds = 0;
		static time_t _seconds = 0;

		_milliSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		_seconds = static_cast<time_t>(_milliSeconds / uint64_t{1000});
#ifdef _WIN32
		localtime_s(std::addressof(_t), std::addressof(_seconds));
#else
		localtime_r(std::addressof(_seconds), std::addressof(_t));
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
		snprintf(_timestamp, E_ByteOf(_timestamp), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
				 _t.tm_year + 1900, _t.tm_mon + 1, _t.tm_mday,
				 _t.tm_hour, _t.tm_min, _t.tm_sec, static_cast<int>(_milliSeconds % uint64_t{1000}));
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
		return _timestamp;
	}

	template<typename T>
	inline
	void
	FormatHelper(std::stringstream &_ss, const T &t)
	{
		_ss << t;
	}

	template<typename T1, typename ...Tn>
	inline
	void
	FormatHelper(std::stringstream &_ss, const T1 &t1, const Tn &...tn)
	{
		_ss << t1;
		FormatHelper(_ss, tn...);
	}

	template<typename ...Args>
	E_NODISCARD inline
	std::string
	Format(const Args &...args)
	{
		std::stringstream _ss;
#if defined(_MSC_VER) || (__cplusplus >= 201703L) // __cplusplus is always 199711L in visual studio
		(_ss << ... << args); // since C++17
#else
		FormatHelper(_ss, args...);
#endif
		return _ss.str();
	}

	E_NODISCARD static inline
	std::string
	GetExeFullPath()
	{
		char _path[1024 + 8] = {0};
#ifdef _WIN32
		auto _cnt = GetModuleFileNameA(nullptr, _path, 1024);
		assert(_cnt > 3); // like c:\\aaa\\bbb\\ccc
#else
		auto _cnt = readlink("/proc/self/exe", _path, 1024);
		assert(_cnt > 1); // like /aaa/bbb/ccc
#endif
		return (_cnt > 0) ? std::string(_path, _cnt) : "";
	}

	/***
	 * @warning not include the last separator
	 */
	E_NODISCARD static inline
	std::string
	GetExeDir()
	{
		const auto _path = GetExeFullPath();
		const auto _pos = _path.rfind(E_PATH_SEPARATOR);
		assert(std::string::npos != _pos);
		return (std::string::npos == _pos) ? "" : _path.substr(0, (1 > _pos) ? 1 : _pos);
	}

	E_NODISCARD static inline
	std::string
	GetExeName()
	{
		const auto _path = GetExeFullPath();
		const auto _pos = _path.rfind(E_PATH_SEPARATOR);
		assert(std::string::npos != _pos);
		return (std::string::npos == _pos) ? _path : _path.substr(_pos + 1, _path.size() - _pos - 1);
	}

	static
	std::string
	EnsurePath(const std::string &_path, bool bCreateIfNotExist = true)
	{
		auto _exeDir = GetExeDir();
		if (_path.empty())
		{
			return _exeDir;
		}
		const auto _dir = ('.' == _path[0]) ? (("/" == _exeDir) ? (_exeDir + _path) : (_exeDir + "/" + _path)) : _path;

		try
		{
#if defined(_MSC_VER) || (__GNUC__ >= 8)
			const auto _p = std::filesystem::absolute(std::filesystem::path{_dir});
			if (std::filesystem::is_directory(_p))
			{
				return _p.string();
			}
			else if (bCreateIfNotExist)
			{
				if (std::filesystem::create_directories(_p))
				{
					return _p.string();
				}
			}
#else
			const auto _p = std::experimental::filesystem::absolute(std::experimental::filesystem::path{_dir});
			if (std::experimental::filesystem::is_directory(_p))
			{
				return _p.string();
			}
			else if (bCreateIfNotExist)
			{
				if (std::experimental::filesystem::create_directories(_p))
				{
					return _p.string();
				}
			}
#endif
		}
		catch (...)
		{
		}
		return _exeDir;
	}

private:
	char const **m_strLevel;

	bool m_bAlwaysMarkSourceCodePosition;

	// standard log
	bool m_bLogStd;
	bool m_bColorStd;
	unsigned int m_levelStd;
#ifdef _WIN32
	const WORD *m_stdColor;
#else
	char const **m_stdColor;
#endif

	// file log
	bool m_bLogFile;
	bool m_bWriteThreadAlive;
	unsigned int m_levelFile;
	unsigned int m_writeErrorCnt;
	size_t m_byteMax;           // log file max byte size
	size_t m_cntMax;            // log file max count
	std::atomic_bool m_bStop;
	std::string m_strDir;       // log directory
	std::string m_strName;      // log file base name
	LogQueue m_queueLog;        // the log queue wait for writing
	FileQueue m_queueFile;      // the previous file queue
	ThreadPtr m_ptrWriteThread; // write file thread

	Mutex m_mutex;
	Condition m_cond;
};

}

#undef E_PATH_SEPARATOR
