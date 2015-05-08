#include "CPreprocessor.hpp"
#include <stdio.h>
#include <sstream>
#include <iostream>

static std::string removeQuotes(const std::string& in)
{
    return in.substr(1,in.size()-2);
}

static std::string addPaths(const std::string& first, const std::string& second)
{
    std::string result;
    size_t slash_pos = first.find_last_of('/');
    if (slash_pos == 0 || slash_pos >= first.size()) return second;
    result = first.substr(0,slash_pos+1);
    result += second;
    return result;
}

static void setLineMacro(CPreprocessor::DefineTable& defineTable, unsigned int line)
{
    CPreprocessor::DefineEntry def;
    CLexer::Token token;
    token.type = CLexer::NUMBER;
    std::stringstream sstr;
    sstr << (line+1);
    sstr >> token.value;
    def.tokens.push_back(token);
    defineTable["__LINE__"] = def;
}

static void setFileMacro(CPreprocessor::DefineTable& defineTable, const std::string& file)
{
    CPreprocessor::DefineEntry def;
    CLexer::Token token;
    token.type = CLexer::STRING;
    token.value = std::string("\"")+file+"\"";
    def.tokens.push_back(token);
    defineTable["__FILE__"] = def;
}

CPreprocessor::CPreprocessor()
{
}

void CPreprocessor::define(const std::string& def)
{
    if (def.empty())
        return;

    std::string data = "#define ";
    data += def + "\n";
    CLexer::TokenList tokens;
    CLexer lexer;
    lexer.lex(&data.front(), &data.back(), tokens);

    _parseDefine(m_applicationDefined, tokens);
}

void CPreprocessor::undefine(const std::string& def)
{
    m_applicationDefined.erase(def);
}

void CPreprocessor::registerPragma(const std::string& name, std::function<void (PragmaInstance)> cb)
{
    PragmaIterator iter = m_registeredPragmas.find(name);
    if (iter != m_registeredPragmas.end())
        m_registeredPragmas.erase(iter);

    m_registeredPragmas[name] = cb;
}

void CPreprocessor::registerHook(const std::string& name, std::function<void (CLexer::TokenList&, CPreprocessor::DefineTable&)> cb)
{
    std::string pre = "#" + name;
    HookIterator iter = m_registeredHooks.find(pre);
    if (iter != m_registeredHooks.end())
        m_registeredHooks.erase(iter);

    m_registeredHooks[pre] = cb;
}

std::string CPreprocessor::finalizedSource()
{
    std::string ret;
    for (const CLexer::Token& token : m_tokens)
        ret += token.value;

    return ret;
}

int CPreprocessor::preprocessFile(const std::string& filename)
{
    std::string code = _loadSource(filename);
    if (code == std::string())
        printErrorMessage(std::string("Empty source file specified: ") + filename);
    else
        preprocessCode(filename, code);
    return m_errorCount;
}

int CPreprocessor::preprocessCode(const std::string& filename, const std::string& code)
{
    m_currentLine = 0;
    m_errorCount = 0;
    m_rootFile = filename;
    DefineTable defineTable = m_applicationDefined;
    m_lineTranslator.reset();
    m_tokens.clear();
    preprocessRecursive(filename, code, m_tokens, defineTable);

    return m_errorCount;
}

std::string CPreprocessor::_loadSource(const std::string& filename)
{
    std::string code;
    FILE* file = fopen(filename.c_str(), "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        size_t length = ftell(file);
        rewind(file);
        code.resize(length);
        fread(&code.front(), 1, length, file);
        fclose(file);

        if (code != std::string() && code.back() != '\n')
        {
            printWarningMessage(std::string("No new line at end of file: ") + filename);
            code += '\n';
        }
    }
    return code;
}

void CPreprocessor::advanceList(CLexer::TokenList& tokens)
{
    tokens.pop_front();
    while(true)
    {
        if (tokens.front().type != CLexer::WHITESPACE)
            break;
        tokens.pop_front();
    }
}

void CPreprocessor::printErrorMessage(const std::string& errMsg)
{
    std::cout << errMsg << std::endl;
    m_errorCount++;
}

void CPreprocessor::printWarningMessage(const std::string& warnMesg)
{
    std::cout << warnMesg << std::endl;
}

void CPreprocessor::preprocessRecursive(const std::string& filename, const std::string& code, CLexer::TokenList& tokens, DefineTable& defineTable)
{
    unsigned int startLine = m_currentLine;
    m_currentFile = filename;
    m_currentFileLines = 0;
    setFileMacro(defineTable, m_currentFile);
    setLineMacro(defineTable, m_currentFileLines);

    CLexer lexer;
    lexer.lex((char*)&code.front(), (char*)&code.back(), tokens);

    CLexer::TokenIterator begin = tokens.begin();
    CLexer::TokenIterator end   = tokens.end();

    while (begin != end)
    {
        while (begin->type == CLexer::WHITESPACE)
            ++begin;

        if (begin->type == CLexer::NEWLINE)
        {
            m_currentLine++;
            m_currentFileLines++;
            ++begin;
            setLineMacro(defineTable, m_currentFileLines);
        }
        else if (begin->type == CLexer::PREPROCESSOR)
        {
            CLexer::TokenIterator lineStart = begin;
            CLexer::TokenIterator lineEnd = _findToken(begin, end, CLexer::NEWLINE);
            CLexer::TokenList directive(lineStart, lineEnd);
            begin = tokens.erase(lineStart, lineEnd);

            std::string value = directive.begin()->value;
            if (value == "#define")
                _parseDefine(defineTable, directive);
            else if (value == "#undef")
            {
                std::string defName;
                _parseIf(directive, defName);
                DefineIterator defineIter = defineTable.find(defName);
                if (defineIter != defineTable.end())
                    defineTable.erase(defName);
            }
            else if (value == "#ifdef")
            {
                std::string defName;
                _parseIf(directive, defName);
                DefineIterator defineIter = defineTable.find(defName);
                if (defineIter == defineTable.end())
                {
                    CLexer::TokenIterator spliceTo = _parseIfDef(begin, end);
                    begin = tokens.erase(begin, spliceTo);
                }
            }
            else if (value == "#ifndef")
            {
                std::string defName;
                _parseIf(directive, defName);
                DefineIterator defineIter = defineTable.find(defName);
                if (defineIter != defineTable.end())
                {
                    CLexer::TokenIterator spliceTo = _parseIfDef(begin, end);
                    begin = tokens.erase(begin, spliceTo);
                }
            }
            else if (value == "#include")
            {
                m_lineTranslator.table().addLineRange(filename, startLine, m_currentLine - m_currentFileLines);
                std::string includeFilename;
                _parseIf(directive, includeFilename);
                includeFilename = removeQuotes(includeFilename);
                std::string newCode = _loadSource(includeFilename);
                if (newCode != std::string())
                {
                    unsigned int oldCurrentFileLines = m_currentFileLines;
                    CLexer::TokenList nextFile;
                    preprocessRecursive(addPaths(filename, includeFilename), newCode, nextFile, defineTable);
                    tokens.splice(begin, nextFile);
                    startLine = m_currentLine;
                    m_currentFileLines = oldCurrentFileLines;
                    m_currentFile = filename;
                    setFileMacro(defineTable, filename);
                    setLineMacro(defineTable, m_currentFileLines);
                }
                else
                    printErrorMessage(std::string("Unable to find include file ") + includeFilename);

            }
            else if (value == "#pragma")
                _parsePragma(directive);
            else if (value == "#warning")
                _parseWarning(directive, defineTable);
            else if (value == "#error")
                _parseError(directive, defineTable);
            else
            {
                HookIterator iter = m_registeredHooks.find(value);
                if (iter != m_registeredHooks.end() && m_registeredHooks[value])
                    m_registeredHooks[value](directive, defineTable);
            }
        }
        else if (begin->type == CLexer::IDENTIFIER)
        {
            begin = _expandDefine(begin, end, tokens, defineTable);
        }
        else
            ++begin;
    }
}

void CPreprocessor::callPragma(const std::string& name, const PragmaInstance& parms)
{
    PragmaIterator iter = m_registeredPragmas.find(name);
    if (iter == m_registeredPragmas.end())
    {
        printErrorMessage("Unknown pragma command.");
        return;
    }

    if (iter->second)
        iter->second(parms);
}

CLexer::TokenIterator CPreprocessor::_findToken(CLexer::TokenIterator begin, CLexer::TokenIterator end, CLexer::TokenType type)
{
    while (begin != end && begin->type != type)
        ++begin;

    return begin;
}

CLexer::TokenIterator CPreprocessor::_parseStatement(CLexer::TokenIterator begin, CLexer::TokenIterator end, CLexer::TokenList& dest)
{
    int depth = 0;
    while (begin != end)
    {
        if (begin->value == "," && depth == 0)
            return begin;
        if (begin->type == CLexer::CLOSE && depth == 0)
            return begin;
        if (begin->type == CLexer::SEMICOLON && depth == 0)
            return begin;
        dest.push_back(*begin);
        if (begin->type == CLexer::OPEN)
            depth++;
        if (begin->type == CLexer::CLOSE)
        {
            if (depth == 0)
                printErrorMessage("Mismatched braces while parsing statement.");
            depth--;
        }
        ++begin;
    }

    return begin;
}

CLexer::TokenIterator CPreprocessor::_parseDefineArguments(CLexer::TokenIterator begin, CLexer::TokenIterator end, CLexer::TokenList& tokens, std::vector<CLexer::TokenList>& args)
{
    if (begin == end || begin->value == "(")
    {
        printErrorMessage("Expected argument list.");
        return begin;
    }

    CLexer::TokenIterator beginErase = begin;
    ++begin;

    while (begin != end)
    {
        CLexer::TokenList argument;
        begin = _parseStatement(begin, end, argument);
        args.push_back(argument);

        if (begin == end)
        {
            printErrorMessage("Unexpected end of file");
            return begin;
        }

        if (begin->value == ",")
        {
            ++begin;
            if (begin == end)
            {
                printErrorMessage("Unexpected end of file.");
                return begin;
            }
            continue;
        }

        if (begin->value == ")")
        {
            ++begin;
            break;
        }
    }

    return tokens.erase(beginErase, begin);
}

CLexer::TokenIterator CPreprocessor::_expandDefine(CLexer::TokenIterator begin, CLexer::TokenIterator end, CLexer::TokenList& tokens, CPreprocessor::DefineTable& defineTable)
{
    DefineIterator defineEntry = defineTable.find(begin->value);
    if (defineEntry == defineTable.end())
        return ++begin;
    begin = tokens.erase(begin);

    if (defineEntry->second.arguments.size() == 0)
    {
        tokens.insert(begin, defineEntry->second.tokens.begin(), defineEntry->second.tokens.end());
        return begin;
    }

    // We have arguments
    std::vector<CLexer::TokenList> arguments;
    begin = _parseDefineArguments(begin, end, tokens, arguments);

    if (defineEntry->second.arguments.size() != arguments.size())
    {
        printErrorMessage("Didn't supply right number of arguments to define");
        return begin;
    }

    CLexer::TokenList tmpList(defineEntry->second.tokens.begin(), defineEntry->second.tokens.end());

    CLexer::TokenIterator tmpIter = tmpList.begin();
    while (tmpIter != tmpList.end())
    {
        ArgSet::iterator arg = defineEntry->second.arguments.find(tmpIter->value);
        if (arg == defineEntry->second.arguments.end())
        {
            ++tmpIter;
            continue;
        }

        tmpIter = tmpList.erase(tmpIter);
        tmpList.insert(begin, arguments[arg->second].begin(), arguments[arg->second].end());
    }

    tokens.insert(begin, tmpList.begin(), tmpList.end());

    return begin;
}

void CPreprocessor::_parseDefine(CPreprocessor::DefineTable& defineTable, CLexer::TokenList& tokens)
{
    advanceList(tokens);
    if (tokens.empty())
    {
        printErrorMessage("Define directive without arguments");
        return;
    }

    CLexer::Token name = *tokens.begin();
    if (name.type != CLexer::IDENTIFIER)
    {
        printErrorMessage("Defines's name was not an identifier.");
        return;
    }
    if (defineTable.find(name.value) != defineTable.end())
    {
        printErrorMessage(name.value + " already defined.");
        return;
    }
    advanceList(tokens);

    DefineEntry def;

    if (!tokens.empty())
    {
        if (tokens.begin()->type == CLexer::PREPROCESSOR && tokens.begin()->value == "#")
        {
            // macro has arguments
            advanceList(tokens);

            if (tokens.empty() || tokens.begin()->value != "(")
            {
                printErrorMessage("Expected arguments");
                return;
            }
            advanceList(tokens);

            int argCount = 0;
            while (!tokens.empty() && tokens.begin()->value != ")")
            {
                if (tokens.begin()->type != CLexer::IDENTIFIER)
                {
                    printErrorMessage("Expected identifier");
                    return;
                }

                def.arguments[tokens.begin()->value] = argCount;
                advanceList(tokens);
                if (!tokens.empty() && tokens.begin()->value == ",")
                    advanceList(tokens);
                argCount++;
            }

            if (!tokens.empty())
            {
                if (tokens.begin()->value != ")")
                {
                    printErrorMessage("Expected closing parentheses");
                    return;
                }
                advanceList(tokens);
            }
            else
            {
                printErrorMessage("Unexpected end of file");
            }
        }

        CLexer::TokenIterator iter = tokens.begin();
        while (iter != tokens.end())
            iter = _expandDefine(iter, tokens.end(), tokens, defineTable);
    }

    def.tokens = tokens;
    defineTable[name.value] = def;
}

CLexer::TokenIterator CPreprocessor::_parseIfDef(CLexer::TokenIterator begin, CLexer::TokenIterator end)
{
    int depth = 0;
    bool foundEnd = false;
    while (begin != end)
    {
        CLexer::Token token = *begin;
        if (token.type == CLexer::PREPROCESSOR)
        {
            if (token.value == "#endif" && depth == 0)
            {
                ++begin;
                foundEnd = true;
                break;
            }
            if (token.value == "#ifdef" || token.value == "#ifndef")
                depth++;
            if (token.value == "#endif" && depth > 0)
                depth--;
        }
        ++begin;
    }

    if (begin == end && !foundEnd)
    {
        printErrorMessage("Unexpected end of file");
        return begin;
    }

    return begin;
}

void CPreprocessor::_parseIf(CLexer::TokenList& directive, std::string& nameOut)
{
    advanceList(directive);
    if (directive.empty())
    {
        printErrorMessage("Expected argument.");
        return;
    }

    nameOut = directive.begin()->value;
    advanceList(directive);
    if (!directive.empty())
        printErrorMessage("Too many arguments.");
}

void CPreprocessor::_parsePragma(CLexer::TokenList& args)
{
    advanceList(args);
    if (args.empty())
    {
        printErrorMessage("Pragmas need arguments.");
        return;
    }
    std::string pragmaName = args.begin()->value;

    advanceList(args);

    std::string pragmaArgs;
    if (!args.empty())
    {
        if (args.begin()->type != CLexer::STRING)
            printErrorMessage("Pragma parameter should be a string literal");
        pragmaArgs = removeQuotes(args.begin()->value);
        advanceList(args);
    }
    if (!args.empty())
        printErrorMessage("Too many paremeters for pragma.");

    PragmaInstance pi;
    pi.name = pragmaName;
    pi.text = pragmaArgs;
    pi.currentFile = m_currentFile;
    pi.currentLine = m_currentFileLines;
    pi.rootFile    = m_rootFile;
    pi.globalLine  = m_currentLine;
    callPragma(pragmaName, pi);
}

std::string CPreprocessor::_expandMessage(DefineTable& defineTable, CLexer::TokenList& args)
{
    std::string msg;
    while (!args.empty())
    {
        DefineIterator defineEntry = defineTable.find(args.begin()->value);
        if (defineEntry != defineTable.end())
        {
            CLexer::TokenList tmpList(defineEntry->second.tokens.begin(), defineEntry->second.tokens.end());
            CLexer::TokenIterator tmpIter = tmpList.begin();

            while (tmpIter != tmpList.end())
            {
                msg += tmpIter->value;
                tmpIter = tmpList.erase(tmpIter);
            }
        }
        else if (args.begin()->type != CLexer::IGNORE)
            msg += args.begin()->value;
        args.pop_front();
    }

    return msg;
}

void CPreprocessor::_parseWarning(CLexer::TokenList& args, DefineTable& defineTable)
{
    advanceList(args);
    if (args.empty())
    {
        printErrorMessage("Warnings need messages.");
        return;
    }

    std::string msg = _expandMessage(defineTable, args);
    printWarningMessage(msg);
}

void CPreprocessor::_parseError(CLexer::TokenList& args, CPreprocessor::DefineTable& defineTable)
{
    advanceList(args);
    if (args.empty())
    {
        printErrorMessage("Errors need messages.");
        return;
    }
    std::string msg = _expandMessage(defineTable, args);
    printErrorMessage(msg);
}

