#include <iostream>
#include <fstream>
#include <vector>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <occa/defines.hpp>

#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
#  include <dirent.h>
#  include <unistd.h>
#else
//#  include <windows.h>

#  include <string>
#  include <filesystem>
#  include <algorithm> // std::replace
#  include <fcntl.h>
#  include <io.h>
namespace fs = std::filesystem;
#pragma warning(disable:4996)
//#  include <io.h> // for _commit
#endif

#include <occa/utils/hash.hpp>
#include <occa/internal/io/cache.hpp>
#include <occa/internal/io/utils.hpp>
#include <occa/internal/utils/env.hpp>
#include <occa/internal/utils/lex.hpp>
#include <occa/internal/utils/misc.hpp>
#include <occa/internal/utils/string.hpp>
#include <occa/internal/utils/sys.hpp>

namespace occa {
  // Kernel Caching
  namespace kc {
    const std::string launcherSourceFile = "launcher_source.cpp";
    const std::string buildFile          = "build.json";
    const std::string launcherBuildFile  = "launcher_build.json";
#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
    const std::string binaryFile         = "binary";
    const std::string launcherBinaryFile = "launcher_binary";
#else
    const std::string binaryFile         = "binary.dll";
    const std::string launcherBinaryFile = "launcher_binary.dll";
#endif

    std::string cachedRawSourceFilename(std::string filename, bool compilingCpp) {
      const std::string basename = io::basename(filename, false);
      const std::string extension = compilingCpp ? ".cpp" : ".c";
      return basename + std::string(".raw_source") + extension;
    }
    std::string cachedSourceFilename(std::string filename) {
      const std::string basename = io::basename(filename, false);
      return basename + std::string(".source.cpp");
    }
  }

  namespace io {
    // Might not be defined in Windows
#ifndef DT_REG
    static const unsigned char DT_REG = 'r';
#endif
#ifndef DT_DIR
    static const unsigned char DT_DIR = 'd';
#endif

    std::string cachePath() {
      return env::OCCA_CACHE_DIR + "cache/";
    }

    std::string libraryPath() {
      return env::OCCA_CACHE_DIR + "libraries/";
    }

    std::string currentWorkingDirectory() {
#if (OCCA_OS == OCCA_WINDOWS_OS)
      fs::path location =  fs::current_path();
      return endWithSlash(((const std::string)location.string()));

#else
      char cwdBuff[FILENAME_MAX];
      ignoreResult(
        getcwd(cwdBuff, sizeof(cwdBuff))
      );
      return endWithSlash(std::string(cwdBuff));
#endif
    }

    libraryPathMap_t &getLibraryPathMap() {
      static libraryPathMap_t libraryPaths;
      return libraryPaths;
    }

    void endWithSlash(std::string &dir) {
      const int chars = (int) dir.size();
      if ((0 < chars) &&
          (dir[chars - 1] != '/')) {
        dir += '/';
      }
    }

    std::string endWithSlash(const std::string &dir) {
      std::string ret = dir;
      endWithSlash(ret);
      return ret;
    }

    void removeEndSlash(std::string &dir) {
      const int chars = (int) dir.size();
      if ((0 < chars) &&
          (dir[chars - 1] == '/')) {
        dir.erase(chars - 1, 1);
      }
    }

    std::string removeEndSlash(const std::string &dir) {
      std::string ret = dir;
      removeEndSlash(ret);
      return ret;
    }

    std::string convertSlashes(const std::string &filename) {
      auto temp = filename;
#if (OCCA_OS == OCCA_WINDOWS_OS)
      std::replace(temp.begin(), temp.end(),
                            '\\', '/');
#endif
      return temp;
    }

    std::string slashToSnake(const std::string &str) {
      std::string ret = str;
      const size_t chars = str.size();

      for (size_t i = 0; i < chars; ++i) {
        if (ret[i] == '/')
          ret[i] = '_';
      }

      return ret;
    }

    bool isAbsolutePath(const std::string &filename) {
#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
      return ((0 < filename.size()) &&
              (filename[0] == '/'));
#else
      fs::path pathTemp(filename);
      return pathTemp.is_absolute();
#endif
    }

    std::string getRelativePath(const std::string &filename) {
      if (startsWith(filename, "./")) {
        return filename.substr(2);
      }
      return filename;
    }

    std::string expandEnvVariables(const std::string &filename) {
      const int chars = (int) filename.size();
      if (!chars) {
        return filename;
      }

#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
      const char *c = filename.c_str();
      if ((c[0] == '~') &&
          ((c[1] == '/') || (c[1] == '\0'))) {
        if (chars == 1) {
          return env::HOME;
        }
        std::string localPath = filename.substr(2, filename.size() - 1);
        return env::HOME + sys::expandEnvVariables(localPath);
      }
#endif
      return sys::expandEnvVariables(filename);
    }

    std::string expandFilename(const std::string &filename, bool makeAbsolute) {
      const std::string cleanFilename = convertSlashes(expandEnvVariables(filename));

      std::string expFilename;
      if (startsWith(cleanFilename, "occa://")) {
        expFilename = expandOccaFilename(cleanFilename);
      } else {
        expFilename = cleanFilename;
      }

      if (makeAbsolute && !isAbsolutePath(expFilename)) {
        return env::CWD + getRelativePath(expFilename);
      }
      return expFilename;
    }

    std::string expandOccaFilename(const std::string &filename) {
      const std::string path = filename.substr(7);
      const size_t firstSlash = path.find('/');

      if ((firstSlash == 0) ||
          (firstSlash == std::string::npos)) {
        return "";
      }

      const std::string library = path.substr(0, firstSlash);
      const std::string relativePath = path.substr(firstSlash);

      libraryPathMap_t libraryPaths = getLibraryPathMap();
      libraryPathMap_t::const_iterator it = libraryPaths.find(library);
      if (it == libraryPaths.end()) {
        return "";
      }

      return it->second + relativePath;
    }

    std::string binaryName(const std::string &filename) {
#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
      return filename;
#else
      return (filename + ".dll");
#endif
    }

    std::string basename(const std::string &filename,
                         const bool keepExtension) {

      const int chars = (int) filename.size();
      const char *c = filename.c_str();

      int lastSlash = 0;

      for (int i = 0; i < chars; ++i) {
        lastSlash = (c[i] == '/') ? i : lastSlash;
      }
      if (lastSlash || (c[0] == '/')) {
        ++lastSlash;
      }

      if (keepExtension) {
        return filename.substr(lastSlash);
      }
      int extLength = (int) extension(filename).size();
      // Include the .
      if (extLength) {
        extLength += 1;
      }
      return filename.substr(lastSlash,
                             filename.size() - lastSlash - extLength);
    }

    std::string dirname(const std::string &filename) {
      std::string expFilename = removeEndSlash((const std::string)io::expandFilename(filename));
      std::string basename = io::basename(expFilename);
      return expFilename.substr(0, expFilename.size() - basename.size());
    }

    std::string extension(const std::string &filename) {
      const char *cStart = filename.c_str();
      const char *c = cStart + filename.size();

      while (cStart < c) {
        if (*c == '.') {
          break;
        }
        --c;
      }

      if (*c == '.') {
        return filename.substr(c - cStart + 1);
      }
      return "";
    }

    std::string shortname(const std::string &filename) {
      std::string expFilename = io::expandFilename(filename);

      if (!startsWith(expFilename, env::OCCA_CACHE_DIR)) {
        return filename;
      }

      const std::string &cPath = cachePath();
      return expFilename.substr(cPath.size());
    }

    std::string findInPaths(const std::string &filename, const strVector &paths) {
      if (io::isAbsolutePath(filename)) {
        return filename;
      }

      // Test paths until one exists
      // Default to a relative path if none are found
      std::string absFilename = env::CWD + filename;
      for (size_t i = 0; i < paths.size(); ++i) {
        const std::string path = paths[i];
        if (io::exists(io::endWithSlash(path) + filename)) {
          absFilename = io::endWithSlash(path) + filename;
          break;
        }
      }

      if (io::exists(absFilename)) {
        return absFilename;
      }
      return filename;
    }

    bool isFile(const std::string &filename) {
      #if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
      const std::string expFilename = io::expandFilename(filename);
      struct stat statInfo;
      return ((stat(expFilename.c_str(), &statInfo) == 0) &&
              ((statInfo.st_mode & S_IFMT) == S_IFREG));
      #else
      fs::path pathTemp(filename);
      return fs::is_regular_file(pathTemp);
      #endif
    }

    bool isDir(const std::string &filename) {
      #if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
      const std::string expFilename = io::expandFilename(filename);
      struct stat statInfo;
      return ((stat(expFilename.c_str(), &statInfo) == 0) &&
              ((statInfo.st_mode & S_IFMT) == S_IFDIR));
      #else
      fs::path pathTemp(filename);
      return fs::is_directory(pathTemp);
      #endif
    }

    strVector filesInDir(const std::string &dir, const unsigned char fileType) {
      strVector files;
      #if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
      const std::string expDir = expandFilename(dir);

      DIR *c_dir = ::opendir(expDir.c_str());
      if (!c_dir) {
        return files;
      }

      struct dirent *file;
      while ((file = ::readdir(c_dir)) != NULL) {
        const std::string filename = file->d_name;
        if ((filename == ".") ||
            (filename == "..")) {
          continue;
        }
        if (file->d_type == fileType) {
          std::string fullname = expDir + filename;
          if (fileType == DT_DIR) {
            endWithSlash(fullname);
          }
          files.push_back(fullname);
        }
      }
      ::closedir(c_dir);
      return files;
      #else
      fs::path pathTemp(dir);
      if(!fs::is_directory(pathTemp)){
        return files;
      }
      else{
        fs::directory_iterator Ditr(pathTemp);
        for(auto file : Ditr){
          if(file.is_regular_file()){
            fs::path fullpath = file.path();
            fullpath = fs::absolute(fullpath);
            files.push_back(fullpath.string());
          }
        }
        return files;
      }



      #endif
    }

    strVector directories(const std::string &dir) {
      return filesInDir(endWithSlash(dir), DT_DIR);
    }

    strVector files(const std::string &dir) {
      return filesInDir(dir, DT_REG);
    }

    char* c_read(const std::string &filename,
                 size_t *chars,
                 enums::FileType fileType) {
      std::string expFilename = io::expandFilename(filename);
      FILE *fp = fopen(
        expFilename.c_str(),
        fileType == enums::FILE_TYPE_BINARY ? "rb" : "r"
      );
      OCCA_ERROR("Failed to open [" << io::shortname(expFilename) << "]",
                 fp != NULL);

      char *buffer;
      size_t bufferSize = 0;

      if (fileType != enums::FILE_TYPE_PSEUDO) {
        #if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
        struct stat statbuf;
        stat(expFilename.c_str(), &statbuf);

        const size_t nchars = statbuf.st_size;
        #else
        fs::path pathTemp(filename);
        auto nchars = fs::file_size(pathTemp);
        #endif
        // Initialize buffer
        buffer = new char[nchars + 1];
        ::memset(buffer, 0, nchars + 1);

        // Read file
        bufferSize = fread(buffer, sizeof(char), nchars, fp);
      } else {
        // Pseudo files don't have a static size, so we need to fetch it line-by-line
        char *linePtr = NULL;
        size_t lineSize = 0;
        std::stringstream ss;
        #if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
        while (getline(&linePtr, &lineSize, fp) != -1) {
          ss << linePtr;
        }
        #else
        std::vector<char> readBuffer(1024);
        while(true){
          lineSize = fread(readBuffer.data(), 1, readBuffer.size(), fp);
          if(lineSize != 0){
            ss.write(readBuffer.data(), lineSize);
          }
          else{
            break;
          }
        }
        #endif
        ::free(linePtr);

        const std::string bufferContents = ss.str();
        bufferSize = bufferContents.size();

        buffer = new char[bufferSize + 1];
        ::memcpy(buffer, bufferContents.c_str(), bufferSize);
      }

      fclose(fp);

      // Set null terminator
      buffer[bufferSize] = '\0';

      // Set the char count
      if (chars != NULL) {
        *chars = bufferSize;
      }

      return buffer;
    }

    std::string read(const std::string &filename,
                     const enums::FileType fileType) {
      size_t chars = 0;
      const char *c = c_read(filename, &chars, fileType);
      std::string contents(c, chars);
      delete [] c;
      return contents;
    }

    void sync(const std::string &filename) {
      const std::string filedir(dirname(filename));
      int fd;

#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
      fd = open(filename.c_str(), O_RDONLY);
      fsync(fd);
      close(fd);
#else
      fd = _open(filename.c_str(), _O_RDONLY);
      _commit(fd);
      _close(fd);
#endif

#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
      fd = open(filedir.c_str(), O_RDONLY);
      fsync(fd);
      close(fd);
#else
      fd = _open(filedir.c_str(), _O_RDONLY);
      _commit(fd);
      _close(fd);
#endif
    }

    void write(const std::string &filename,
               const std::string &content) {
      std::string expFilename = io::expandFilename(filename);
      sys::mkpath(dirname(expFilename));

      FILE *fp = fopen(expFilename.c_str(), "w");
      OCCA_ERROR("Failed to open [" << io::shortname(expFilename) << "]",
                 fp != 0);

      fputs(content.c_str(), fp);
      fclose(fp);
      io::sync(expFilename);
    }

    void stageFile(
      const std::string &filename,
      const bool skipExisting,
      std::function<bool(const std::string &tempFilename)> func
    ) {
      stageFiles(
        { filename },
        skipExisting,
        [&](const strVector &tempFilenames) -> bool {
          return func(tempFilenames[0]);
        }
      );
    }

    void stageFiles(
      const strVector &filenames,
      const bool skipExisting,
      std::function<bool(const strVector &tempFilenames)> func
    ) {
      strVector tempFilenames;
      bool doNothing = skipExisting;
      for (std::string filename : filenames) {
        const std::string expFilename = io::expandFilename(filename);

        sys::mkpath(dirname(expFilename));
        tempFilenames.push_back(
          getStagedTempFilename(expFilename)
        );

        doNothing &= isFile(expFilename);
      }

      if (doNothing) {
        return;
      }

      if (!func(tempFilenames)) {
        return;
      }

      for (int i = 0; i < (int) filenames.size(); ++i) {
        moveStagedTempFile(
          tempFilenames[i],
          io::expandFilename(filenames[i])
        );
      }
    }

    std::string getStagedTempFilename(const std::string &expFilename) {
      // Generate a temporary and unique filename
      // ~/foo.cpp -> ~/1234.foo.cpp
      return (
        dirname(expFilename)
        + hash_t::random().getString()
        + "." + basename(expFilename)
      );
    }

    void moveStagedTempFile(const std::string &tempFilename,
                            const std::string &expFilename) {
      // If the temporary file was not created, nothing is needed to be done
      if (!isFile(tempFilename)) {
        return;
      }

      const int status = std::rename(
        tempFilename.c_str(),
        expFilename.c_str()
      );

      /*
        On NFS filesystems, you can not assume that if the operation
        failed, the file was not renamed. If the server does the rename
        operation and then crashes, the retransmitted RPC which will be
        processed when the server is up again causes a failure.
      */
      OCCA_ERROR(
        "Failed to rename [" << tempFilename << "] to"
        << " [" << expFilename << "]: " << strerror(errno),
        status == 0 || isFile(expFilename)
      );
    }
  }
}
