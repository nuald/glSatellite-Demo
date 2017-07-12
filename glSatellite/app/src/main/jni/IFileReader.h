#pragma once

class IFileReader {
public:
    virtual bool is_open() = 0;
    virtual bool eof() = 0;
    virtual std::string getline() = 0;
    virtual ~IFileReader() {
    }
};
