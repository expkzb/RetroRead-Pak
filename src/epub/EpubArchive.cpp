#include "epub/EpubArchive.h"

#include <string>

#include <zip.h>

bool EpubArchive::open(const std::string& epubPath) {
    epubPath_ = epubPath;
    return !epubPath_.empty();
}

bool EpubArchive::readTextFile(const std::string& internalPath, std::string& outText) const {
    if (epubPath_.empty()) {
        return false;
    }

    int errorCode = 0;
    zip_t* archive = zip_open(epubPath_.c_str(), ZIP_RDONLY, &errorCode);
    if (archive == nullptr) {
        return false;
    }

    zip_stat_t stat{};
    if (zip_stat(archive, internalPath.c_str(), ZIP_FL_ENC_GUESS, &stat) != 0) {
        zip_close(archive);
        return false;
    }

    zip_file_t* file = zip_fopen(archive, internalPath.c_str(), ZIP_FL_ENC_GUESS);
    if (file == nullptr) {
        zip_close(archive);
        return false;
    }

    outText.assign(static_cast<std::size_t>(stat.size), '\0');
    const zip_int64_t bytesRead = zip_fread(file, outText.data(), stat.size);

    zip_fclose(file);
    zip_close(archive);

    return bytesRead == static_cast<zip_int64_t>(stat.size);
}
