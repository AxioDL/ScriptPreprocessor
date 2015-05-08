#include "CLineTranslator.hpp"


CLineTranslator::Table::Entry& CLineTranslator::Table::search(unsigned int line)
{
    for (size_t i = 1; i < lines.size(); ++i)
    {
        if (line < lines[i].startLine)
            return lines[i-1];
    }

    return lines[lines.size() - 1];
}

void CLineTranslator::Table::addLineRange(const std::string& file, unsigned int startLine, unsigned int offset)
{
    Entry e;
    e.file = file;
    e.startLine  = startLine;
    e.offset = offset;
    lines.push_back(e);
}


CLineTranslator::CLineTranslator()
{
}

std::string CLineTranslator::resolveOriginalFile(unsigned int lineNumber)
{
    return m_table.search(lineNumber).file;
}

unsigned int CLineTranslator::resolveOriginalLine(unsigned int lineNumber)
{
    return lineNumber - m_table.search(lineNumber).offset;
}

void CLineTranslator::reset()
{
    m_table.lines.clear();
}
