# CLogManager

A lightweight and easy-to-use C++ logging class that supports multi-threaded environments.

</br>

## ✨ Features
- Thread-safe class (Singleton access)
- Easy to use with macro function
- Various Log levels (TRACE, DEBUG, INFO, WARN, ERROR, FATAL)
- Automatic log file rotation (100 MB max per file)
- Logs stored in per-day folders (e.g., `C:\MyLog\20250612`)
- Configurable log level via `log_conf.ini`


</br>

## 📁 File Structure
- `CLogManager.h` – Header file for the logging manager
- `CLogManager.cpp` – Implementation file

</br>

## 🔧 Configuration

Write `C:/MyLog/log_conf.ini` with the following content:

~~~ini
[LOG_SETTING]
log_level = 1
# 0: trace, 1: debug, 2: info, 3: warn, 4: error, 5: fatal
~~~

<br/>

## 🛠️ Usage

### Include the header and declare the class object
~~~cpp
#include "CLogManager.h"


int main ()
{
    CLogManager CLogManager;

    return 0;
}
~~~

<!-- <br/> -->

### Log a message macro function
~~~cpp
void callbackFuncB(const double& data)
{
    LOG_DEBUG("this is callback B");
    LOG_WARN("received data is : %f", data);
}

void callbackFuncA()
{
    LOG_INFO("this is callback A");

    // Possible Execption..
    LOG_ERROR("Error occured :(");
}
~~~

<br/>

## 📂 Example Log Output
Logged path : `C:\MyLog\20250612\20250612_134508.txt`

~~~rust
[2025-06-12 13:45:30.112] [INFO] callbackFuncA (Line : 5) -> this is callback A
[2025-06-12 13:45:30.114] [DEBUG] callbackFuncB (Line : 2) -> this is callback B
[2025-06-12 13:45:30.113] [ERROR] callbackFuncA (Line : 8) -> Error occured :(
[2025-06-12 13:45:31.116] [WARN] callbackFuncB (Line : 3) -> received data is 123.45
~~~

<br/>

## 📌 Notes
* Log files are automatically created inside daily folders `C:\MyLog\YYYYMMDD`.
* If a log file exceeds **100 MB**, a new file with timestamp is created.
* Log output is buffered via an internal queue and flushed by a worker thread.
* Uses standard C++17 (`<thread>`, `<mutex>`, `<chrono>`, `<filesystem>`).

<br/>

## 📄 License

This project is licensed under the MIT License – see the [LICENSE](./LICENSE) file for details.

<br/>

## 🧑‍💻 Author
- Name: Seongchang Park
- Email: scsc1125@gmail.com
