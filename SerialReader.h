#ifndef SERIALREADER_H
#define SERIALREADER_H

#include <string>
#include <unordered_map>
#include <sstream>

class SerialReader {
public:
    SerialReader(const std::string& data);
    size_t GetLinePadding();
    std::string FormatRow(const std::string& title, const std::string& value);
    bool WriteRow(const std::string& row, const std::string& value);
    bool ReadRow(const std::string& row, std::string& value);
    const std::unordered_map<std::string, std::string>& GetRows();
    std::string Export() const;

private:
    std::unordered_map<std::string, std::string> data_map;
    static std::string TrimLineEndings(const std::string& str);
    static std::string RemoveBrackets(const std::string& str);
};

#endif // SERIALREADER_H
