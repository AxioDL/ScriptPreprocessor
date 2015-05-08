#ifndef CLINENUMBERTRANSLATOR_HPP
#define CLINENUMBERTRANSLATOR_HPP

#include <string>
#include <vector>

class CLineTranslator
{
public:
    struct Table
    {
        struct Entry
        {
            std::string file;
            unsigned int startLine;
            unsigned int offset;
        };

        std::vector<Entry> lines;

        Entry& search(unsigned int line);

        void addLineRange(const std::string& file, unsigned int startLine, unsigned int offset);
    };

    CLineTranslator();
    std::string resolveOriginalFile(unsigned int lineNumber);
    unsigned int resolveOriginalLine(unsigned int lineNumber);
    inline void setTable(Table table) { m_table = table; }
    inline Table& table() { return m_table; }
    void reset();
private:
    Table m_table;
};

#endif // CLINENUMBERTRANSLATOR_HPP
