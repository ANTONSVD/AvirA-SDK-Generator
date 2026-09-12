#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace AvirA
{
	class C_Mapped
	{
	public:
		std::string cpp;
		int kind = 0;
		std::string elem;
	};

	class C_TypeMap
	{
	public:
		static const int Prim = 0;
		static const int Object = 1;
		static const int String = 2;
		static const int Array = 3;
		static const int Enum = 4;
		static const int Struct = 5;
		static const int Void = 6;

		void Clear();
		void AddEnum(const std::string& name, const std::string& cpp);
		void SetGeneric(bool generic);

		C_Mapped Field(const std::string& type) const;
		C_Mapped Param(const std::string& type) const;
		C_Mapped Ret(const std::string& type) const;

		static std::string Sanitize(const std::string& text);
		static std::string SanitizeMember(const std::string& text);
		static std::string SanitizeNamespace(const std::string& text);
		static std::string SanitizeFile(const std::string& text);
		static std::string BaseName(const std::string& text);
		static bool IsKeyword(const std::string& text);

	private:
		C_Mapped Map(const std::string& type, bool param) const;

		std::unordered_map<std::string, std::string> m_enums;
		bool m_generic = false;
	};
}
