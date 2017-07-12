#pragma once

#include <string>

#include "IFileReader.h"

enum READER_TYPE {
    SYS, APP
};

class FileReaderFactory {
public:
    static std::unique_ptr<IFileReader> Get(READER_TYPE type,
        const std::string& path);
};
