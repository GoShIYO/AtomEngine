#include "LogSystem.h"
#include <fstream>
#include <stringapiset.h>
#include <cstdarg>
#include <filesystem>

namespace AtomEngine
{
	std::ofstream logStream;

	void AtomEngine::InitLog()
	{
#ifdef _DEBUG
		std::filesystem::create_directory("logs");

		// 現在時刻を取得（UTC時刻）
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		// ログファイルの名前にコンマ何秒はいらないので、削って秒にする
		std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
			nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
		// 日本時間（PCの設定時間）に変換
		std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
		// formatを使って年月日_時分秒の文字列に変換
		std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
		// 時刻を使ってファイル名を決定
		std::string logFilePath = std::string("logs/") + dateString + ".log";
		// ファイルを開く
		logStream.open(logFilePath, std::ios::out);
#endif // _DEBUG
	}

	void FinalizeLog()
	{
		if (logStream.is_open())
		{
            logStream.close();
		}
	}

	void AtomEngine::Log(const char* format, ...)
	{
#ifdef _DEBUG
		char buffer[256];
		va_list ap;
		va_start(ap, format);
		vsprintf_s(buffer, 256, format, ap);
		va_end(ap);

		logStream << buffer << std::endl;
		Print(buffer);
#endif // _DEBUG
	}

	void AtomEngine::Log(const wchar_t* format, ...)
	{
#ifdef _DEBUG
		wchar_t buffer[256];
		va_list ap;
		va_start(ap, format);
		vswprintf(buffer, 256, format, ap);
		va_end(ap);

		logStream << WStringToUTF8(buffer) << std::endl;
		Print(buffer);
#endif // _DEBUG
	}
}
