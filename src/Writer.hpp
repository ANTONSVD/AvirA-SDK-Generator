#pragma once
#include "Dump.hpp"
#include <string>
#include <vector>

namespace AvirA
{
	class C_Options
	{
	public:
		std::string dump;
		std::string out;
		std::vector<std::string> images;
		std::vector<std::string> ns;
		std::vector<std::string> skip;
		std::string prefix = "C_";
		std::string enum_prefix = "E_";
	};

	class C_Writer
	{
	public:
		bool Write(const C_Dump& dump, const C_Options& options, size_t& classes, size_t& fields, size_t& methods, size_t& enums);

	private:
		bool WriteImage(const C_DumpImage& image, const std::string& dir, const C_Options& options, size_t& classes, size_t& fields, size_t& methods, size_t& enums);
		void WriteEnum(std::string& out, const C_DumpClass& klass, const std::string& wrap);
		void WriteOffsets(std::string& out, const C_DumpImage& image, const C_DumpClass& klass, const std::string& wrap, const C_Options& options, size_t& fields, size_t& methods);

		C_Options m_options;
	};
}
