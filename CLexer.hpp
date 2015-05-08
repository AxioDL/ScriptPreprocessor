#ifndef CLEXER_HPP
#define CLEXER_HPP


#include <list>
#include <string>

class CLexer
{
public:
    enum TokenType
    {
        IDENTIFIER,		//Names which can be expanded.
        COMMA,			//,
        SEMICOLON,
        OPEN,			//{[(
        CLOSE,			//}])
        PREPROCESSOR,	//Begins with #
        NEWLINE,
        WHITESPACE,
        IGNORE,
        COMMENT,
        STRING,
        NUMBER
    };

    struct Token
    {
        std::string value;
        TokenType type;
    };

    typedef std::list<CLexer::Token> TokenList;
    typedef TokenList::iterator TokenIterator;

    CLexer()
    {
    }

    int lex(char* start, char* end, TokenList& tokens);
private:
    bool _searchString(const std::string& str, char in) const;
    bool _isTrivial(char in) const;
    bool _isIdentifierStart(char in) const;
    bool _isIdentifierBody(char in) const;
    bool _isNumber(char in) const;
    bool _isHex(char in) const;
    char* _parseToken(char* start, char* end, Token& out);
    char* _parseLiteral(char* start, char* end, Token& out);
    char* _parseLineComment(char* start, char* end, Token& out);
    char* _parseBlockComment(char* start, char* end, Token& out);
    char* _parseNumber(char* start, char* end, Token& out);
    char* _parseHexConstant(char* start, char* end, Token& out);
    char* _parseFloatingPoint(char* start, char* end, Token& out);
    char* _parseCharacterLiteral(char* start, char* end, Token& out);
    char* _parseStringLiteral(char* start, char* end, Token& out);
    char* _parseIdentifier(char* start, char* end, Token& out);
};

#endif // CLEXER_HPP
