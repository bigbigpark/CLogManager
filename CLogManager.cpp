/*****************************************************************//**
 * \file   CLogManager.cpp
 * \brief  �α� ���� Ŭ���� ����
 *
 * \author Seongchang Park
 * \date   26 November 2024
 * 
 * \copyright
 * MIT License
 * 
 * Copyright (c) 2024 Seongchang Park
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *********************************************************************/
#include "pch.h"
#include "CLogManager.h"

CLogManager::CLogManager() :
log_directory_(_T("C:\\MyLog")),
current_log_file_size_(0)
{
	// �α� ���� �о����
	readInitialLogSetting();

	// ���� ��¥ ���丮 ����
	std::time_t t = std::time(nullptr);
	std::tm tm_tm = *std::localtime(&t);

	char buffer[9];
	(void)std::strftime(buffer, sizeof(buffer), "%Y%m%d", &tm_tm);
	log_directory_ += "\\" + std::string(buffer);

	// ���丮�� ������ �ڵ� ����
	(void)std::filesystem::create_directories(log_directory_);

	// �α� ó�� ������ ����
	log_worker_thread_ = std::thread(&CLogManager::logWorker, this);
}

CLogManager::~CLogManager()
{
	// ������ ���� ���
	if (log_worker_thread_.joinable())
	{
		log_worker_thread_.join();
	}

	// �α� ���� �ݱ�
	if (current_log_file_.is_open())
	{
		current_log_file_.close();
	}
}

// singleton �ν��Ͻ� ����
CLogManager& CLogManager::getInstance()
{
	static CLogManager instance;
	return instance;
}

void CLogManager::logMessage(LogLevel level, const char* msg, const char* function, int line, ...)
{
	std::lock_guard<std::mutex> lock(mutex_);

	// ���� �ð� ��� (�ý��� �ð�)
	auto now = std::chrono::system_clock::now();

	// ���� �ð��� �ý��� �ð�� ��ȯ (�� ����)
	// MISRA_CPP_17_00_02 time ���� �� ���� ����
	std::time_t temptime = std::chrono::system_clock::to_time_t(now);
	struct tm* time_info = std::localtime(&temptime);

	// �и��� ���� �ð� ���
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

	// �ð� ������: YYYY-MM-DD HH:MM:SS
	std::stringstream time_stream;
	time_stream << std::put_time(time_info, "%Y-%m-%d %H:%M:%S");

	// �и��ʸ� �߰�
	time_stream << "." << std::setw(3) << std::setfill('0') << milliseconds.count();

	// ���� ���� ����Ʈ ó��
	va_list args;
	va_start(args, line);  // line�� ���� ������ ������ ��ġ

	// �α� �޽��� �������� ���� ����
	char buffer[1024];
	(void)vsnprintf(buffer, sizeof(buffer), msg, args);  // ���� ���� ó��

	va_end(args);  // ���� ���� ����

	// �α� �޽��� ������
	std::ostringstream log_msg_stream;
	log_msg_stream << "[" << time_stream.str() << "] "
		<< "[" << convertLogLevelToString(level) << "] "
		<< function << " (Line : " << line << ") -> "
		<< buffer;

	// �α� �޽��� �ϼ�
	std::string log_msg = log_msg_stream.str();

	// �α� �޼��� �� (�α׷��� - �α׸޼���)
	// MISRA_CPP_17_00_02 log ���� �� ���� ����
	std::pair<LogLevel, std::string> tmplog;
	tmplog.first = level;
	tmplog.second = log_msg;

	log_queue_.push(tmplog);
	cv_.notify_one();
}

// �α� ���� �о����
void CLogManager::readInitialLogSetting()
{
	// �α� ���� ���� ��� 
	CString log_config_file = _T("C:/MyLog/log_conf.ini");

	// [LOG_SETTING] ���ǿ��� log_level �� �о���� (��������)
	TCHAR buffer[MAX_PATH] = { 0, };
	GetPrivateProfileString(_T("LOG_SETTING"), _T("log_level"), _T(""), buffer, MAX_PATH, log_config_file);
	int log_level = 1;

	try
	{
		log_level = std::stoi(std::string(buffer));
	}
	catch (const std::exception&)
	{
		if ((log_level !=4) || (log_level < 0)) {
			log_level = 4;
		}
		LOG_ERROR("log_level ������ ����");
	}

	// �α� ���� ����
	switch (log_level)
	{
	case 0:
		log_level_ = LogLevel::LOG_TRACE;
		break;
	case 1:
		log_level_ = LogLevel::LOG_DEBUG;
		break;
	case 2:
		log_level_ = LogLevel::LOG_INFO;
		break;
	case 3:
		log_level_ = LogLevel::LOG_WARN;
		break;
	case 4:
		log_level_ = LogLevel::LOG_ERROR;
		break;
	case 5:
		log_level_ = LogLevel::LOG_FATAL;
		break;
	default:
		log_level_ = LogLevel::LOG_DEBUG;
		break;
	}
}

// �α� ó�� worker ������ �Լ�
void CLogManager::logWorker()
{
	while (true)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		cv_.wait(lock); // ť�� �αװ� ä�����⸦ ��� 

		while (!log_queue_.empty())
		{
			// ť���� �α׸� ��
			std::pair log_msg = log_queue_.front();
			log_queue_.pop();

			// �α׸� ���Ͽ� ���
			writeLogToFile(log_msg);
		}
	}
}

// �α� ������ ���ڿ��� ��ȯ
std::string CLogManager::convertLogLevelToString(LogLevel level)
{
	std::string level_str;

	switch (level)
	{
	case LogLevel::LOG_TRACE:
		level_str = _T("TRACE");
		break;
	case LogLevel::LOG_DEBUG:
		level_str = _T("DEBUG");
		break;
	case LogLevel::LOG_INFO:
		level_str = _T("INFO");
		break;
	case LogLevel::LOG_WARN:
		level_str = _T("WARN");
		break;
	case LogLevel::LOG_ERROR:
		level_str = _T("ERROR");
		break;
	case LogLevel::LOG_FATAL:
		level_str = _T("FATAL");
		break;
	default:
		level_str = _T("UNKNOWN");
		break;
	}
	return level_str;
}

// �α׸� �ؽ�Ʈ ���Ϸ� ����
void CLogManager::writeLogToFile(const std::pair<LogLevel, std::string>& message)
{
	// �α� ������ �������� �ʰų�, �뷮�� 100 MB�� ������ �� ���Ϸ� ����Ѵ�
	if ((!current_log_file_.is_open()) || (current_log_file_size_ >= 100 * 1024 * 1024))
	{
		// ���� ��¥�� �ð� ������� ���� ���� [������_�ú���]
		std::time_t t = std::time(nullptr);
		std::tm tm_tm = *std::localtime(&t);
		char file_name[20];
		size_t tm_size = std::strftime(file_name, sizeof(file_name), "%Y%m%d_%H%M%S.txt", &tm_tm);

		if (tm_size == 0)
		{
			// ���� ó��
			return;
		}
		else
		{
			std::string file_path = log_directory_ + "\\" + file_name;

			// �� ���� ����
			current_log_file_.open(file_path, std::ios::out | std::ios::app);
			current_log_file_size_ = 0; // ���ο� �����̹Ƿ� ũ�� �ʱ�ȭ
		}
	}

	// �α� ������ ���������� ������ ����
	if (current_log_file_.is_open())
	{
		// �α� ���� �̻��� �޼����� ���
		if (message.first >= log_level_)
		{
			// ���Ͽ� �α� �޼��� ���
			current_log_file_ << message.second << std::endl;

			// ���� ũ�� ����
			current_log_file_size_ += message.second.size();
		}
	}
}
// EOF