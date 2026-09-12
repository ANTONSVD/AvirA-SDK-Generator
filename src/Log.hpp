#pragma once
#include <cstdio>
#include <string>

namespace AvirA
{
	class C_Log
	{
	public:
		static void Ok(const std::string& text)
		{
			std::printf("[+] %s\n", text.c_str());
		}

		static void Info(const std::string& text)
		{
			std::printf("[*] %s\n", text.c_str());
		}

		static void Fail(const std::string& text)
		{
			std::printf("[x] %s\n", text.c_str());
		}
	};
}
