#pragma once

#include <string>

class EpubArchive {
public:
    bool open(const std::string& epubPath);
    bool readTextFile(const std::string& internalPath, std::string& outText) const;

private:
    std::string epubPath_;
};
