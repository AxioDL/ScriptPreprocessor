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
    struct Macro
    {
        std::string name;
        std::vector<CLexer::Token> args;
        std::vector<CLexer::Token> code;
    };

    struct PreprocessorState
    {
        std::string currentFile;
        std::string rootFile;
        unsigned int currentLine;
        unsigned int globalLine;
    };

    struct PragmaInstance
    {
        std::string name;
        std::string text;
        PreprocessorState state;
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
    typedef std::map<std::string, std::function<void(CLexer::TokenList&, DefineTable&, PreprocessorState)> > HookMap;
    typedef HookMap::iterator HookIterator;
    typedef std::vector<Macro> MacroList;
    typedef MacroList::iterator MacroIterator;

    void define(const std::string& def);
    void undefine(const std::string& def);
    void registerPragma(const std::string& name, std::function<void(PragmaInstance)>  cb);
    void registerHook(const std::string& name, std::function<void(CLexer::TokenList&, DefineTable&, PreprocessorState)> cb);

    std::string finalizedSource();
    bool preprocessFile(const std::string& filename);
    bool preprocessCode(const std::string& filename, const std::string& code);

    static void advanceList(CLexer::TokenList& tokens);
private:
    std::string _loadSource(const std::string& filename);
    void printErrorMessage(const std::string& errMsg);
    void printWarningMessage(const std::string& warnMesg);

    bool preprocessRecursive(const std::string& filename, const std::string& code, CLexer::TokenList& tokens, DefineTable& defineTable);

    void callPragma(const std::string& name, const PragmaInstance& parms);
    CLexer::TokenIterator _findToken(CLexer::TokenIterator begin, CLexer::TokenIterator end, CLexer::TokenType type);
    CLexer::TokenIterator _parseStatement(CLexer::TokenIterator begin, CLexer::TokenIterator end, CLexer::TokenList& dest);
    CLexer::TokenIterator _parseDefineArguments(CLexer::TokenIterator begin, CLexer::TokenIterator end, CLexer::TokenList& tokens, std::vector<CLexer::TokenList>& args);
    CLexer::TokenIterator _expandDefine(CLexer::TokenIterator begin, CLexer::TokenIterator end, CLexer::TokenList& tokens, DefineTable& defineTable);
    void _expandMacro(CLexer::TokenIterator begin, CLexer::TokenIterator end, CLexer::TokenList& tokens, const Macro& macro);
    void _parseDefine(DefineTable& defineTable, CLexer::TokenList& tokens);
    CLexer::TokenIterator _parseIfDef(CLexer::TokenIterator begin, CLexer::TokenIterator end);
    void _parseIf(CLexer::TokenList& directive, std::string& nameOut);
    void _parsePragma(CLexer::TokenList& args);
    std::string _expandMessage(DefineTable& defineTable, CLexer::TokenList& args);
    void _parseWarning(CLexer::TokenList& args, DefineTable& defineTable);
    void _parseError(CLexer::TokenList& args, DefineTable& defineTable);
    CLexer::TokenIterator _parseIdentifier(CLexer::TokenIterator begin, CLexer::TokenIterator end, CLexer::TokenList& tokens, DefineTable& defineTable);

    DefineTable      m_applicationDefined;
    PragmaMap        m_registeredPragmas;
    HookMap          m_registeredHooks;
    CLineTranslator  m_lineTranslator;
    CLexer::TokenList m_tokens;

    std::string  m_rootFile;
    std::string  m_currentFile;
    unsigned int m_currentLine;
    unsigned int m_currentFileLines;
    unsigned int m_errorCount;
    MacroList    m_macros;
};

#endif // CPREPROCESSOR_HPP
