#include "CLexer.hpp"
#include <algorithm>
#include <assert.h>

const std::string numbers = "0123456789";
const std::string identifierStart = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string identifierBody = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const std::string hexnumbers = "0123456789abcdefABCDEF";
const std::string trivials = ",;\n\r\t [{(]})";

const CLexer::TokenType TrivialTypes[] =
{
	CLexer::COMMA,
	CLexer::SEMICOLON,
	CLexer::NEWLINE,
	CLexer::WHITESPACE,
	CLexer::WHITESPACE,
	CLexer::WHITESPACE,
	CLexer::OPEN,
	CLexer::OPEN,
	CLexer::OPEN,
	CLexer::CLOSE,
	CLexer::CLOSE,
	CLexer::CLOSE
};

int CLexer::lex(char* start, char* end, TokenList& tokens)
{
    assert(start != 0 && end != 0 && "start and end cannot be null");
    assert(start <= end && "degenerate lex detected: end < start");

    while (true)
    {
        CLexer::Token currentToken;
        start = _parseToken(start, end, currentToken);
        //if (currentToken.type != CLexer::COMMENT)
        tokens.push_back(currentToken);
        if (start == end)
            return 0;
    }
}

bool CLexer::_searchString(const std::string& str, char in) const
{
    return (str.find_first_of(in) != std::string::npos);
}

bool CLexer::_isTrivial(char in) const
{
    return _searchString(trivials, in);
}

bool CLexer::_isIdentifierStart(char in) const
{
    return _searchString(identifierStart, in);
}

bool CLexer::_isIdentifierBody(char in) const
{
    return _searchString(identifierBody, in);
}

bool CLexer::_isNumber(char in) const
{
    return _searchString(numbers, in);
}

bool CLexer::_isHex(char in) const
{
    return _searchString(hexnumbers, in);
}

char* CLexer::_parseToken(char* start, char* end, CLexer::Token& out)
{
    if (start == end)
        return start;
    char curChar = *start;
    if (_isTrivial(curChar))
    {
        out.value += curChar;
        out.type = TrivialTypes[trivials.find_first_of(curChar)];
        return ++start;
    }

    if (_isIdentifierStart(curChar))
        return _parseIdentifier(start, end, out);

    if (curChar == '#')
    {
        start = _parseIdentifier(start, end, out);
        out.type = CLexer::PREPROCESSOR;
        return start;
    }


    if (_isNumber(curChar))
        return _parseNumber(start, end, out);
    if (curChar == '\"')
        return _parseStringLiteral(start, end, out);
    if (curChar == '\'')
        return _parseCharacterLiteral(start, end, out);
    if (curChar == '/')
    {
        ++start;
        // Is it a comment?
        if (start == end)
            return start;
        if (*start == '*')
            return _parseBlockComment(start, end, out);
        if (*start == '/')
            return _parseLineComment(start, end, out);
        --start;
    }

    // Nothing intradasting
    out.value = curChar;
    out.type = CLexer::IGNORE;
    return ++start;
}

char* CLexer::_parseLineComment(char* start, char* end, CLexer::Token& out)
{
    out.type = CLexer::COMMENT;
    out.value += "//";

    while (true)
    {
        ++start;
        if (start == end)
            return start;
        out.value += *start;
        if  (*start == '\n')
            return start;
    }
}

char* CLexer::_parseBlockComment(char* start, char* end, CLexer::Token& out)
{
    out.type = CLexer::COMMENT;
    out.value += "/*";

    while (true)
    {
        ++start;
        if (start == end)
            break;

        out.value += *start;
        if (*start == '*')
        {
            ++start;
            if (start == end)
                break;
            if (*start == '/')
            {
                out.value += *start;
                ++start;
                break;
            }
            else
                --start;
        }
    }

    return start;
}

char* CLexer::_parseNumber(char* start, char* end, CLexer::Token& out)
{
    out.type = CLexer::NUMBER;
    out.value += *start;
    while (true)
    {
        ++start;
        if (start == end)
            return start;
        if (_isNumber(*start))
            out.value += *start;
        else if (*start == '.')
            return _parseFloatingPoint(start, end, out);
        else if (*start == 'x')
            return _parseFloatingPoint(start, end, out);
        else
            return start;
    }
}

char* CLexer::_parseHexConstant(char* start, char* end, CLexer::Token& out)
{
    out.value += *start;
    while (true)
    {
        ++start;
        if (start == end)
            return start;
        if (_isHex(*start))
            out.value += *start;
        else
            return start;
    }
}

char* CLexer::_parseFloatingPoint(char* start, char* end, CLexer::Token& out)
{
    out.value += *start;
    while (true)
    {
        ++start;
        if (start == end)
            return start;
        if (!_isNumber(*start))
        {
            if (*start == 'f' || *start == 'F')
            {
                out.value += *start;
                ++start;
                return start;
            }
            return start;
        }
        else
            out.value += *start;
    }
}

char* CLexer::_parseCharacterLiteral(char* start, char* end, CLexer::Token& out)
{
    ++start;
    if (start == end)
        return start;
    out.type = CLexer::NUMBER;
    if (*start == '\\')
    {
        ++start;
        if (start == end)
            return start;
        if (*start == 'n')
            out.value = '\n';
        if (*start == 't')
            out.value = '\t';
        if (*start == 'r')
            out.value = '\r';
        ++start;
        if (start == end)
            return start;
        ++start;
    }
    else
    {
        out.value = *start;
        ++start;
        if (start == end)
            return start;
        ++start;
    }
    return start;
}

char* CLexer::_parseStringLiteral(char* start, char* end, CLexer::Token& out)
{
    out.type = CLexer::STRING;
    out.value += *start;

    while (true)
    {
        ++start;
        if (start == end)
            return start;
        out.value += *start;

        // Are we at the end of the string?
        if (*start == '\"')
            return ++start;
        // Is it an escape sequence?
        if (*start == '\\')
        {
            ++start;
            if (start == end)
                return start;
            out.value += *start;
        }
    }
}

char* CLexer::_parseIdentifier(char* start, char* end, CLexer::Token& out)
{
    out.type = CLexer::IDENTIFIER;
    out.value += *start;
    while (true)
    {
        ++start;
        if (start == end)
            return start;
        if (_isIdentifierBody(*start))
            out.value += *start;
        else
            return start;
    }
}


