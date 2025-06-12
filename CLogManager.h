/*****************************************************************//**
 * \file   CLogManager.h
 * \brief  로그 관리 클래스 헤더 파일
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
#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <cstdarg>  // va_list
#include <Windows.h>

 // 로그 매크로 (외부에서는 이 함수들만 사용해야 함)
#define LOG_TRACE(msg, ...)CLogManager::getInstance().logMessage(LogLevel::LOG_TRACE, msg, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(msg, ...)CLogManager::getInstance().logMessage(LogLevel::LOG_DEBUG, msg, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_INFO(msg, ...) CLogManager::getInstance().logMessage(LogLevel::LOG_INFO, msg, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_WARN(msg, ...) CLogManager::getInstance().logMessage(LogLevel::LOG_WARN, msg, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(msg, ...)CLogManager::getInstance().logMessage(LogLevel::LOG_ERROR, msg, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(msg, ...)CLogManager::getInstance().logMessage(LogLevel::LOG_FATAL, msg, __FUNCTION__, __LINE__, __VA_ARGS__)

enum class LogLevel
{
	LOG_TRACE,
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
	LOG_FATAL
};

class CLogManager
{
	CLogManager();
	~CLogManager();

public:
	static CLogManager& getInstance(); // 클래스 인스턴스 반환 (singleton)
	void logMessage(LogLevel level, const char* msg, const char* function, int line, ...); // 로그 메세지 작성 함수

private:
	std::queue<std::pair<LogLevel, std::string>> log_queue_; // 로그 queue
	std::mutex mutex_;				// 동기화용 mutex
	std::condition_variable cv_;	// 대기/알림을 위한 conditional variable
	std::thread log_worker_thread_;	// 로그 처리하는 work thread

	std::fstream current_log_file_; // 현재 로그 파일
	size_t current_log_file_size_;	// 현재 로그 파일 사이즈
	std::string log_directory_;		// 로그 디렉토리
	LogLevel log_level_;			// 로그 레벨

	void readInitialLogSetting();	// 로그 레벨 읽어오기
	void logWorker(); // 로그 처리 worker 스레드 함수
	std::string convertLogLevelToString(LogLevel level); // 로그 레벨을 문자열로 변환
	void writeLogToFile(const std::pair<LogLevel, std::string>& message); // 로그를 텍스트 파일로 저장
};
// EOF
