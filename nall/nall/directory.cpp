#include <nall/directory.hpp>
#if defined(PLATFORM_WINDOWS)
  #include <winioctl.h>

  //normally defined in ntifs.h (WDK)
  typedef struct _REPARSE_DATA_BUFFER {
    unsigned long  ReparseTag;
    unsigned short ReparseDataLength;
    unsigned short Reserved;
    union {
      struct {
        unsigned short   SubstituteNameOffset;
        unsigned short   SubstituteNameLength;
        unsigned short   PrintNameOffset;
        unsigned short   PrintNameLength;
        unsigned long    Flags;
        wchar_t          PathBuffer[1];
      } SymbolicLinkReparseBuffer;
      struct {
        unsigned short   SubstituteNameOffset;
        unsigned short   SubstituteNameLength;
        unsigned short   PrintNameOffset;
        unsigned short   PrintNameLength;
        wchar_t          PathBuffer[1];
      } MountPointReparseBuffer;
      struct {
        unsigned char    DataBuffer[1];
      } GenericReparseBuffer;
    } DUMMYUNIONNAME;
  } REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#endif

namespace nall {

#if defined(PLATFORM_WINDOWS)

NALL_HEADER_INLINE auto directory::exists(const string& pathname) -> bool {
  if(!pathname) return false;
  string name = pathname;
  name.trim("\"", "\"");
  DWORD result = GetFileAttributes(utf16_t(name));
  if(result == INVALID_FILE_ATTRIBUTES) return false;
  return (result & FILE_ATTRIBUTE_DIRECTORY);
}

NALL_HEADER_INLINE auto directory::ufolders(const string& pathname, const string& pattern) -> std::vector<string> {
  if(!pathname) {
    //special root pseudo-folder (return list of drives)
    wchar_t drives[PATH_MAX] = {0};
    GetLogicalDriveStrings(PATH_MAX, drives);
    wchar_t* p = drives;
    while(*p || *(p + 1)) {
      if(!*p) *p = ';';
      p++;
    }
    auto parts = nall::split((const char*)utf8_t(drives), ";");
    std::vector<string> out;
    out.reserve(parts.size());
    for(auto& s : parts) out.push_back(s);
    return out;
  }

  std::vector<string> list;
  string path = pathname;
  path.transform("/", "\\");
  if(!path.endsWith("\\")) path.append("\\");
  path.append("*");
  HANDLE handle;
  WIN32_FIND_DATA data;
  handle = FindFirstFile(utf16_t(path), &data);
  if(handle != INVALID_HANDLE_VALUE) {
    if(wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L"..")) {
      if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        string name = (const char*)utf8_t(data.cFileName);
        if(name.match(pattern)) list.push_back(name);
      }
    }
    while(FindNextFile(handle, &data) != false) {
      if(wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L"..")) {
        if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
          string name = (const char*)utf8_t(data.cFileName);
          if(name.match(pattern)) list.push_back(name);
        }
      }
    }
    FindClose(handle);
  }
  return list;
}

NALL_HEADER_INLINE auto directory::ufiles(const string& pathname, const string& pattern) -> std::vector<string> {
  if(!pathname) return {};

  std::vector<string> list;
  string path = pathname;
  path.transform("/", "\\");
  if(!path.endsWith("\\")) path.append("\\");
  path.append("*");
  HANDLE handle;
  WIN32_FIND_DATA data;
  handle = FindFirstFile(utf16_t(path), &data);
  if(handle != INVALID_HANDLE_VALUE) {
    if((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
      string name = (const char*)utf8_t(data.cFileName);
      if(name.match(pattern)) list.push_back(name);
    }
    while(FindNextFile(handle, &data) != false) {
      if((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        string name = (const char*)utf8_t(data.cFileName);
        if(name.match(pattern)) list.push_back(name);
      }
    }
    FindClose(handle);
  }
  return list;
}

#endif

NALL_HEADER_INLINE auto directory::resolveSymLink(const string& pathname) -> string {
  string result = pathname;
#if defined (PLATFORM_WINDOWS)
  HANDLE hFile = CreateFile(utf16_t(result.data()), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
                            NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
  if(hFile != INVALID_HANDLE_VALUE)
  {
    BYTE buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
    memset(buffer, 0, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if(DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, NULL, 0, buffer, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, NULL, NULL)) {
      REPARSE_DATA_BUFFER* reparseData = (REPARSE_DATA_BUFFER*)buffer;
      u16 substituteNameOffset = 0, substituteNameLength = 0;
      wchar_t* pathBuffer = nullptr;
      switch(reparseData->ReparseTag) {
        case IO_REPARSE_TAG_SYMLINK:
          pathBuffer = reparseData->SymbolicLinkReparseBuffer.PathBuffer;
          substituteNameOffset = reparseData->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(wchar_t);
          substituteNameLength = reparseData->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(wchar_t);
          break;
        case IO_REPARSE_TAG_MOUNT_POINT:
          pathBuffer = reparseData->MountPointReparseBuffer.PathBuffer;
          substituteNameOffset = reparseData->MountPointReparseBuffer.SubstituteNameOffset / sizeof(wchar_t);
          substituteNameLength = reparseData->MountPointReparseBuffer.SubstituteNameLength / sizeof(wchar_t);
          break;
        default:
          CloseHandle(hFile);
          return result; 
      }
      result = (slice((const char*)utf8_t(pathBuffer) + substituteNameOffset, 0, substituteNameLength)).transform("\\", "/");
      if(result.beginsWith("/??/")) result = result.slice(4);
    }
    CloseHandle(hFile);
  }
#else
  struct stat sb = {};
  if(lstat(result.data(), &sb) != -1 && S_ISLNK(sb.st_mode)) {
    char buffer[PATH_MAX];
    memset(buffer, 0, PATH_MAX);
    if(readlink(result.data(), buffer, PATH_MAX) < PATH_MAX) {
      result = string{buffer};
    }
  }
#endif
  return result;
}

}
