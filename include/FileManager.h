#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H


#include <string>
#include <vector>
#include <filesystem>


namespace fs = std::filesystem;

// Stateless utility class for file I/O, hashing, and ignore-pattern matching.
// All methods are static — no instance needed.
class FileManager {
public:
    // --- Basic file operations ---

    static bool        exists(const std::string& path);
    static bool        createDirectory(const std::string& path);
    static std::string readFile(const std::string& path);
    static bool        writeFile(const std::string& path, const std::string& content);
    static std::string getCurrentDirectory();

    // Recursively list all regular files under rootDir,
    // excluding files inside excludeDir (default: ".vcs").
    // Also filters out files matching any pattern in ignorePatterns.
    static std::vector<std::string> getWorkingFiles(
        const std::string& rootDir,
        const std::string& excludeDir = ".vcs",
        const std::vector<std::string>& ignorePatterns = {});

    // --- Hashing ---

    // SHA-256 of string content → 64-char lowercase hex
    static std::string computeHash(const std::string& content);

    // SHA-256 of file contents (reads binary)
    static std::string hashFile(const std::string& path);

    // --- Path guards ---

    // True if path is inside .vcs/ (should never be staged or diffed)
    static bool isVcsPath(const std::string& path);

    // --- .vcsignore support ---

    // Read .vcsignore from workingDir and return list of patterns.
    // Returns empty vector if no .vcsignore exists.
    static std::vector<std::string> loadIgnorePatterns(const std::string& workingDir);

    // True if relPath matches any ignore pattern.
    // Supported patterns:
    //   exact/path   — exact relative path match
    //   *.ext        — any file whose name ends with .ext
    //   dir/         — any path that starts with dir/
    static bool isIgnored(const std::string& relPath,
                           const std::vector<std::string>& patterns);
};

#endif
