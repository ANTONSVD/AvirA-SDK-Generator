#include "Types.hpp"
#include "Util.hpp"

namespace AvirA
{
	void C_TypeMap::Clear()
	{
		m_enums.clear();
		m_generic = false;
	}

	void C_TypeMap::AddEnum(const std::string& name, const std::string& cpp)
	{
		m_enums[BaseName(name)] = cpp;
	}

	void C_TypeMap::SetGeneric(bool generic)
	{
		m_generic = generic;
	}

	C_Mapped C_TypeMap::Field(const std::string& type) const
	{
		return Map(type, false);
	}

	C_Mapped C_TypeMap::Param(const std::string& type) const
	{
		return Map(type, true);
	}

	C_Mapped C_TypeMap::Ret(const std::string& type) const
	{
		return Map(type, false);
	}

	static std::string Primitive(const std::string& type)
	{
		if (type == "bool")
			return "bool";
		if (type == "byte")
			return "u8";
		if (type == "sbyte")
			return "i8";
		if (type == "short")
			return "i16";
		if (type == "ushort")
			return "u16";
		if (type == "int")
			return "i32";
		if (type == "uint")
			return "u32";
		if (type == "long")
			return "i64";
		if (type == "ulong")
			return "u64";
		if (type == "float")
			return "float";
		if (type == "double")
			return "double";
		if (type == "char")
			return "u16";
		if (type == "nint")
			return "i64";
		if (type == "nuint")
			return "u64";
		if (type == "void")
			return "void";
		return std::string();
	}

	static std::string UnityStruct(const std::string& type)
	{
		if (type == "UnityEngine.Vector2")
			return "Vector2";
		if (type == "UnityEngine.Vector3")
			return "Vector3";
		if (type == "UnityEngine.Vector4")
			return "Vector4";
		if (type == "UnityEngine.Quaternion")
			return "Quaternion";
		if (type == "UnityEngine.Color")
			return "Color";
		if (type == "UnityEngine.Rect")
			return "Rect";
		if (type == "UnityEngine.Bounds")
			return "Bounds";
		if (type == "UnityEngine.Ray")
			return "Ray";
		if (type == "UnityEngine.Matrix4x4")
			return "Matrix4x4";
		return std::string();
	}

	C_Mapped C_TypeMap::Map(const std::string& type, bool param) const
	{
		C_Mapped out;
		std::string t = Trim(type);
		if (!t.empty() && t.back() == '?')
			t = Trim(t.substr(0, t.size() - 1));
		if (t.size() > 2 && EndsWith(t, "[]"))
		{
			C_Mapped elem = Map(t.substr(0, t.size() - 2), param);
			out.kind = Array;
			if (elem.kind == Prim || elem.kind == Enum || elem.kind == Struct)
			{
				out.elem = elem.cpp;
				out.cpp = "C_Array<" + elem.cpp + ">";
			}
			else
			{
				out.elem = "C_Object*";
				out.cpp = "C_Array<C_Object*>";
			}
			return out;
		}
		std::string prim = Primitive(t);
		if (!prim.empty())
		{
			out.cpp = prim;
			out.kind = prim == "void" ? Void : Prim;
			return out;
		}
		if (t == "string")
		{
			out.cpp = "C_String";
			out.kind = String;
			return out;
		}
		if (t == "object")
		{
			out.cpp = "C_Object*";
			out.kind = Object;
			return out;
		}
		std::string uni = UnityStruct(t);
		if (!uni.empty())
		{
			out.cpp = uni;
			out.kind = Struct;
			return out;
		}
		auto found = m_enums.find(BaseName(t));
		if (found != m_enums.end())
		{
			out.cpp = found->second;
			out.kind = Enum;
			return out;
		}
		if (m_generic && !t.empty() && t[0] == 'T')
		{
			bool generic_param = t.size() == 1 || (t[1] >= 'A' && t[1] <= 'Z');
			if (generic_param)
			{
				for (size_t i = 1; generic_param && i < t.size(); i++)
				{
					char c = t[i];
					bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
					if (!ok)
						generic_param = false;
				}
			}
			if (generic_param)
			{
				out.cpp = "void*";
				out.kind = Object;
				return out;
			}
		}
		(void)param;
		out.cpp = "C_Object*";
		out.kind = Object;
		return out;
	}

	std::string C_TypeMap::BaseName(const std::string& text)
	{
		std::string t = Trim(text);
		size_t lt = t.find('<');
		if (lt != std::string::npos)
			t = t.substr(0, lt);
		size_t dot = t.rfind('.');
		if (dot != std::string::npos)
			t = t.substr(dot + 1);
		size_t tick = t.find('`');
		if (tick != std::string::npos)
			t = t.substr(0, tick);
		return Trim(t);
	}

	bool C_TypeMap::IsKeyword(const std::string& text)
	{
		static const char* keys[] = {
			"alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
			"bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t",
			"class", "compl", "concept", "const", "consteval", "constexpr", "const_cast",
			"continue", "co_await", "co_return", "co_yield", "decltype", "default",
			"delete", "do", "double", "dynamic_cast", "else", "enum", "explicit",
			"export", "extern", "false", "float", "for", "friend", "goto", "if",
			"inline", "int", "long", "mutable", "namespace", "new", "noexcept",
			"not", "not_eq", "nullptr", "operator", "or", "or_eq", "private",
			"protected", "public", "reflexpr", "register", "reinterpret_cast",
			"requires", "return", "short", "signed", "sizeof", "static",
			"static_assert", "static_cast", "struct", "switch", "template", "this",
			"thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
			"union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t",
			"while", "xor", "xor_eq", nullptr
		};
		for (int i = 0; keys[i]; i++)
		{
			if (text == keys[i])
				return true;
		}
		return false;
	}

	std::string C_TypeMap::Sanitize(const std::string& text)
	{
		std::string s = Trim(text);
		if (!s.empty() && s[0] == '@')
			s = s.substr(1);
		std::string tmp;
		for (size_t i = 0; i < s.size(); i++)
		{
			if (s.compare(i, 17, "k__BackingField") == 0)
			{
				tmp += "BackingField";
				i += 16;
			}
			else if (s.compare(i, 3, "k__") == 0)
				i += 2;
			else
				tmp += s[i];
		}
		std::string out;
		for (char c : tmp)
		{
			bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
			out += ok ? c : '_';
		}
		if (out.empty())
			out = "_";
		if (out[0] >= '0' && out[0] <= '9')
			out = "_" + out;
		if (IsKeyword(out))
			out += "_";
		return out;
	}

	std::string C_TypeMap::SanitizeMember(const std::string& text)
	{
		std::string out = Sanitize(text);
		static const char* reserved[] = {
			"Get", "Set", "GetPtr", "GetObj", "GetAt", "SetAt", "Call", "Class",
			"Field", "Size", "Virtual", "Unbox", "Valid", "Raw", "Api",
			"GetObscured", "SetObscured", "ResolveClass", "C_Object", nullptr
		};
		for (int i = 0; reserved[i]; i++)
		{
			if (out == reserved[i])
			{
				out += "_";
				break;
			}
		}
		return out;
	}

	std::string C_TypeMap::SanitizeNamespace(const std::string& text)
	{
		std::string s = Trim(text);
		if (s.empty())
			return "Global";
		std::string out;
		for (std::string part : SplitTop(s, '.'))
		{
			part = Sanitize(part);
			if (!out.empty())
				out += "::";
			out += part;
		}
		return out;
	}

	std::string C_TypeMap::SanitizeFile(const std::string& text)
	{
		std::string s = Trim(text);
		size_t dot = s.rfind('.');
		if (dot != std::string::npos)
			s = s.substr(0, dot);
		return Sanitize(s);
	}
}
