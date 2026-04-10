#pragma once

#include "core/BookTypes.h"

class FileSystem;

class SettingsStore {
public:
    bool load(FileSystem& fileSystem, ReaderSettings& settings) const;
    bool save(FileSystem& fileSystem, const ReaderSettings& settings) const;
};
