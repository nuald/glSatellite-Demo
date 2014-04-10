#include "FileReaderFactory.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "NDKHelper.h"

using namespace std;
using namespace ndk_helper;

class SysFileReader: public IFileReader {
    ifstream fd_;
public:
    SysFileReader(const string& path) :
            fd_(path) {
    }

    virtual bool is_open() {
        return fd_.is_open();
    }

    virtual bool eof() {
        return fd_.eof();
    }

    virtual std::string getline() {
        std::string str;
        std::getline(fd_, str);
        return str;
    }
};

class AppFileReader: public IFileReader {
    stringstream stream_;
public:
    AppFileReader(const string& path) {
        std::vector < uint8_t > data;
        if (!JNIHelper::GetInstance()->ReadFile(path.c_str(), &data)) {
            LOGI("Can not open a file: %s", path.c_str());
        } else {
            std::string str(data.begin(), data.end());
            stream_ = stringstream(str);
        }
    }

    virtual bool is_open() {
        return !eof();
    }

    virtual bool eof() {
        return stream_.eof();
    }

    virtual std::string getline() {
        std::string str;
        std::getline(stream_, str);
        return str;
    }
};

unique_ptr<IFileReader> FileReaderFactory::Get(READER_TYPE type,
        const string& path) {

    switch (type) {
    case SYS:
        return unique_ptr < IFileReader > (new SysFileReader(path));
    case APP:
        return unique_ptr < IFileReader > (new AppFileReader(path));
    }
}
