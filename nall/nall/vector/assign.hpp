#pragma once

namespace nall {

template<typename T> auto vector<T>::operator=(const vector<T>& source) -> vector<T>& {
  if(this == &source) return *this;
  reset();
  _pool = memory::allocate<T>(source._size);
  _size = source._size;
  _left = 0;
  _right = 0;
  for(u64 n : range(_size)) new(_pool + n) T(source._pool[n]);
  return *this;
}

template<typename T> auto vector<T>::operator=(vector<T>&& source) -> vector<T>& {
  if(this == &source) return *this;
  reset();
  _pool = source._pool;
  _size = source._size;
  _left = source._left;
  _right = source._right;
  source._pool = nullptr;
  source._size = 0;
  source._left = 0;
  source._right = 0;
  return *this;
}

}
