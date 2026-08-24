#include <cassert>
#include <cctype>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "exceptions.h"
#include "file.h"

class CSVEntry
{
public:
    enum Type
    {
	Unknown,
	String,
	Number,
    };
    CSVEntry() : m_ty{ Unknown } {}
    CSVEntry(int i) : m_ty(Number) { m_content.i = i; }
    CSVEntry(const std::string& s) : m_ty{ String }
    {
	m_content.s = s;
	m_content.i = 0xdeadbeef;
    }

    int         AsNumber();
    std::string AsString();

private:
    Type m_ty;
    struct
    {
	int         i;
	std::string s;
    } m_content;
};

class CSVList
{
public:
    CSVList(int num) : m_num{ num } {}
    void AddEntry(const CSVEntry& e);

private:
    int                   m_num;
    std::vector<CSVEntry> m_list;
};

void CSVList::AddEntry(const CSVEntry& e)
{
    m_list.push_back(e);
}

class CSVGroup
{
public:
    CSVGroup() : m_num{}, m_lists{} {}
    CSVGroup(int num) : m_num{ num }, m_lists{} {}
    void AddList(const CSVList& list);
    int  Num() { return m_num; }

private:
    int                  m_num;
    std::vector<CSVList> m_lists;
};

void CSVGroup::AddList(const CSVList& list)
{
    m_lists.push_back(list);
}

std::map<int, CSVGroup> lddMap;

int ReadNumber(TextStream& in)
{
    std::string num;
    char        ch = in.Get();
    while (std::isxdigit(ch))
    {
	num += ch;
	ch = in.Get();
    }
    in.UnGet(ch);
    return std::stol(num, nullptr, 16);
}

CSVList ReadList(TextStream& in, int listNum)
{
    CSVList list{ listNum };
    do
    {
	bool        isNum = true;
	std::string str;
	while (in.Peek() != ',' && in.Peek() != '\n')
	{
	    char ch = in.Get();
	    isNum &= std::isxdigit(ch);
	    str += ch;
	}
	if (isNum && str != "")
	{
	    list.AddEntry(CSVEntry(std::stol(str, nullptr, 16)));
	}
	else
	{
	    list.AddEntry(CSVEntry(str));
	}
	if (in.Peek() == '\n')
	{
	    return list;
	}
	in.Expect(',');
    } while (true);
}

CSVGroup ReadContent(TextStream& in, int groupNum)
{
    CSVGroup g(groupNum);
    while (isxdigit(in.Peek()))
    {
	int num = ReadNumber(in);
	// Catch the last line, which doesn't have an equal sign.
	if (in.Peek() != '=')
	{
	    in.Expect('\n');
	    // This should read "past the end".
	    in.Peek();
	    assert(!in);
	    return g;
	}
	in.Expect('=');
	CSVList l = ReadList(in, num);
	g.AddList(l);
	in.Expect('\n');
    }
    return g;
}

// LDD looks a lot like a Windows style "ini" file.
void ReadLDD(TextStream& in)
{
    int num;
    while (in)
    {
	char ch = in.Get();
	if (ch == '[')
	{
	    num = ReadNumber(in);
	    std::cout << "num=" << num << std::endl;
	    in.Expect(']');
	    in.Expect('\n');

	    auto v = ReadContent(in, num);
	    lddMap[v.Num()] = v;
	}
    }
}

void Usage()
{
    std::cerr << "Usage: logconvert filename" << std::endl;
}

int main(int argc, char** argv)
{
    std::string fname;

    if (argc != 2)
    {
	Usage();
	exit(1);
    }

    if (argc > 1)
    {
	fname = argv[1];
    }

    try
    {
	TextStream in(fname);
	ReadLDD(in);
    }
    catch (std::exception& e)
    {
	std::cerr << "Exception caught... details:" << e.what() << std::endl;
	exit(1);
    }
    return 0;
}
