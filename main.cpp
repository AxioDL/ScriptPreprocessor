#include <iostream>
#include "CLexer.hpp"
#include "CPreprocessor.hpp"
#include <string.h>
#include <stdlib.h>
#include <list>

void linkInstancePreprocessor(CLexer::TokenList& tokens, CPreprocessor::DefineTable& defineTable, CPreprocessor::PreprocessorState state)
{
    (void)(defineTable);
    CPreprocessor::advanceList(tokens);
    if (tokens.empty())
        return;

    CLexer::Token source = tokens.front();
    CPreprocessor::advanceList(tokens);
    if (source.type != CLexer::NUMBER)
    {
        std::cout << state.currentFile + ": Degenerate link request, invalid source" << std::endl;
        return;
    }
    if (tokens.empty())
    {
        std::cout << state.currentFile + ": Degenerate link request, missing arguments" << std::endl;
        return;
    }

    CLexer::Token target = tokens.front();
    CPreprocessor::advanceList(tokens);
    if (target.type != CLexer::NUMBER)
    {
        std::cout << state.currentFile + ": Degenerate link request, invalid target" << std::endl;
        return;
    }
    if (tokens.empty())
    {
        std::cout << state.currentFile + ": Degenerate link request, missing arguments" << std::endl;
        return;
    }

    CLexer::Token token = tokens.front();
    CPreprocessor::advanceList(tokens);

    std::vector<CLexer::Token> args;
    if (token.type == CLexer::OPEN && token.value == "(")
    {
        while (!tokens.empty())
        {
            CLexer::Token currentToken = tokens.front();
            CPreprocessor::advanceList(tokens);
            if (currentToken.type == CLexer::CLOSE)
                break;
            if (currentToken.type == CLexer::COMMA)
                continue;
            if (currentToken.type == CLexer::NUMBER)
                args.push_back(currentToken);
        }
    }

    if (args.size() != 2)
        std::cout << "Unable to link object instances, missing arguments" << std::endl;
    else
        std::cout << "Connecting object " << source.value << " to " << target.value << " with " << args.at(0).value << " " << args.at(1).value << std::endl;
}

int main()
{
    CPreprocessor preprocessor;
    preprocessor.define("_DEBUG_");
    preprocessor.registerHook("link_instance", linkInstancePreprocessor);
    if (preprocessor.preprocessFile("main.as"))
        std::cout << preprocessor.finalizedSource() << std::endl;
    else
        std::cout << "Failed to preprocess script source :(" << std::endl;
    return 0;
}

