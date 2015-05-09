#include "CLexer.hpp"
#include <algorithm>
#include <assert.h>

const std::string numbers = "0123456789";
const std::string identifierStart = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string identifierBody = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const std::string hexnumbers = "0123456789abcdefABCDEF";
const std::string binaryNumbers = "01";
const std::vector<std::string> keywords =
{
    "and", "abstract", "auto", "bool", "break",
    "case", "cast", "class", "const", "continue",
    "default", "do", "double", "else", "enum",
    "false", "final", "float", "for", "from",
    "funcdef", "get", "if", "import", "in",
    "inout", "int", "interface", "int8",
    "int16", "int32", "int64", "is",
    "mixin", "namespace", "not", "null",
    "or", "out", "override", "private",
    "protected", "return", "set", "shared",
    "super", "switch", "this", "true", "typedef",
    "uint", "uint8", "uint16", "uint32", "uint64",
    "void", "while", "xor"
};

const std::string trivials = ",;\n\r\t [{(]})";
const std::string opStart = "*/%+-<=>!?:^&>@|~.";

const std::vector<std::string> operators=
{
    "*","**","/","%","+","-","<=",">"">=","==",
    "!=","?","+=","-=","*=","/=","%=","**=","++",
    "--","&","|","~","^","<<",">>",">>>","<<=",
    ">>=",">>>=",".","||","!","^^","::","="
};

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

CLexer::CLexer()
    : m_lastIdentifier(nullptr)
{
}

void CLexer::lex(char* start, char* end, TokenList& tokens)
{
    assert(start != 0 && end != 0 && "start and end cannot be null");
    assert(start <= end && "degenerate lex detected: end < start");

    while (true)
    {
        CLexer::Token currentToken;
        start = _parseToken(start, end, currentToken);
        if (currentToken.type != CLexer::INVALID)
            tokens.push_back(currentToken);

        if (currentToken.value != "#include")
        {
            if ((currentToken.type == CLexer::IDENTIFIER || currentToken.type == CLexer::PREPROCESSOR) && !m_lastIdentifier)
                m_lastIdentifier = &tokens.back();
        }
        else
            m_lastIdentifier = nullptr;

        if (start == end)
            break;
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

bool CLexer::_isBinary(char in) const
{
    return _searchString(binaryNumbers, in);
}

bool CLexer::_isHex(char in) const
{
    return _searchString(hexnumbers, in);
}

bool CLexer::_isOperatorStart(char in) const
{
    return _searchString(opStart, in);
}

bool CLexer::_isOperator(const std::string& val) const
{
    auto it = std::find(operators.begin(), operators.end(), val);
    return (it != operators.end());
}

bool CLexer::_isKeyword(const std::string& val) const
{
    auto it = std::find(keywords.begin(), keywords.end(), val);
    return (it != keywords.end());
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
        if (out.value == "(" && m_lastIdentifier)
        {
            if (m_lastIdentifier->type == CLexer::IDENTIFIER)
                m_lastIdentifier->type = CLexer::FUNCTION;
            else if (m_lastIdentifier->type == CLexer::PREPROCESSOR && m_lastIdentifier->value == "#define")
                m_lastIdentifier->type = CLexer::MACRO;
        }
        else if (out.value == "\n")
            m_lastIdentifier = nullptr;

        return ++start;
    }

    if (m_lastIdentifier && curChar == '\\')
    {
        if ((m_lastIdentifier->type == CLexer::PREPROCESSOR || m_lastIdentifier->type == CLexer::MACRO))
        {
            // only handle this if we're working on a preprocessor or a macro
            while(*start != '\n')
                ++start;
            *start = ' ';
            return start;
        }
    }

    if (_isIdentifierStart(curChar))
    {
        start = _parseIdentifier(start, end, out);
        if (_isKeyword(out.value))
            out.type = CLexer::KEYWORD;

        return start;
    }

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
    if (_isOperatorStart(curChar))
        return _parseOperator(start, end, out);

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
    bool opened = true;

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
                opened = false;
                break;
            }
            else
                --start;
        }
    }

    out.degenerate = opened;
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
            return _parseHexConstant(start, end, out);
        else if (*start == 'd')
            out.value += *start;
        else if (*start == 'b')
            return _parseBinaryConstant(start, end, out);
        else
            return start;
    }
}

char* CLexer::_parseBinaryConstant(char* start, char* end, CLexer::Token& out)
{
    out.value += *start;
    while (true)
    {
        ++start;
        if (start == end)
            return start;
        if (_isBinary(*start))
            out.value += *start;
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
    bool hasExponent = false;
    while (true)
    {
        ++start;
        if (start == end)
            return start;
        if (!_isNumber(*start))
        {
            if (*start == 'e')
            {
                out.degenerate = hasExponent;
                out.value += *start;
                hasExponent = true;
                continue;
            }
            else if (*start == 'f')
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

char* CLexer::_parseOperator(char* start, char* end, CLexer::Token& out)
{
    out.type = CLexer::OPERATOR;
    std::string operatorValue;

    while (true)
    {
        if (start == end)
        {
            out.type = CLexer::INVALID;
            return start;
        }
        if (_isOperatorStart(*start))
            operatorValue += *start;
        else
            break;

        ++start;
    }

    if (_isOperator(operatorValue))
        out.value = operatorValue;
    return start;
}


