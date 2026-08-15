#include "PythonBridge.h"
#include "FileManager.h"
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <array>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

// --- Find a working Python 3 executable ---

std::string PythonBridge::findPython() {
    // Try python3 first (Linux/macOS), then python (Windows/some distros)
    for (const char* cmd : {"python2", "python"}) {
        std::string check = std::string(cmd) + " --version 2>/dev/null";
        int ret = std::system(check.c_str());
        if (ret == 0) return cmd;
    }
    return "";
}

// --- JSON helpers (hand-written to avoid external deps) ---

// Escape special JSON characters in a string
static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;
        }
    }
    return out;
}

// Serialize RepoData → JSON string
std::string PythonBridge::toJson(const RepoData& data) {
    std::ostringstream j;
    j << "{\n";

    // Repository metadata
    j << "  \"active_branch\": \"" << jsonEscape(data.activeBranch) << "\",\n";
    j << "  \"branches\": [";
    for (size_t i = 0; i < data.branches.size(); ++i) {
        if (i) j << ", ";
        j << "\"" << jsonEscape(data.branches[i]) << "\"";
    }
    j << "],\n";
    j << "  \"total_objects\": " << data.totalObjects << ",\n";
    j << "  \"repo_size_kb\": " << std::fixed << std::setprecision(2) << data.repoSizeKB << ",\n";
    j << "  \"staged_count\": " << data.stagedCount << ",\n";
    j << "  \"modified_count\": " << data.modifiedCount << ",\n";
    j << "  \"untracked_count\": " << data.untrackedCount << ",\n";

    // HEAD file manifest
    j << "  \"head_files\": {";
    {
        bool first = true;
        for (const auto& [path, hash] : data.headFiles) {
            if (!first) j << ", ";
            j << "\"" << jsonEscape(path) << "\": \"" << jsonEscape(hash) << "\"";
            first = false;
        }
    }
    j << "},\n";

    // Commits array
    j << "  \"commits\": [\n";
    for (size_t i = 0; i < data.commits.size(); ++i) {
        const Commit& c = data.commits[i];
        j << "    {\n";
        j << "      \"id\": \"" << jsonEscape(c.id) << "\",\n";
        j << "      \"message\": \"" << jsonEscape(c.message) << "\",\n";
        j << "      \"timestamp\": \"" << jsonEscape(c.timestamp) << "\",\n";
        j << "      \"parent_id\": \"" << jsonEscape(c.parentId) << "\",\n";
        j << "      \"files\": {";
        {
            bool first = true;
            for (const auto& [path, hash] : c.files) {
                if (!first) j << ", ";
                j << "\"" << jsonEscape(path) << "\": \"" << jsonEscape(hash) << "\"";
                first = false;
            }
        }
        j << "}\n";
        j << "    }";
        if (i + 1 < data.commits.size()) j << ",";
        j << "\n";
    }
    j << "  ]\n";

    j << "}\n";
    return j.str();
}

// --- Invoke Python analytics ---

int PythonBridge::invoke(const std::string& repoRoot,
                          const RepoData& data,
                          const std::vector<std::string>& flags) {
    std::string python = findPython();
    if (python.empty()) {
        std::cerr << "Error: Python 3 was not found.\n"
                  << "Install Python 3 to use repository analytics." << std::endl;
        return 1;
    }

    // Locate the analytics.py script relative to the VCS binary
    std::string scriptDir = repoRoot + "/python";
    std::string scriptPath = scriptDir + "/analytics.py";

    // Also try relative to the executable location if not found at repo root
    if (!FileManager::exists(scriptPath)) {
        // Try the directory where the vcs binary is located
        std::string exeDir;
        try {
            exeDir = fs::read_symlink("/proc/self/exe").parent_path().string();
        } catch (...) {}
        if (!exeDir.empty()) {
            scriptPath = exeDir + "/python/analytics.py";
        }
    }

    if (!FileManager::exists(scriptPath)) {
        std::cerr << "Error: Python analytics script not found.\n"
                  << "Expected at: " << repoRoot << "/python/analytics.py" << std::endl;
        return 1;
    }

    // Serialize data to a temp JSON file
    std::string tmpFile = repoRoot + "/.vcs/analytics_input.json";
    std::string json = toJson(data);
    if (!FileManager::writeFile(tmpFile, json)) {
        std::cerr << "Error: Could not write analytics input." << std::endl;
        return 1;
    }

    // Build the command
    std::string cmd = python + " \"" + scriptPath + "\" \"" + tmpFile + "\"";
    for (const auto& f : flags) cmd += " " + f;

    // Execute
    int ret = std::system(cmd.c_str());

    // Clean up temp file
    try { fs::remove(tmpFile); } catch (...) {}

    return ret;
}
