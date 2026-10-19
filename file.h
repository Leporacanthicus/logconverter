#ifndef FILE_H__
#define FILE_H__

#include <string>
#include <fstream>

class TextStream
{
public:
    TextStream(const std::string& name) :  m_name{name}, m_linenum{1}, m_curChar{}, m_curCharValid{}
    {
	Open();
    }
    char Get();
    char Peek();
    void UnGet(char c);
    void Expect(char c);

    operator bool() { return (bool)m_file; }
protected:
    void Open();
    
private:
    std::ifstream m_file;
    std::string m_name;
    int m_linenum;
    char m_curChar;
    bool m_curCharValid;
};

class BinaryStream
{
public:
    BinaryStream(const std::string& name) : m_name{ name }, m_offset{ 0 } { Open(); }
    void Open();
    template<typename T>
    T Read()
    {
	T data{};
	m_file.read(reinterpret_cast<char*>(&data), sizeof(data));
	return data;
    }

private:
    std::ifstream m_file;
    std::string   m_name;
    std::string   m_offset;
};

#endif // FILE_H
