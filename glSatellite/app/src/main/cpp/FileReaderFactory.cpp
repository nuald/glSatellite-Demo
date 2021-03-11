#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "ndk_helper/NDKHelper.h"
#include "DebugUtils.h"
#include "FileReaderFactory.h"

using namespace std;
using namespace ndk_helper;

class SysFileReader: public IFileReader {
    ifstream fd_;
public:
    explicit SysFileReader(const string& path) :
                fd_(path) {
    }

    bool is_open() override {
        return fd_.is_open();
    }

    bool eof() override {
        return fd_.eof();
    }

    std::string getline() override {
        std::string str;
        std::getline(fd_, str);
        return str;
    }
};

class AppFileReader: public IFileReader {
    stringstream stream_;
public:
    explicit AppFileReader(const string& path) {
        std::vector < uint8_t > data;
        if (!JNIHelper::GetInstance()->ReadFile(path.c_str(), &data)) {
            if (g_developer_mode) {
                LOGI("Can not open a file: %s", path.c_str());
            }
        } else {
            std::string str(data.begin(), data.end());
            stream_ = stringstream(str);
        }
    }

    bool is_open() override {
        return !eof();
    }

    bool eof() override {
        return stream_.eof();
    }

    std::string getline() override {
        std::string str;
        std::getline(stream_, str);
        return str;
    }
};

unique_ptr<IFileReader> FileReaderFactory::Get(READER_TYPE type, const string& path) {
    switch (type) {
        case SYS:
            return unique_ptr < IFileReader > (new SysFileReader(path));
        case APP:
            return unique_ptr < IFileReader > (new AppFileReader(path));
        default:
            if (g_developer_mode) {
                LOGI("Unknown file reader: %d", type);
            }
            return nullptr;
    }
}
