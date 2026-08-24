#include <cassert>
#include <string>

#include "file.h"
#include "exceptions.h"

char TextStream::Get()
{
    if (m_curCharValid)
    {
	m_curCharValid = false;
	return m_curChar;
    }
    char ch = m_file.get();

    // Ignore CR characters. These files may be windows format.
    if (ch == '\r')
    {
	ch = m_file.get();
	assert(ch == '\n' && "Expected line feed afer CR char");
    }
    if (ch == '\n')
    {
	m_linenum++;
    }
    return ch;
}

void TextStream::UnGet(char c)
{

    assert(!m_curCharValid && "Already ungetted");
    m_curCharValid = true;
    m_curChar = c;
}

char TextStream::Peek()
{
    char c = Get();
    UnGet(c);
    return c;
}

void TextStream::Expect(char c)
{
    char ch = Get();
    if (ch != c)
    {
	throw ExpectedException(std::string(1, c), std::string(1, ch), m_linenum, m_name);
    }
}

void TextStream::Open()
{
    // Open file, throw on failure.
    auto old = m_file.exceptions();  
    m_file.exceptions(old | std::ifstream::failbit);
    m_file.open(m_name);
    m_file.exceptions(old);
}
