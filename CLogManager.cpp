/*****************************************************************//**
 * \file   CLogManager.cpp
 * \brief  로그 관리 클래스 구현
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
#include "CLogManager.h"

CLogManager::CLogManager() :
	log_directory_("C:\\MyLog"),
	current_log_file_size_(0)
{
	// 로그 레벨 읽어오기
	readInitialLogSetting();

	// 오늘 날짜 디렉토리 생성
	std::time_t t = std::time(nullptr);
	std::tm tm_tm = *std::localtime(&t);

	char buffer[9];
	(void)std::strftime(buffer, sizeof(buffer), "%Y%m%d", &tm_tm);
	log_directory_ += "\\" + std::string(buffer);

	// 디렉토리가 없으면 자동 생성
	(void)std::filesystem::create_directories(log_directory_);

	// 로그 처리 스레드 시작
	log_worker_thread_ = std::thread(&CLogManager::logWorker, this);
}

CLogManager::~CLogManager()
{
	// 쓰레드 종료 대기
	if (log_worker_thread_.joinable())
	{
		log_worker_thread_.join();
	}

	// 로그 파일 닫기
	if (current_log_file_.is_open())
	{
		current_log_file_.close();
	}
}

// singleton 인스턴스 접근
CLogManager& CLogManager::getInstance()
{
	static CLogManager instance;
	return instance;
}

void CLogManager::logMessage(LogLevel level, const char* msg, const char* function, int line, ...)
{
	std::lock_guard<std::mutex> lock(mutex_);

	// 현재 시간 얻기 (시스템 시계)
	auto now = std::chrono::system_clock::now();

	// 현재 시간을 시스템 시계로 변환 (초 단위)
	// MISRA_CPP_17_00_02 time 선언 및 금지 변수
	std::time_t temptime = std::chrono::system_clock::to_time_t(now);
	struct tm* time_info = std::localtime(&temptime);

	// 밀리초 단위 시간 계산
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

	// 시간 포맷팅: YYYY-MM-DD HH:MM:SS
	std::stringstream time_stream;
	time_stream << std::put_time(time_info, "%Y-%m-%d %H:%M:%S");

	// 밀리초를 추가
	time_stream << "." << std::setw(3) << std::setfill('0') << milliseconds.count();

	// 가변 인자 리스트 처리
	va_list args;
	va_start(args, line);  // line은 가변 인자의 마지막 위치

	// 로그 메시지 포맷팅을 위한 버퍼
	char buffer[1024];
	(void)vsnprintf(buffer, sizeof(buffer), msg, args);  // 가변 인자 처리

	va_end(args);  // 가변 인자 종료

	// 로그 메시지 포맷팅
	std::ostringstream log_msg_stream;
	log_msg_stream << "[" << time_stream.str() << "] "
		<< "[" << convertLogLevelToString(level) << "] "
		<< function << " (Line : " << line << ") -> "
		<< buffer;

	// 로그 메시지 완성
	std::string log_msg = log_msg_stream.str();

	// 로그 메세지 페어링 (로그레벨 - 로그메세지)
	// MISRA_CPP_17_00_02 log 선언 및 금지 변수
	std::pair<LogLevel, std::string> tmplog;
	tmplog.first = level;
	tmplog.second = log_msg;

	log_queue_.push(tmplog);
	cv_.notify_one();
}

// 로그 레벨 읽어오기
void CLogManager::readInitialLogSetting()
{
	// 로그 설정 파일 경로 
	std::string log_config_file = "C:/MyLog/log_conf.ini";

	// [LOG_SETTING] 섹션에서 log_level 값 읽어오기 (정수값만)
	char buffer[260] = { 0, };
	GetPrivateProfileStringA("LOG_SETTING", "log_level", "", buffer, MAX_PATH, log_config_file.c_str());
	int log_level = 1;

	try
	{
		log_level = std::stoi(std::string(buffer));
	}
	catch (const std::exception&)
	{
		if ((log_level != 4) || (log_level < 0)) {
			log_level = 4;
		}
		LOG_ERROR("log_level 설정값 오류");
	}

	// 로그 레벨 설정
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

// 로그 처리 worker 스레드 함수
void CLogManager::logWorker()
{
	while (true)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		//cv_.wait(lock); 
		cv_.wait(lock, [this] { return !log_queue_.empty(); }); // 큐에 로그가 채워지기를 대기 

		while (!log_queue_.empty())
		{
			// 큐에서 로그를 뺌
			std::pair log_msg = log_queue_.front();
			log_queue_.pop();

			// 로그를 파일에 기록
			writeLogToFile(log_msg);
		}
	}
}

// 로그 레벨을 문자열로 변환
std::string CLogManager::convertLogLevelToString(LogLevel level)
{
	std::string level_str;

	switch (level)
	{
	case LogLevel::LOG_TRACE:
		level_str = "TRACE";
		break;
	case LogLevel::LOG_DEBUG:
		level_str = "DEBUG";
		break;
	case LogLevel::LOG_INFO:
		level_str = "INFO";
		break;
	case LogLevel::LOG_WARN:
		level_str = "WARN";
		break;
	case LogLevel::LOG_ERROR:
		level_str = "ERROR";
		break;
	case LogLevel::LOG_FATAL:
		level_str = "FATAL";
		break;
	default:
		level_str = "UNKNOWN";
		break;
	}
	return level_str;
}

// 로그를 텍스트 파일로 저장
void CLogManager::writeLogToFile(const std::pair<LogLevel, std::string>& message)
{
	// 로그 파일이 열려있지 않거나, 용량이 100 MB를 넘으면 새 파일로 기록한다
	if ((!current_log_file_.is_open()) || (current_log_file_size_ >= 100 * 1024 * 1024))
	{
		// 현재 날짜와 시간 기반으로 파일 생성 [연월일_시분초]
		std::time_t t = std::time(nullptr);
		std::tm tm_tm = *std::localtime(&t);
		char file_name[20];
		size_t tm_size = std::strftime(file_name, sizeof(file_name), "%Y%m%d_%H%M%S.txt", &tm_tm);

		if (tm_size == 0)
		{
			// 오류 처리
			return;
		}
		else
		{
			std::string file_path = log_directory_ + "\\" + file_name;

			// 새 파일 오픈
			current_log_file_.open(file_path, std::ios::out | std::ios::app);
			current_log_file_size_ = 0; // 새로운 파일이므로 크기 초기화
		}
	}

	// 로그 파일이 성공적으로 열렸을 때만
	if (current_log_file_.is_open())
	{
		// 로그 레벨 이상인 메세지만 기록
		if (message.first >= log_level_)
		{
			// 파일에 로그 메세지 기록
			current_log_file_ << message.second << std::endl;

			// 파일 크기 갱신
			current_log_file_size_ += message.second.size();
		}
	}
}
// EOF
