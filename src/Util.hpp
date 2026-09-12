#pragma once
#include <string>
#include <vector>

namespace AvirA
{
	inline std::string Trim(const std::string& text)
	{
		size_t first = text.find_first_not_of(" \t\r\n");
		if (first == std::string::npos)
			return std::string();
		size_t last = text.find_last_not_of(" \t\r\n");
		return text.substr(first, last - first + 1);
	}

	inline bool StartsWith(const std::string& text, const std::string& prefix)
	{
		return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
	}

	inline bool EndsWith(const std::string& text, const std::string& suffix)
	{
		return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
	}

	inline std::vector<std::string> SplitTop(const std::string& text, char sep)
	{
		std::vector<std::string> out;
		int angle = 0;
		int paren = 0;
		int square = 0;
		bool quote = false;
		size_t start = 0;
		for (size_t i = 0; i < text.size(); i++)
		{
			char c = text[i];
			if (quote)
			{
				if (c == '"')
					quote = false;
				continue;
			}
			if (c == '"')
				quote = true;
			else if (c == '<')
				angle++;
			else if (c == '>')
			{
				if (angle > 0)
					angle--;
			}
			else if (c == '(')
				paren++;
			else if (c == ')')
			{
				if (paren > 0)
					paren--;
			}
			else if (c == '[')
				square++;
			else if (c == ']')
			{
				if (square > 0)
					square--;
			}
			else if (c == sep && !angle && !paren && !square)
			{
				out.push_back(text.substr(start, i - start));
				start = i + 1;
			}
		}
		out.push_back(text.substr(start));
		return out;
	}

	inline std::string StripInlineComments(const std::string& text)
	{
		std::string out;
		out.reserve(text.size());
		bool quote = false;
		for (size_t i = 0; i < text.size(); i++)
		{
			char c = text[i];
			if (c == '"')
				quote = !quote;
			if (!quote && c == '/' && i + 1 < text.size() && text[i + 1] == '*')
			{
				size_t end = text.find("*/", i + 2);
				if (end == std::string::npos)
					break;
				i = end + 1;
				continue;
			}
			out += c;
		}
		return out;
	}

	inline bool ParseNumber(const std::string& text, long long& value)
	{
		std::string s = Trim(text);
		if (s.empty())
			return false;
		bool negative = false;
		if (s[0] == '-')
		{
			negative = true;
			s = Trim(s.substr(1));
		}
		else if (s[0] == '+')
			s = Trim(s.substr(1));
		int base = 10;
		if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
		{
			base = 16;
			s = s.substr(2);
		}
		if (s.empty())
			return false;
		unsigned long long acc = 0;
		for (char c : s)
		{
			int digit = -1;
			if (c >= '0' && c <= '9')
				digit = c - '0';
			else if (base == 16 && c >= 'a' && c <= 'f')
				digit = c - 'a' + 10;
			else if (base == 16 && c >= 'A' && c <= 'F')
				digit = c - 'A' + 10;
			else if (c == '_' || c == '\'')
				continue;
			else
				return false;
			if (digit >= base)
				return false;
			acc = acc * (unsigned long long)base + (unsigned long long)digit;
		}
		value = negative ? -(long long)acc : (long long)acc;
		return true;
	}

	inline bool ParseHex(const std::string& text, unsigned long long& value)
	{
		long long number = 0;
		if (!ParseNumber(text, number) || number < 0)
			return false;
		value = (unsigned long long)number;
		return true;
	}
}
