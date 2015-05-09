#ifndef CLEXER_HPP
#define CLEXER_HPP


#include <list>
#include <string>

class CLexer
{
public:
    enum TokenType
    {
        INVALID,
        IDENTIFIER,		//Names which can be expanded.
        COMMA,			//,
        SEMICOLON,
        OPEN,			//{[(
        CLOSE,			//}])
        PREPROCESSOR,	        //Begins with #
        NEWLINE,
        WHITESPACE,
        IGNORE,
        COMMENT,
        STRING,
        NUMBER,
        KEYWORD,
        FUNCTION,
        MACRO,
        OPERATOR
    };

    enum OperatorType
    {
    };

    struct Token
    {
        Token()
            : type(INVALID),
              degenerate(false)
        {
        }

        std::string value;
        TokenType type;
        union
        {
            OperatorType opType;
        };
        bool degenerate;
    };

    typedef std::list<CLexer::Token> TokenList;
    typedef TokenList::iterator TokenIterator;

    CLexer();

    void lex(char* start, char* end, TokenList& tokens);
private:
    bool _searchString(const std::string& str, char in) const;
    bool _isTrivial(char in) const;
    bool _isIdentifierStart(char in) const;
    bool _isIdentifierBody(char in) const;
    bool _isNumber(char in) const;
    bool _isBinary(char in) const;
    bool _isHex(char in) const;
    bool _isOperatorStart(char in) const;
    bool _isOperator(const std::string& val) const;
    bool _isKeyword(const std::string& val) const;
    char* _parseToken(char* start, char* end, Token& out);
    char* _parseLiteral(char* start, char* end, Token& out);
    char* _parseLineComment(char* start, char* end, Token& out);
    char* _parseBlockComment(char* start, char* end, Token& out);
    char* _parseNumber(char* start, char* end, Token& out);
    char* _parseBinaryConstant(char* start, char* end, Token& out);
    char* _parseHexConstant(char* start, char* end, Token& out);
    char* _parseFloatingPoint(char* start, char* end, Token& out);
    char* _parseCharacterLiteral(char* start, char* end, Token& out);
    char* _parseStringLiteral(char* start, char* end, Token& out);
    char* _parseIdentifier(char* start, char* end, Token& out);
    char* _parseOperator(char* start, char* end, Token& out);
    Token* m_lastIdentifier;
};

#endif // CLEXER_HPP
