#ifndef EXCEPTIONS_H__
#define EXCEPTIONS_H__

#include <exception>
#include <string>

class BaseException : public std::exception
{
public:
    BaseException(const std::string& msg) : m_whatString{ msg } {}
    const char* what() const noexcept override { return m_whatString.c_str(); }

private:
    std::string m_whatString;
};

class ExpectedException : public BaseException
{
public:
    ExpectedException(const std::string& expected, const std::string& got, int line,
                      const std::string& fname);
};

#endif // EXCEPTIONS_H__
