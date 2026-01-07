#include <nall/directory.hpp>

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
  HANDLE hFile = CreateFile(utf16_t(result.data()), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if(hFile != INVALID_HANDLE_VALUE)
  {
      wchar_t buffer[MAX_PATH];
      memset(buffer, 0, MAX_PATH * sizeof(wchar_t));
      if(GetFinalPathNameByHandle(hFile, buffer, MAX_PATH, 0) < MAX_PATH) {
        result = slice((const char*)utf8_t(buffer), 4, wcslen(buffer) - 4); //remove "\\?\" prefix
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
