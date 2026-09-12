#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace AvirA
{
	class C_DumpParam
	{
	public:
		std::string type;
		std::string name;
	};

	class C_DumpField
	{
	public:
		std::string type;
		std::string name;
		std::uint64_t offset = 0;
		bool has_offset = false;
		bool is_static = false;
		bool is_const = false;
		std::string value;
	};

	class C_DumpMethod
	{
	public:
		std::string ret;
		std::string name;
		std::string resolve;
		std::vector<C_DumpParam> params;
		bool is_static = false;
	};

	class C_DumpClass
	{
	public:
		std::string namespaze;
		std::string name;
		std::vector<std::string> path;
		std::vector<std::string> path_raw;
		bool is_enum = false;
		bool is_struct = false;
		bool is_generic = false;
		std::string underlying;
		std::vector<C_DumpField> fields;
		std::vector<C_DumpMethod> methods;
	};

	class C_DumpImage
	{
	public:
		std::string name;
		int start = -1;
		std::vector<C_DumpClass> classes;
	};

	class C_Dump
	{
	public:
		bool Load(const std::string& path);
		const std::vector<C_DumpImage>& Images() const;
		size_t ClassCount() const;

	private:
		int FindImage(const std::string& name);
		void ParseHead(const std::string& line);
		void ParseField(const std::string& line);
		void ParseMethod(const std::string& line);

		std::vector<C_DumpImage> m_images;
		int m_image = -1;
		std::string m_namespaze;
		C_DumpClass* m_class = nullptr;
		int m_section = 0;
		int m_depth = 0;
		bool m_in_class = false;
		bool m_in_block = false;
	};
}
