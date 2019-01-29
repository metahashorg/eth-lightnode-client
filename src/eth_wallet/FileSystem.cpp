#include "FileSystem.h"

#include <iostream>
#include <set>
#include <fstream>
#include <experimental/filesystem>

#include "check.h"

#include "stringUtils.h"

namespace fs = std::experimental::filesystem;

void createDirectories(const std::string& folder) {
    fs::create_directories(folder);
}

std::string getBasename(const std::string &path) {
    const auto p = fs::path(path);
    return p.filename();
}

std::string getFullPath(const std::string &fileName, const std::string &folder) {
    return fs::path(folder) / fs::path(fileName);
}

std::vector<std::string> getAllFilesRelativeInDir(const std::string &dirPath) {
    std::vector<std::string> listOfFiles;
    if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
        fs::recursive_directory_iterator iter(dirPath);
        fs::recursive_directory_iterator end;
        
        while (iter != end) {
            if (fs::is_directory(iter->path())) {
                iter.disable_recursion_pending();
            } else {
                std::error_code ec;
                std::string r = fs::absolute(iter->path().string());
                r = r.substr(fs::absolute(dirPath).string().size());
                listOfFiles.push_back(r);
            }
            
            std::error_code ec;
            iter.increment(ec);
            CHECK(!ec, "Error While Accessing : " + iter->path().string() + " :: " + ec.message());
        }
    }
    return listOfFiles;
}

void saveToFile(const std::string& folder, const std::string& fileName, const std::string& data) {
    const std::string fullPath = getFullPath(fileName, folder);
    
    saveToFile(fullPath, data);
}

void saveToFile(const std::string& fullPath, const std::string& data) {
    fs::create_directories(fs::path(fullPath).parent_path());
       
    std::ofstream file(fullPath);
    file << data;
    file.close();
}

void removeFile(const std::string& folder, const std::string& fileName) {
    const std::string fullPath = getFullPath(fileName, folder);
       
    if (fs::exists(fullPath)) {
        fs::remove(fullPath);
    }
}

std::string loadFile(const std::string& folder, const std::string& fileName) {
    const std::string fullPath = getFullPath(fileName, folder);
    
    return loadFile(fullPath);
}

std::string loadFile(const std::string& fullPath) {
    std::ifstream file(fullPath);
    std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
    return content;
}

bool isExist(const std::string& fileName) {
    return std::experimental::filesystem::exists(fileName);
}
