// Code based on Wenzel Jakob Filesystem
// https://github.com/wjakob/filesystem 
// used in PBRT
#pragma once

// C++17
#include <filesystem>
#include <vector>

/**
 * \brief Simple class for resolving paths on Linux/Windows/Mac OS
 *
 * This convenience class looks for a file or directory given its name
 * and a set of search paths. The implementation walks through the
 * search paths in order and stops once the file is found.
 */
class FileResolver {
public:
    typedef std::vector<std::filesystem::path>::iterator iterator;
    typedef std::vector<std::filesystem::path>::const_iterator const_iterator;

    FileResolver() {
        paths().push_back(std::filesystem::current_path());
    }

    static void append(const std::filesystem::path &path) { paths().push_back(path); }
    static std::filesystem::path resolve(const std::filesystem::path &value) {
        for (auto it: paths()) {
            auto combined = it / value;
            if (std::filesystem::exists(combined))
                return combined;
        }
        return value;
    }
    static std::vector<std::filesystem::path>&paths()
    {
        static std::vector<std::filesystem::path> ps;
        return ps;
    }
};