#include <cassert>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <format>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "exceptions.h"
#include "file.h"

enum class CSVType
{
    Unknown,
    String,
    Number,
};

struct NumRange
{
    unsigned low, high;
};

struct CSVGroupInfo
{
    unsigned num, low, high;
    CSVType  types[10];
};

// We only cover groups that genuinely have ambiguous content, such as group 4,
// which uses C as "degrees C", but could also be hex C (decimal 12).
CSVGroupInfo groupInfo[] = {
    { 4,
      0x300u,
      0x3ffu,
      { CSVType::String, CSVType::String, CSVType::String, CSVType::Number,
        CSVType::String, CSVType::Number, CSVType::String } },
};

class CSVEntry
{
public:
    CSVEntry() : m_ty{ CSVType::Unknown } {}
    CSVEntry(int i) : m_ty{ CSVType::Number } { m_content.i = i; }
    CSVEntry(const std::string& s) : m_ty{ CSVType::String }
    {
	m_content.s = s;
	m_content.i = 0xdeadbeef;
    }

    int                AsNumber() const;
    const std::string& AsString() const;

private:
    CSVType m_ty;
    struct
    {
	int         i;
	std::string s;
    } m_content;
};

const std::string& CSVEntry::AsString() const
{
    assert(m_ty == CSVType::String);
    return m_content.s;
}

int CSVEntry::AsNumber() const
{
    assert(m_ty == CSVType::Number);
    return m_content.i;
}

class CSVList
{
public:
    CSVList(unsigned num) : m_num{ num } {}
    void AddEntry(const CSVEntry& e);
    unsigned                     Num() const { return m_num; }
    const std::vector<CSVEntry>& List() const { return m_list; }

private:
    unsigned              m_num;
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
    CSVGroup(unsigned num) : m_num{ num }, m_lists{} {}
    void AddList(const CSVList& list);
    unsigned                    Num() const { return m_num; }
    const std::vector<CSVList>& List() const { return m_lists; };

private:
    unsigned             m_num;
    std::vector<CSVList> m_lists;
};

void CSVGroup::AddList(const CSVList& list)
{
    m_lists.push_back(list);
}

std::map<int, CSVGroup> lddMap;

const std::pair<bool, CSVGroupInfo> FindGroupInfo(unsigned group, unsigned num)
{
    for (auto g : groupInfo)
    {
	if (g.low <= group && g.high >= group && g.num == num)
	{
	    return std::make_pair(true, g);
	}
    }
    return std::make_pair(false, CSVGroupInfo{});
}

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

CSVList ReadList(TextStream& in, unsigned groupNum, unsigned listNum)
{
    CSVList list{ listNum };
    auto [found, gi] = FindGroupInfo(groupNum, listNum);
    unsigned index = 0;
    do
    {
	bool        isNum = true;
	std::string str;
	while (in.Peek() != ',' && in.Peek() != '\n')
	{
	    char ch = in.Get();
	    isNum &= !!std::isxdigit(ch);
	    str += ch;
	}
	if (found)
	{
	    isNum &= gi.types[index] == CSVType::Number;
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
	index++;
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
	CSVList l = ReadList(in, groupNum, num);
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
	    //	    std::cout << "num=" << num << std::endl;
	    in.Expect(']');
	    in.Expect('\n');

	    auto v = ReadContent(in, num);
	    [[maybe_unused]] auto [it, success] = lddMap.insert({ v.Num(), v });
	    assert(success);
	}
    }
}

void ReadSome(std::ifstream& f, const size_t bytes, const unsigned bits)
{
    constexpr size_t BUFSIZE = 1024;
    char             buf[BUFSIZE];
    assert(bytes <= BUFSIZE);
    f.read(buf, bytes);
    unsigned wordSize = bits / 8;
    assert(((bits & 7) == 0) && ((bits & (bits - 1)) == 0));
    assert(bits <= 32);
    for (size_t i = 0; i < bytes; i += wordSize)
    {
	unsigned x = 0;
	std::memcpy(&x, &buf[i], wordSize);
	std::cout << std::format("{:0{}x}", x, wordSize * 2) << " ";
	if (((i + wordSize - 1) & 15) == 15)
	{
	    std::cout << std::endl;
	}
    }
}

void Usage()
{
    std::cerr << "Usage: logconvert filename" << std::endl;
}

int main(int argc, char** argv)
{
    std::filesystem::path fname;

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
	for (const auto& i : lddMap)
	{
	    if (i.first >= 0x300 && i.first <= 0x3ff)
	    {
		std::string heading;
		std::string units;
		unsigned    bits;
		for (const auto& j : i.second.List())
		{
		    const auto& list = j.List();
		    if (j.Num() == 1)
		    {
			bits = list[5].AsNumber();
		    }
		    if (j.Num() == 4)
		    {
			heading = list[0].AsString();
			units = list[1].AsString();
		    }
		    if (j.Num() == 6)
		    {
			std::filesystem::path newName = fname;
			newName.replace_extension(list[3].AsString());
			std::ifstream binf(newName,
			                   std::ios_base::in | std::ios_base::binary);
			if (binf)
			{
			    std::cerr << "Opened: " << newName << std::endl;
			    std::cerr << "Content: " << heading << " Unit: " << units
			              << std::endl;
			    ReadSome(binf, 256, bits);
			}
		    }
		}
	    }
	}
    }
    catch (std::exception& e)
    {
	std::cerr << "Exception caught... details:" << e.what() << std::endl;
	exit(1);
    }
    return 0;
}
