#include "Writer.hpp"
#include "Types.hpp"
#include "Util.hpp"
#include <cstdio>
#include <unordered_map>

namespace AvirA
{
	static std::string HexU64(std::uint64_t value)
	{
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "0x%llX", (unsigned long long)value);
		return buffer;
	}

	static std::string FullName(const C_DumpClass& klass)
	{
		std::string out;
		for (size_t i = 0; i < klass.path.size(); i++)
		{
			if (i)
				out += ".";
			out += klass.path[i];
		}
		return out;
	}

	static std::string WrapperName(const C_DumpClass& klass, const std::string& prefix)
	{
		std::string out = prefix;
		for (size_t i = 0; i < klass.path.size(); i++)
		{
			if (i)
				out += "_";
			out += C_TypeMap::Sanitize(klass.path[i]);
		}
		return out;
	}

	static std::string EnumName(const C_DumpClass& klass, const std::string& prefix)
	{
		std::string out = prefix;
		for (size_t i = 0; i < klass.path.size(); i++)
		{
			if (i)
				out += "_";
			out += C_TypeMap::Sanitize(klass.path[i]);
		}
		return out;
	}

	static std::string UnderlyingCpp(const std::string& underlying)
	{
		std::string base = C_TypeMap::BaseName(underlying);
		std::string low;
		for (char c : base)
			low += (char)tolower((unsigned char)c);
		if (low == "bool" || low == "boolean")
			return "bool";
		if (low == "byte")
			return "u8";
		if (low == "sbyte")
			return "i8";
		if (low == "short" || low == "int16")
			return "i16";
		if (low == "ushort" || low == "uint16")
			return "u16";
		if (low == "int" || low == "int32")
			return "i32";
		if (low == "uint" || low == "uint32")
			return "u32";
		if (low == "long" || low == "int64")
			return "i64";
		if (low == "ulong" || low == "uint64")
			return "u64";
		C_TypeMap map;
		C_Mapped m = map.Field(underlying);
		if (m.kind == C_TypeMap::Prim)
			return m.cpp;
		return "i32";
	}

	static bool AutoSkip(const C_DumpClass& klass)
	{
		if (klass.path.empty())
			return true;
		std::string first = klass.path[0];
		return first == "<Module>" || first == "<PrivateImplementationDetails>";
	}

	static bool KeepClass(const C_DumpClass& klass, const C_Options& options)
	{
		if (AutoSkip(klass))
			return false;
		std::string full = klass.namespaze.empty() ? FullName(klass) : klass.namespaze + "." + FullName(klass);
		for (const std::string& skip : options.skip)
		{
			if (!skip.empty() && full.find(skip) != std::string::npos)
				return false;
		}
		if (options.ns.empty())
			return true;
		for (const std::string& ns : options.ns)
		{
			if (!ns.empty() && klass.namespaze.find(ns) != std::string::npos)
				return true;
		}
		return false;
	}

	void C_Writer::WriteEnum(std::string& out, const C_DumpClass& klass, const std::string& wrap)
	{
		std::vector<std::pair<std::string, long long>> members;
		long long next = 0;
		for (const C_DumpField& field : klass.fields)
		{
			if (!field.is_const)
				continue;
			long long value = next;
			long long parsed = 0;
			if (ParseNumber(field.value, parsed))
				value = parsed;
			next = value + 1;
			std::string member = C_TypeMap::Sanitize(field.name);
			if (member == "NULL" || member == "TRUE" || member == "FALSE" || member == "min" || member == "max" || member == "near" || member == "far")
				member += "_";
			members.push_back({ member, value });
		}
		std::string underlying;
		if (!klass.underlying.empty())
		{
			underlying = UnderlyingCpp(klass.underlying);
			if (underlying == "AvirA::i8")
				underlying = "std::int8_t";
			else if (underlying == "AvirA::u8")
				underlying = "std::uint8_t";
			else if (underlying == "AvirA::i16")
				underlying = "std::int16_t";
			else if (underlying == "AvirA::u16")
				underlying = "std::uint16_t";
			else if (underlying == "AvirA::i32")
				underlying = "std::int32_t";
			else if (underlying == "AvirA::u32")
				underlying = "std::uint32_t";
			else if (underlying == "AvirA::i64")
				underlying = "std::int64_t";
			else if (underlying == "AvirA::u64")
				underlying = "std::uint64_t";
			else if (underlying != "bool" && underlying != "float" && underlying != "double")
				underlying = "std::int32_t";
		}
		else
		{
			bool big = false;
			bool huge = false;
			for (const auto& member : members)
			{
				if (member.second > 2147483647ll || member.second < -2147483648ll)
					big = true;
				if (member.second > 4294967295ll || member.second < -9223372036854775807ll - 1)
					huge = true;
			}
			underlying = huge ? "std::int64_t" : (big ? "std::uint32_t" : "std::int32_t");
		}
		out += "\tenum class " + wrap + " : " + underlying + "\n\t{\n";
		if (members.empty())
			out += "\t\t_None = 0\n\t};\n";
		else
		{
			for (size_t i = 0; i < members.size(); i++)
			{
				char buffer[64];
				snprintf(buffer, sizeof(buffer), "%lld", members[i].second);
				out += "\t\t" + members[i].first + " = " + buffer;
				out += i + 1 < members.size() ? ",\n" : "\n\t};\n";
			}
		}
	}

	static std::string UpperFirst(const std::string& text)
	{
		std::string out = text;
		for (size_t i = 0; i < out.size(); i++)
		{
			if (out[i] >= 'a' && out[i] <= 'z')
			{
				out[i] = (char)(out[i] - 'a' + 'A');
				break;
			}
			if (out[i] >= 'A' && out[i] <= 'Z')
				break;
		}
		return out;
	}

	static std::string MethodStructName(const std::string& name)
	{
		std::string s = C_TypeMap::SanitizeMember(name);
		if (s.size() > 4 && s.compare(0, 4, "get_") == 0)
			return "Get" + UpperFirst(s.substr(4));
		if (s.size() > 4 && s.compare(0, 4, "set_") == 0)
			return "Set" + UpperFirst(s.substr(4));
		return UpperFirst(s);
	}

	void C_Writer::WriteOffsets(std::string& out, const C_DumpImage& image, const C_DumpClass& klass, const std::string& wrap, const C_Options& options, size_t& fields, size_t& methods)
	{
		out += "\tstruct " + wrap + "\n\t{\n";
		std::string nested;
		for (size_t i = 0; i + 1 < klass.path.size(); i++)
		{
			if (i)
				nested += ".";
			nested += klass.path[i];
		}
		out += "\t\tstatic constexpr const char* ClassAssembly = \"" + image.name + "\";\n";
		out += "\t\tstatic constexpr const char* ClassNamespace = \"" + klass.namespaze + "\";\n";
		out += "\t\tstatic constexpr const char* ClassName = \"" + klass.path.back() + "\";\n";
		out += "\t\tstatic constexpr const char* ClassNested = \"" + nested + "\";\n";
		std::unordered_map<std::string, int> used;
		used["Static"] = 1;
		used["ClassAssembly"] = 1;
		used["ClassNamespace"] = 1;
		used["ClassName"] = 1;
		used["ClassNested"] = 1;
		auto unique = [&](const std::string& base) {
			auto it = used.find(base);
			if (it == used.end())
			{
				used[base] = 1;
				return base;
			}
			int n = ++it->second;
			return base + "_" + std::to_string(n);
		};
		bool has_static = false;
		for (const C_DumpField& field : klass.fields)
		{
			if (!field.is_const && field.is_static && field.has_offset)
				has_static = true;
		}
		if (has_static)
		{
			out += "\t\tstruct Static\n\t\t{\n";
			std::unordered_map<std::string, int> taken;
			for (const C_DumpField& field : klass.fields)
			{
				if (field.is_const || !field.is_static || !field.has_offset)
					continue;
				std::string acc = C_TypeMap::SanitizeMember(field.name);
				auto it = taken.find(acc);
				if (it == taken.end())
					taken[acc] = 1;
				else
					acc = acc + "_" + std::to_string(++it->second);
				unique(acc);
				out += "\t\t\tstatic constexpr unsigned long long " + acc + " = " + HexU64(field.offset) + ";\n";
				fields++;
			}
			out += "\t\t};\n";
		}
		for (const C_DumpField& field : klass.fields)
		{
			if (field.is_const || field.is_static || !field.has_offset)
				continue;
			std::string acc = unique(C_TypeMap::SanitizeMember(field.name));
			out += "\t\tstatic constexpr unsigned long long " + acc + " = " + HexU64(field.offset) + ";\n";
			fields++;
		}
		for (const C_DumpMethod& method : klass.methods)
		{
			std::string acc = unique(MethodStructName(method.name));
			out += "\t\tstruct " + acc + "\n\t\t{\n";
			out += "\t\t\tstatic constexpr const char* Name = \"" + method.resolve + "\";\n";
			out += "\t\t\tstatic constexpr int Args = " + std::to_string(method.params.size()) + ";\n";
			out += "\t\t};\n";
			methods++;
		}
		out += "\t};\n";
	}


	bool C_Writer::WriteImage(const C_DumpImage& image, const std::string& dir, const C_Options& options, size_t& classes, size_t& fields, size_t& methods, size_t& enums)
	{
		std::unordered_map<size_t, std::string> names;
		std::unordered_map<std::string, int> taken;
		for (size_t i = 0; i < image.classes.size(); i++)
		{
			const C_DumpClass& klass = image.classes[i];
			if (!KeepClass(klass, options))
				continue;
			std::string base = klass.is_enum ? EnumName(klass, options.enum_prefix) : WrapperName(klass, options.prefix);
			std::string key = klass.namespaze + "|" + base;
			auto it = taken.find(key);
			if (it == taken.end())
			{
				taken[key] = 1;
				names[i] = base;
			}
			else
				names[i] = base + "_" + std::to_string(++it->second);
		}
		C_TypeMap map;
		for (const auto& pair : names)
		{
			const C_DumpClass& klass = image.classes[pair.first];
			if (!klass.is_enum)
				continue;
			map.AddEnum((klass.namespaze.empty() ? "" : klass.namespaze + ".") + FullName(klass), pair.second);
		}
		std::string body = "#pragma once\n#include <cstdint>\n";
		std::string file = C_TypeMap::SanitizeFile(image.name);
		std::string name = file;
		if (name.empty())
			name = "Unknown";
		std::string last_ns;
		for (size_t i = 0; i < image.classes.size(); i++)
		{
			auto found = names.find(i);
			if (found == names.end())
				continue;
			const C_DumpClass& klass = image.classes[i];
			std::string ns = C_TypeMap::SanitizeNamespace(klass.namespaze);
			if (ns != last_ns)
			{
				if (!last_ns.empty())
					body += "}\n";
				body += "namespace " + ns + "\n{\n";
				last_ns = ns;
			}
			if (klass.is_enum)
			{
				WriteEnum(body, klass, found->second);
				enums++;
			}
			else
			{
				WriteOffsets(body, image, klass, found->second, options, fields, methods);
				classes++;
			}
		}
		if (!last_ns.empty())
			body += "}\n";
		std::string path = dir + "/Sdk_" + name + ".hpp";
		FILE* f = nullptr;
		if (fopen_s(&f, path.c_str(), "wb") != 0 || !f)
			return false;
		size_t put = std::fwrite(body.data(), 1, body.size(), f);
		std::fclose(f);
		return put == body.size();
	}

	bool C_Writer::Write(const C_Dump& dump, const C_Options& options, size_t& classes, size_t& fields, size_t& methods, size_t& enums)
	{
		m_options = options;
		classes = 0;
		fields = 0;
		methods = 0;
		enums = 0;
		std::string umbrella = "#pragma once\n#include \"Resolver.hpp\"\n#include \"Unity/Math.hpp\"\n";
		for (const C_DumpImage& image : dump.Images())
		{
			bool keep = options.images.empty();
			for (const std::string& pick : options.images)
			{
				if (!pick.empty() && image.name.find(pick) != std::string::npos)
					keep = true;
			}
			if (!keep)
				continue;
			std::string name = C_TypeMap::SanitizeFile(image.name);
			if (name.empty())
				name = "Unknown";
			if (!WriteImage(image, options.out, options, classes, fields, methods, enums))
				return false;
			umbrella += "#include \"Sdk_" + name + ".hpp\"\n";
		}
		std::string path = options.out + "/Sdk.hpp";
		FILE* f = nullptr;
		if (fopen_s(&f, path.c_str(), "wb") != 0 || !f)
			return false;
		size_t put = std::fwrite(umbrella.data(), 1, umbrella.size(), f);
		std::fclose(f);
		return put == umbrella.size();
	}
}
