#include "Dump.hpp"
#include "Util.hpp"
#include <cstdio>

namespace AvirA
{
	const std::vector<C_DumpImage>& C_Dump::Images() const
	{
		return m_images;
	}

	size_t C_Dump::ClassCount() const
	{
		size_t count = 0;
		for (const C_DumpImage& image : m_images)
			count += image.classes.size();
		return count;
	}

	int C_Dump::FindImage(const std::string& name)
	{
		for (size_t i = 0; i < m_images.size(); i++)
		{
			if (m_images[i].name == name)
				return (int)i;
		}
		C_DumpImage image;
		image.name = name;
		m_images.push_back(image);
		return (int)m_images.size() - 1;
	}

	static std::string StripGenerics(const std::string& text)
	{
		std::string out;
		int angle = 0;
		for (char c : text)
		{
			if (c == '<')
				angle++;
			else if (c == '>')
			{
				if (angle > 0)
					angle--;
			}
			else if (!angle)
				out += c;
		}
		return Trim(out);
	}

	static int GenericArity(const std::string& text)
	{
		size_t open = text.find('<');
		size_t close = text.rfind('>');
		if (open == std::string::npos || close == std::string::npos || close <= open)
			return 0;
		std::string inner = text.substr(open + 1, close - open - 1);
		return (int)SplitTop(inner, ',').size();
	}

	static bool IsModifier(const std::string& token)
	{
		static const char* mods[] = {
			"public", "private", "protected", "internal", "static", "readonly",
			"volatile", "const", "sealed", "extern", "abstract", "virtual",
			"override", "new", "unsafe", "async", "ref", "in", "out", nullptr
		};
		for (int i = 0; mods[i]; i++)
		{
			if (token == mods[i])
				return true;
		}
		return false;
	}

	void C_Dump::ParseHead(const std::string& line)
	{
		size_t marker = line.find("// TypeDefIndex:");
		if (marker == std::string::npos)
			return;
		long long typedef_index = -1;
		ParseNumber(Trim(line.substr(marker + 17)), typedef_index);
		if (typedef_index >= 0)
		{
			int best = -1;
			for (size_t i = 0; i < m_images.size(); i++)
			{
				if (m_images[i].start >= 0 && m_images[i].start <= typedef_index)
					best = (int)i;
			}
			if (best >= 0)
				m_image = best;
		}
		std::string head = Trim(line.substr(0, marker));
		const char* keys[] = { " class ", " struct ", " enum ", " interface " };
		int kind = -1;
		size_t at = std::string::npos;
		for (int i = 0; i < 4; i++)
		{
			size_t found = head.find(keys[i]);
			if (found != std::string::npos && (at == std::string::npos || found < at))
			{
				at = found;
				kind = i;
			}
			if (StartsWith(head, keys[i] + 1))
			{
				at = 0;
				kind = i;
			}
		}
		if (kind < 0)
			return;
		size_t name_at = at ? at + 7 : head.find(' ') + 1;
		std::string rest = Trim(head.substr(name_at));
		size_t end = rest.find(" :");
		if (end == std::string::npos)
			end = rest.find(':');
		std::string decl = Trim(end == std::string::npos ? rest : rest.substr(0, end));
		std::string parent = end == std::string::npos ? std::string() : Trim(rest.substr(end + 1));
		if (m_image < 0)
			m_image = FindImage("Unknown");
		m_images[(size_t)m_image].classes.push_back(C_DumpClass());
		C_DumpClass& klass = m_images[(size_t)m_image].classes.back();
		klass.namespaze = m_namespaze;
		klass.name = StripGenerics(decl);
		klass.is_enum = kind == 2;
		klass.is_struct = kind == 1;
		for (std::string part : SplitTop(decl, '.'))
		{
			part = Trim(part);
			if (part.empty())
				continue;
			int arity = GenericArity(part);
			std::string simple = StripGenerics(part);
			klass.path.push_back(simple);
			klass.path_raw.push_back(arity > 0 ? simple + "`" + std::to_string(arity) : simple);
			if (arity > 0)
				klass.is_generic = true;
		}
		if (klass.path.empty())
		{
			m_images[(size_t)m_image].classes.pop_back();
			return;
		}
		if (klass.is_enum && !parent.empty())
		{
			std::string base = StripGenerics(parent);
			size_t dot = base.rfind('.');
			if (dot != std::string::npos)
				base = base.substr(dot + 1);
			if (!base.empty())
				klass.underlying = base;
		}
		m_class = &m_images[(size_t)m_image].classes.back();
		m_section = 0;
		m_in_class = true;
		m_depth = 0;
		size_t body = line.find('{', marker);
		if (body != std::string::npos)
		{
			size_t close = line.find('}', body);
			if (close != std::string::npos)
			{
				m_in_class = false;
				m_class = nullptr;
			}
			else
				m_depth = 1;
		}
		else
			m_depth = 0;
	}

	void C_Dump::ParseField(const std::string& line)
	{
		if (!m_class)
			return;
		std::string code = line;
		std::string comment;
		size_t mark = line.find("//");
		if (mark != std::string::npos)
		{
			code = line.substr(0, mark);
			comment = Trim(line.substr(mark + 2));
		}
		code = Trim(StripInlineComments(code));
		if (code.empty() || code.back() != ';')
			return;
		code.resize(code.size() - 1);
		std::string value;
		size_t eq = std::string::npos;
		{
			int angle = 0;
			bool quote = false;
			for (size_t i = 0; i < code.size(); i++)
			{
				char c = code[i];
				if (c == '"')
					quote = !quote;
				else if (!quote && c == '<')
					angle++;
				else if (!quote && c == '>')
				{
					if (angle > 0)
						angle--;
				}
				else if (!quote && !angle && c == '=')
				{
					eq = i;
					break;
				}
			}
		}
		if (eq != std::string::npos)
		{
			value = Trim(StripInlineComments(code.substr(eq + 1)));
			code = Trim(code.substr(0, eq));
		}
		std::vector<std::string> parts;
		for (std::string token : SplitTop(code, ' '))
		{
			token = Trim(token);
			if (!token.empty())
				parts.push_back(token);
		}
		size_t i = 0;
		bool is_static = false;
		bool is_const = false;
		while (i < parts.size() && IsModifier(parts[i]))
		{
			if (parts[i] == "static")
				is_static = true;
			if (parts[i] == "const")
				is_const = true;
			i++;
		}
		if (parts.size() - i < 2)
			return;
		std::string name = parts.back();
		std::string type;
		for (size_t k = i; k + 1 < parts.size(); k++)
		{
			if (!type.empty())
				type += " ";
			type += parts[k];
		}
		C_DumpField field;
		field.type = type;
		field.name = name;
		field.is_static = is_static;
		field.is_const = is_const;
		field.value = value;
		if (!is_const && !comment.empty())
		{
			unsigned long long offset = 0;
			if (ParseHex(comment, offset))
			{
				field.offset = (std::uint64_t)offset;
				field.has_offset = true;
			}
		}
		m_class->fields.push_back(field);
	}

	void C_Dump::ParseMethod(const std::string& line)
	{
		if (!m_class)
			return;
		std::string code = Trim(line);
		size_t open = code.find('(');
		size_t close = code.rfind(')');
		if (open == std::string::npos || close == std::string::npos || close <= open)
			return;
		std::string head = Trim(code.substr(0, open));
		std::string arglist = code.substr(open + 1, close - open - 1);
		std::vector<std::string> parts;
		for (std::string token : SplitTop(head, ' '))
		{
			token = Trim(token);
			if (!token.empty())
				parts.push_back(token);
		}
		size_t i = 0;
		bool is_static = false;
		while (i < parts.size() && IsModifier(parts[i]))
		{
			if (parts[i] == "static")
				is_static = true;
			i++;
		}
		if (parts.size() - i < 2)
			return;
		std::string name = parts.back();
		if (name == ".ctor" || name == ".cctor")
			return;
		std::string resolve = name;
		{
			int arity = GenericArity(name);
			resolve = StripGenerics(name);
			if (arity > 0)
				resolve += "`" + std::to_string(arity);
			name = resolve;
			size_t tick = name.find('`');
			if (tick != std::string::npos)
				name = name.substr(0, tick);
		}
		std::string ret;
		for (size_t k = i; k + 1 < parts.size(); k++)
		{
			if (!ret.empty())
				ret += " ";
			ret += parts[k];
		}
		if (StartsWith(ret, "ref "))
			ret = Trim(ret.substr(4));
		C_DumpMethod method;
		method.ret = ret.empty() ? "void" : ret;
		method.name = name;
		method.resolve = resolve;
		method.is_static = is_static;
		for (std::string item : SplitTop(arglist, ','))
		{
			item = Trim(StripInlineComments(item));
			if (item.empty())
				continue;
			size_t eq = std::string::npos;
			{
				int angle = 0;
				bool quote = false;
				for (size_t k = 0; k < item.size(); k++)
				{
					char c = item[k];
					if (c == '"')
						quote = !quote;
					else if (!quote && c == '<')
						angle++;
					else if (!quote && c == '>')
					{
						if (angle > 0)
							angle--;
					}
					else if (!quote && !angle && c == '=')
					{
						eq = k;
						break;
					}
				}
			}
			if (eq != std::string::npos)
				item = Trim(item.substr(0, eq));
			std::vector<std::string> tokens;
			for (std::string token : SplitTop(item, ' '))
			{
				token = Trim(token);
				if (!token.empty())
					tokens.push_back(token);
			}
			size_t t = 0;
			while (t < tokens.size() && (tokens[t] == "ref" || tokens[t] == "out" || tokens[t] == "in" || tokens[t] == "[In]" || tokens[t] == "[Out]"))
				t++;
			if (tokens.size() - t < 2)
				continue;
			C_DumpParam param;
			param.name = tokens.back();
			for (size_t k = t; k + 1 < tokens.size(); k++)
			{
				if (!param.type.empty())
					param.type += " ";
				param.type += tokens[k];
			}
			method.params.push_back(param);
		}
		m_class->methods.push_back(method);
	}

	bool C_Dump::Load(const std::string& path)
	{
		m_images.clear();
		m_image = -1;
		m_class = nullptr;
		m_section = 0;
		m_depth = 0;
		m_in_class = false;
		m_in_block = false;
		m_namespaze.clear();
		FILE* file = nullptr;
		if (fopen_s(&file, path.c_str(), "rb") != 0 || !file)
			return false;
		std::fseek(file, 0, SEEK_END);
		long size = std::ftell(file);
		std::fseek(file, 0, SEEK_SET);
		if (size <= 0)
		{
			std::fclose(file);
			return false;
		}
		std::string data;
		data.resize((size_t)size);
		size_t got = std::fread(data.data(), 1, data.size(), file);
		std::fclose(file);
		if (got != data.size())
			return false;
		size_t pos = 0;
		std::string line;
		line.reserve(1024);
		while (pos <= data.size())
		{
			line.clear();
			while (pos < data.size() && data[pos] != '\n')
			{
				if (data[pos] != '\r')
					line += data[pos];
				pos++;
			}
			pos++;
			std::string s = Trim(line);
			if (s.empty())
				continue;
			if (m_in_block)
			{
				if (s.find("*/") != std::string::npos)
					m_in_block = false;
				continue;
			}
			if (StartsWith(s, "/*"))
			{
				if (s.find("*/", 2) == std::string::npos)
					m_in_block = true;
				continue;
			}
			if (StartsWith(s, "// Image"))
			{
				size_t colon = s.find(':');
				if (colon != std::string::npos)
				{
					std::string rest = Trim(s.substr(colon + 1));
					size_t dash = rest.rfind(" - ");
					std::string name = Trim(dash == std::string::npos ? rest : rest.substr(0, dash));
					if (!name.empty())
					{
						m_image = FindImage(name);
						if (dash != std::string::npos)
						{
							long long start = -1;
							if (ParseNumber(Trim(rest.substr(dash + 3)), start) && start >= 0)
								m_images[(size_t)m_image].start = (int)start;
						}
					}
				}
				m_in_class = false;
				m_class = nullptr;
				continue;
			}
			if (StartsWith(s, "// Namespace:"))
			{
				m_namespaze = s.size() > 14 ? Trim(s.substr(14)) : std::string();
				continue;
			}
			if (!m_in_class)
			{
				if (s.find("// TypeDefIndex:") != std::string::npos)
					ParseHead(s);
				continue;
			}
			if (s == "// Fields")
			{
				m_section = 1;
				continue;
			}
			if (s == "// Properties")
			{
				m_section = 2;
				continue;
			}
			if (s == "// Methods")
			{
				m_section = 3;
				continue;
			}
			if (!s.empty() && s[0] == '[')
				continue;
			if (StartsWith(s, "//"))
				continue;
			if (m_section == 1)
				ParseField(s);
			else if (m_section == 3)
				ParseMethod(s);
			int open = 0;
			int close = 0;
			{
				bool quote = false;
				for (char c : s)
				{
					if (c == '"')
						quote = !quote;
					else if (!quote && c == '{')
						open++;
					else if (!quote && c == '}')
						close++;
				}
			}
			m_depth += open - close;
			if (m_depth <= 0)
			{
				m_in_class = false;
				m_class = nullptr;
				m_section = 0;
			}
		}
		return true;
	}
}
