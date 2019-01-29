#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <unordered_map>
#include <string>
#include <vector>

struct FileInfo;

std::string getBasename(const std::string &path);

void createDirectories(const std::string& folder);

std::string getFullPath(const std::string &fileName, const std::string &folder);

inline std::string getFullPath2(const std::string &folder, const std::string &fileName) {
    return getFullPath(fileName, folder);
}

std::vector<std::string> getAllFilesRelativeInDir(const std::string &dirPath);

void saveToFile(const std::string &folder, const std::string &fileName, const std::string &data);

void saveToFile(const std::string &fullPath, const std::string &data);

void removeFile(const std::string &folder, const std::string &fileName);

std::string loadFile(const std::string &folder, const std::string &fileName);

std::string loadFile(const std::string &fullPath);

bool isExist(const std::string &fileName);

inline std::string makePath(const std::string &arg) {
    return arg;
}

template<typename... Args>
inline std::string makePath(const std::string &arg, const Args& ...args) {
    return getFullPath2(arg, makePath(args...));
}

#endif // FILESYSTEM_H_
