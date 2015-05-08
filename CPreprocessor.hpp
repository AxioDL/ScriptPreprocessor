#ifndef CPREPROCESSOR_HPP
#define CPREPROCESSOR_HPP

#include <map>
#include <string>
#include <functional>
#include "CLexer.hpp"
#include "CLineTranslator.hpp"

class CPreprocessor
{
public:
    struct PragmaInstance
    {
        std::string name;
        std::string text;
        std::string currentFile;
        unsigned int currentLine;
        std::string rootFile;
        std::string globalLine;
    };

    CPreprocessor();
    typedef std::map<std::string, int> ArgSet;
    struct DefineEntry
    {
        CLexer::TokenList tokens;
        ArgSet arguments;
    };

    typedef std::map<std::string, DefineEntry> DefineTable;
    typedef DefineTable::iterator DefineIterator;
    typedef std::map<std::string, std::function<void(PragmaInstance)> > PragmaMap;
    typedef PragmaMap::iterator PragmaIterator;

    void define(const std::string& def);
    void undefine(const std::string& def);
    void registerPragma(const std::string& name, std::function<void(PragmaInstance)>  cb);

    std::string finalizedSource();
    int preprocessFile(const std::string& filename);
    int preprocessCode(const std::string& filename, const std::string& code);
private:
    std::string _loadSource(const std::string& filename);
    void printErrorMessage(const std::string& errMsg);
    void printWarningMessage(const std::string& warnMesg);

    void preprocessRecursive(const std::string& filename, const std::string& code, CLexer::TokenList& tokens, DefineTable& defineTable);

    void callPragma(const std::string& name, const PragmaInstance& parms);
    void _advanceList(CLexer::TokenList& tokens);
    CLexer::TokenIterator _findToken(CLexer::TokenIterator begin, CLexer::TokenIterator end, CLexer::TokenType type);
    CLexer::TokenIterator _parseStatement(CLexer::TokenIterator begin, CLexer::TokenIterator end, CLexer::TokenList& dest);
    CLexer::TokenIterator _parseDefineArguments(CLexer::TokenIterator begin, CLexer::TokenIterator end, CLexer::TokenList& tokens, std::vector<CLexer::TokenList>& args);
    CLexer::TokenIterator _expandDefine(CLexer::TokenIterator begin, CLexer::TokenIterator end, CLexer::TokenList& tokens, DefineTable& defineTable);
    void _parseDefine(DefineTable& defineTable, CLexer::TokenList& tokens);
    CLexer::TokenIterator _parseIfDef(CLexer::TokenIterator begin, CLexer::TokenIterator end);
    void _parseIf(CLexer::TokenList& directive, std::string& nameOut);
    void _parsePragma(CLexer::TokenList& args);
    std::string _expandMessage(DefineTable& defineTable, CLexer::TokenList& args);
    void _parseWarning(CLexer::TokenList& args, DefineTable& defineTable);
    void _parseError(CLexer::TokenList& args, DefineTable& defineTable);

    DefineTable      m_applicationDefined;
    PragmaMap        m_registeredPragmas;
    CLineTranslator  m_lineTranslator;
    CLexer::TokenList m_tokens;

    std::string  m_rootFile;
    std::string  m_currentFile;
    unsigned int m_currentLine;
    unsigned int m_currentFileLines;
    unsigned int m_errorCount;
};

#endif // CPREPROCESSOR_HPP
