#include "exceptions.h"
#include <format>

static std::string Visibelize(const std::string& s)
{
    std::string newStr;
    for (auto i : s)
    {
	if (i > 31 && i != 127)
	{
	    newStr += i;
	}
	else
	{
	    switch (i)
	    {
	    case '\n':
		newStr += "\\n";
		break;
	    case '\t':
		newStr += "\\t";
		break;
	    case '\r':
		newStr += "\\r";
		break;
	    case '\a':
		newStr += "\\a";
		break;
	    default:
	    {
		int x = i;
		newStr += std::format("{:03o}", x);
		break;
	    }
	    }
	}
    }
    return newStr;
}

static std::string MakeString(const std::string& expected, const std::string& got,
                              int line, const std::string& fname)
{
    auto cleanExp = Visibelize(expected);
    auto cleanGot = Visibelize(got);
    return "Expected " + cleanExp + " got " + cleanGot + " at line " +
           std::to_string(line) + " of " + fname;
}

ExpectedException::ExpectedException(const std::string& expected, const std::string& got,
                                     int line, const std::string& fname)
    : BaseException(MakeString(expected, got, line, fname))
{
}
