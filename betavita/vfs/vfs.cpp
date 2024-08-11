#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>

enum FileType {
    FILE_TYPE_NONE,
    FILE_TYPE_DEVICE,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_FILE
};

struct AbstractFile {
    uint8_t type = 0;
    std::string name;
    virtual ~AbstractFile() {}
};

struct Directory : public AbstractFile {
    Directory *previousDirectory;
    Directory *currentDirectory;
    std::vector<AbstractFile *> files;

    Directory();
    void setPreviousDirectory(Directory *directory);
    void addFile(AbstractFile *file);
    void removeFile(const std::string& name);
    AbstractFile *findFile(const std::string& name);
    ~Directory();
};

Directory::Directory() { currentDirectory = this; previousDirectory = nullptr; }
void Directory::setPreviousDirectory(Directory *directory) { previousDirectory = directory; }
void Directory::addFile(AbstractFile *file) { files.push_back(file); }

void Directory::removeFile(const std::string& name) {
    if (auto it = std::find_if(files.begin(), files.end(), [name](AbstractFile *file) { return file->name == name; }); it != files.end()) {
        delete *it;
        files.erase(it);
    }
}

AbstractFile *Directory::findFile(const std::string& name) {
    if (auto it = std::find_if(files.begin(), files.end(), [name](AbstractFile *file) { return file->name == name; }); it != files.end()) {
        return *it;
    }
    return nullptr;
}

Directory::~Directory() {
    for (auto& i : files) {
        delete i;
    }
    files.clear();
}

struct MountPoint {
    std::string name;
    Directory *root;

    MountPoint();
    ~MountPoint();
};

MountPoint::MountPoint() {
    root = new Directory;
    root->name = "noname";
    root->previousDirectory = root;
    root->currentDirectory = root;
}

MountPoint::~MountPoint() {
    delete root;
}

struct VirtualFilesystem {
private:
    std::unordered_map<std::string, MountPoint *> mountpoints;

    class Scanner {
    private:
        std::string string;
    public:
        std::string scanToken();
        void setString(const std::string& string);
    } scanner;
    
    void parse(std::vector<std::string>& branches);
public:
    VirtualFilesystem();
    ~VirtualFilesystem();
    bool mount(const std::string& name);
    MountPoint *findMountpoint(const std::string& name);
    bool unmount(const std::string& name);

    bool createFile(int type, const std::string& path);
    bool removeFile(const std::string& path);
    std::string getFile(const std::string& path);

    MountPoint *createMountpoint(const std::string& name) {
        MountPoint *mp = new MountPoint;

        if (!mp)
            return nullptr;

        mp->name = name;
        return mp;
    }
};

std::string VirtualFilesystem::Scanner::scanToken() {
    int length = 0;
    const char *data;
    char lookahead;
    std::string token;

    if (string.length() == 0)
        return "";

    auto check_identifier = [&](char lookahead) { return lookahead == '_' || lookahead == '.' || isdigit(lookahead) || isalpha(lookahead) || lookahead == '$'; };

    data = string.c_str();
    do {
        lookahead = data[length];

        if (check_identifier(lookahead)) {
            do {
                token += lookahead;
                length++;
            } while (lookahead = data[length], check_identifier(lookahead));

            break;
        }

        if (lookahead == '/') {
            token = "/";
            do {
                length++;
            } while (lookahead = data[length], lookahead == '/');

            break;
        }

        if (lookahead == ':') {
            token = ":";

            do {
                length++;
            } while (lookahead = data[length], lookahead == '/');
            break;
        }

        if (lookahead != '\0') {
            printf("invalid token\n");
            length++;
        }
    } while (lookahead != '\0');

    string = string.substr(length);
    return token;
}

void VirtualFilesystem::Scanner::setString(const std::string& string) {
    this->string = string;
}

VirtualFilesystem::VirtualFilesystem() {
}

VirtualFilesystem::~VirtualFilesystem() {
    for (auto& i : mountpoints) {
        delete i.second;
    }

    mountpoints.clear();
}

bool VirtualFilesystem::mount(const std::string& name) {
    if (mountpoints.find(name) == mountpoints.end()) {
        mountpoints[name] = createMountpoint(name);
        return true;
    }
    return false;
}

MountPoint *VirtualFilesystem::findMountpoint(const std::string& name) {
    if (auto it = mountpoints.find(name); it != mountpoints.end()) {
        return it->second;
    }

    return nullptr;
}

bool VirtualFilesystem::createFile(int fileType, const std::string& path) {
    scanner.setString(path);
    std::vector<std::string> branches;
    parse(branches);

    MountPoint *mp = findMountpoint("ux0");

    Directory *root = mp->root;

    int x = 0;
    for (auto& i : branches) {
        x++;

        if (i == "..") {
            root = root->previousDirectory;
            continue;
        }

        if (i == ".")
            continue;

        if (auto file = root->findFile(i); file != nullptr) {
            if (file->type != FILE_TYPE_DIRECTORY)
                return false;
            
            root = (Directory *) file;
        } else {
            switch (fileType) {
            case FILE_TYPE_DIRECTORY:
            {
                Directory *dir = new Directory;
                dir->name = i;
                dir->type = fileType;
                dir->previousDirectory = root;
                file = dir;
                break;
            }
            case FILE_TYPE_FILE:
                file = new AbstractFile;
                file->name = i;
                file->type = fileType;
                break;
            default:
                return false;
            }

            if (x != branches.size())
                return false;

            root->addFile(file);
        }
    }
    return true;
}

bool VirtualFilesystem::removeFile(const std::string& path) {
    std::vector<std::string> branches;
    parse(branches);

    MountPoint *mp = findMountpoint("ux0");
    Directory *root = mp->root;
    std::string end;

    for (auto& i : branches) {
        if (i == "..") {
            root = root->previousDirectory;
            continue;
        }

        if (i == ".")
            continue;

        end = i;
    }

    if (end.length() != 0) {
        if (root->findFile(end) != nullptr) {
            root->removeFile(end);
            return true;
        }
    }
    return false;
}

std::string VirtualFilesystem::getFile(const std::string& path) {
    scanner.setString(path);

    std::vector<std::string> branches;
    std::vector<std::string> oldPath;
    parse(branches);

    MountPoint *mp = findMountpoint("ux0");
    Directory *root = mp->root;
    std::string currentPath;

    for (auto& i : branches) {
        if (i == "..") {
            if (oldPath.size() != 0) {
                currentPath = oldPath.back();
                oldPath.pop_back();
            }

            root = root->previousDirectory;
            if (mp->root == root)
                continue;
            continue;
        }

        if (i == ".")
            continue;

        if (auto file = root->findFile(i); file != nullptr) {
            oldPath.push_back(currentPath);
            currentPath += file->name;
            if (file->type == FILE_TYPE_DIRECTORY) {
                root = (Directory *) file;
                currentPath += "/";
            }
        } else {
            return "";
        }
    }

    return currentPath;
}

void VirtualFilesystem::parse(std::vector<std::string>& branches) {
    std::string i = scanner.scanToken();

    if (i == ".." || i == ".") {
        branches.push_back(i);
        if (scanner.scanToken() != "/") {
            printf("BAD PARSER?\n");
            return;
        }
        return parse(branches);
    }

    if (i != "")
        branches.push_back(i);

    if ((i = scanner.scanToken()) == "/")
        return parse(branches);
    else if (i != "") { printf("WTF?\n"); }
}
