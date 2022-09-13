#pragma once

namespace nall {

struct string_pascal {
  using type = string_pascal;

  string_pascal(const char* text = nullptr) {
    if(text && *text) {
      u32 size = strlen(text);
      _data = memory::allocate<char>(sizeof(u32) + size + 1);
      ((u32*)_data)[0] = size;
      memory::copy(_data + sizeof(u32), text, size);
      _data[sizeof(u32) + size] = 0;
    }
  }

  string_pascal(const string& text) {
    if(text.size()) {
      _data = memory::allocate<char>(sizeof(u32) + text.size() + 1);
      ((u32*)_data)[0] = text.size();
      memory::copy(_data + sizeof(u32), text.data(), text.size());
      _data[sizeof(u32) + text.size()] = 0;
    }
  }

  string_pascal(const string_pascal& source) { operator=(source); }
  string_pascal(string_pascal&& source) { operator=(std::move(source)); }

  ~string_pascal() {
    if(_data) memory::free(_data);
  }

  explicit operator bool() const { return _data; }
  operator const char*() const { return _data ? _data + sizeof(u32) : nullptr; }
  operator string() const { return _data ? string{_data + sizeof(u32)} : ""; }

  auto operator=(const string_pascal& source) -> type& {
    if(this == &source) return *this;
    if(_data) { memory::free(_data); _data = nullptr; }
    if(source._data) {
      u32 size = source.size();
      _data = memory::allocate<char>(sizeof(u32) + size);
      memory::copy(_data, source._data, sizeof(u32) + size);
    }
    return *this;
  }

  auto operator=(string_pascal&& source) -> type& {
    if(this == &source) return *this;
    if(_data) memory::free(_data);
    _data = source._data;
    source._data = nullptr;
    return *this;
  }

  auto operator==(string_view source) const -> bool {
    return size() == source.size() && memory::compare(data(), source.data(), size()) == 0;
  }

  auto operator!=(string_view source) const -> bool {
    return size() != source.size() || memory::compare(data(), source.data(), size()) != 0;
  }

  auto data() const -> char* {
    if(!_data) return nullptr;
    return _data + sizeof(u32);
  }

  auto size() const -> u32 {
    if(!_data) return 0;
    return ((u32*)_data)[0];
  }

protected:
  char* _data = nullptr;
};

}
