#ifndef FILE_MANAGER_H
#include <string>

// Manages the .vcs/ repository directory: initialization and path lookups.
// All path strings are absolute so other classes don't need to know the layout.
class RepositoryManager {
private:
    std::string repoRoot;    // project root (working directory)
    std::string vcsDir;      // .vcs/
    std::string stagingDir;  // .vcs/staging/
    std::string commitsDir;  // .vcs/commits/
    std::string objectsDir;  // .vcs/objects/
    std::string branchesDir; // .vcs/branches/
    std::string headFile;    // .vcs/HEAD   (current commit ID)
    std::string branchFile;  // .vcs/BRANCH (current branch name)
    std::string indexFile;   // .vcs/staging/index

    void buildPaths();

public:
    explicit RepositoryManager(const std::string& rootPath);

    // Create the full .vcs/ directory structure and default "main" branch
    bool initRepository();

    // True if .vcs/ directory exists in repoRoot
    bool isInitialized() const;

    // Path getters
    std::string getRepoRoot()    const;
    std::string getVcsDir()      const;
    std::string getStagingDir()  const;
    std::string getCommitsDir()  const;
    std::string getObjectsDir()  const;
    std::string getBranchesDir() const;
    std::string getHeadFile()    const;
    std::string getBranchFile()  const;
    std::string getIndexFile()   const;
};

#endif
