#include <iostream>
#include "CLexer.hpp"
#include "CPreprocessor.hpp"
#include <string.h>
#include <stdlib.h>
#include <list>

void printTokenList(CLexer::TokenList& out)
{
    for (const CLexer::Token& token : out)
        std::cout << token.value;
    std::cout << std::endl;
}


void linkInstancePreprocessor(CLexer::TokenList& tokens, CPreprocessor::DefineTable& defineTable)
{
    (void)(defineTable);
    CPreprocessor::advanceList(tokens);
    if (tokens.size() == 0)
        return;

    std::cout << tokens.begin()->value << std::endl;
}

int main()
{
    CPreprocessor preprocessor;
    preprocessor.registerHook("link_instance", linkInstancePreprocessor);
    preprocessor.preprocessFile("main.as");
    std::cout << preprocessor.finalizedSource() << std::endl;
    return 0;
}

