#include "Dump.hpp"
#include "Writer.hpp"
#include "Log.hpp"
#include <Windows.h>

namespace AvirA
{
	static void Usage()
	{
		C_Log::Info("AvirA SDK Generator");
		C_Log::Info("Usage: AvirASdkGen.exe <dump.cs> [out_dir] [options]");
		C_Log::Info("Without out_dir the SDK goes to Sdk folder next to this exe.");
		C_Log::Info("You can also drag and drop dump.cs onto the exe.");
		C_Log::Info("Options:");
		C_Log::Info("  --image <part>       only images with this part in name, repeatable");
		C_Log::Info("  --ns <part>          only classes with this part in namespace, repeatable");
		C_Log::Info("  --skip <part>        skip classes with this part in full name, repeatable");
		C_Log::Info("  --prefix <text>      class prefix, default C_");
		C_Log::Info("  --enumprefix <text>  enum prefix, default E_");
	}

	static std::string ExeDir()
	{
		char path[MAX_PATH];
		DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
		std::string exe = length ? std::string(path, length) : std::string();
		size_t slash = exe.find_last_of("\\/");
		if (slash != std::string::npos)
			return exe.substr(0, slash);
		return std::string();
	}

	static int Run(int count, char** args)
	{
		if (count < 2)
		{
			Usage();
			return 1;
		}
		C_Options options;
		options.dump = args[1];
		if (count > 2 && args[2][0] != '-')
			options.out = args[2];
		else
			options.out = ExeDir() + "\\Sdk";
		int first = (count > 2 && args[2][0] != '-') ? 3 : 2;
		for (int i = first; i < count; i++)
		{
			std::string key = args[i];
			if ((key == "--image" || key == "--ns" || key == "--skip" || key == "--prefix" || key == "--enumprefix") && i + 1 < count)
			{
				std::string value = args[++i];
				if (key == "--image")
					options.images.push_back(value);
				else if (key == "--ns")
					options.ns.push_back(value);
				else if (key == "--skip")
					options.skip.push_back(value);
				else if (key == "--prefix")
					options.prefix = value;
				else
					options.enum_prefix = value;
			}
			else
			{
				Usage();
				return 1;
			}
		}
		if (options.prefix.empty())
			options.prefix = "C_";
		if (options.enum_prefix.empty())
			options.enum_prefix = "E_";
		CreateDirectoryA(options.out.c_str(), nullptr);
		unsigned long long start = GetTickCount64();
		C_Dump dump;
		C_Log::Info("Loading " + options.dump);
		if (!dump.Load(options.dump))
		{
			C_Log::Fail("Failed to load dump");
			return 1;
		}
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "Parsed %llu images, %llu classes", (unsigned long long)dump.Images().size(), (unsigned long long)dump.ClassCount());
		C_Log::Info(buffer);
		C_Writer writer;
		size_t classes = 0;
		size_t fields = 0;
		size_t methods = 0;
		size_t enums = 0;
		C_Log::Info("Writing SDK to " + options.out);
		if (!writer.Write(dump, options, classes, fields, methods, enums))
		{
			C_Log::Fail("Failed to write SDK");
			return 1;
		}
		snprintf(buffer, sizeof(buffer), "Done in %llums: %llu classes, %llu enums, %llu fields, %llu methods",
			(unsigned long long)(GetTickCount64() - start),
			(unsigned long long)classes, (unsigned long long)enums,
			(unsigned long long)fields, (unsigned long long)methods);
		C_Log::Ok(buffer);
		return 0;
	}
}

int main(int count, char** args)
{
	return AvirA::Run(count, args);
}
