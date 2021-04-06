#pragma once
#include <algorithm>
#include <clocale>
#include <cwctype>
#include <numeric>
#include <set>
#include <string>
#include <vector>

inline bool ends_with(const std::wstring& wstr, const std::wstring& wsub)
{
	if (wstr.size() < wsub.size())
		return false;
	else
		return std::equal(wsub.rbegin(), wsub.rend(), wstr.rbegin());
}

//该函数多线程下会有莫名其妙的越界问题，增加(str1.size() == str2.size())暂时缓解
//但问题应该还存在，模板化的东东实在时很哪个啥...
inline bool iequals(const std::wstring& str1, const std::wstring& str2)
{
	//长度不相等既可判断字符串不相同，无需执行后续std::equal
	return (str1.size() == str2.size()) && std::equal(str1.begin(), str1.end(), str2.begin(), [](const wchar_t& wc1, const wchar_t& wc2)
	{
		return std::towlower(wc1) == std::towlower(wc2);
	});
}

inline void ireplace_last(std::wstring& input, const std::wstring& search, const std::wstring& sub)
{
	std::size_t pos = input.rfind(search);
	if (pos != std::wstring::npos)
		input.replace(pos, search.length(), sub);
}

inline std::string join(const std::set<std::string>& list, const std::string& delim)
{
	return std::accumulate(list.begin(), list.end(), std::string(), [&delim](std::string& str1, const std::string& str2)
	{
		return str1.empty() ? str2 : str1 + delim + str2;
	});
}

inline std::vector<std::wstring>& split(std::vector<std::wstring>& result, const std::wstring& input, const wchar_t* delim)
{
	result.clear();
	size_t current = 0;
	size_t next = std::wstring::npos;
	do
	{
		current = next + 1;
		next = input.find_first_of(delim, next + 1);
		result.push_back(input.substr(current, next - current));
	} while (next != std::wstring::npos);
	return result;
}

inline bool starts_with(const std::wstring& wstr, const std::wstring& wsub)
{
	if (wstr.size() < wsub.size())
		return false;
	else
		return std::equal(wsub.begin(), wsub.end(), wstr.begin());
}

inline void to_lower(std::wstring& wstr)
{
	std::setlocale(LC_ALL, "");
	std::transform(wstr.begin(), wstr.end(), wstr.begin(), std::towlower);
}
