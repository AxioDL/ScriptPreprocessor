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

int main()
{
    CPreprocessor preprocessor;
    preprocessor.preprocessFile("main.as");
    std::cout << preprocessor.finalizedSource() << std::endl;
    return 0;
}

