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
#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <afxstr.h>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <afx.h>
#include <cstdarg>  // va_list

// �α� ��ũ�� (�ܺο����� �� �Լ��鸸 ����ؾ� ��)
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
	static CLogManager& getInstance(); // Ŭ���� �ν��Ͻ� ��ȯ (singleton)
	void logMessage(LogLevel level, const char* msg, const char* function, int line, ...); // �α� �޼��� �ۼ� �Լ�

private:
	std::queue<std::pair<LogLevel, std::string>> log_queue_; // �α� queue
	std::mutex mutex_;				// ����ȭ�� mutex
	std::condition_variable cv_;	// ���/�˸��� ���� conditional variable
	std::thread log_worker_thread_;	// �α� ó���ϴ� work thread

	std::fstream current_log_file_; // ���� �α� ����
	size_t current_log_file_size_;	// ���� �α� ���� ������
	std::string log_directory_;		// �α� ���丮
	LogLevel log_level_;			// �α� ����

	void readInitialLogSetting();	// �α� ���� �о����
	void logWorker(); // �α� ó�� worker ������ �Լ�
	std::string convertLogLevelToString(LogLevel level); // �α� ������ ���ڿ��� ��ȯ
	void writeLogToFile(const std::pair<LogLevel, std::string>& message); // �α׸� �ؽ�Ʈ ���Ϸ� ����
};
// EOF