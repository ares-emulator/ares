#pragma once

#include <algorithm>
#include <nall/utility.hpp>

//class:   merge sort
//average: O(n log n)
//worst:   O(n log n)
//memory:  O(n)
//stack:   O(log n)
//stable?: yes

//note: merge sort was chosen over quick sort, because:
//* it is a stable sort
//* it lacks O(n^2) worst-case overhead
//* it usually runs faster than quick sort anyway

//note: insertion sort is generally more performant than selection sort
#define NALL_MERGE_SORT_INSERTION
//#define NALL_MERGE_SORT_SELECTION

namespace nall {

template<typename T, typename Comparator> auto sort(T list[], u32 size, const Comparator& lessthan) -> void {
  if(size <= 1) return;  //nothing to sort

  //sort smaller blocks using an O(n^2) algorithm (which for small sizes, increases performance)
  if(size < 64) {
    //insertion sort requires a copy (via move construction)
    #if defined(NALL_MERGE_SORT_INSERTION)
    for(s32 i = 1, j; i < size; i++) {
      T copy(std::move(list[i]));
      for(j = i - 1; j >= 0; j--) {
        if(!lessthan(copy, list[j])) break;
        list[j + 1] = std::move(list[j]);
      }
      list[j + 1] = std::move(copy);
    }
    //selection sort requires a swap
    #elif defined(NALL_MERGE_SORT_SELECTION)
    for(u32 i = 0; i < size; i++) {
      u32 min = i;
      for(u32 j = i + 1; j < size; j++) {
        if(lessthan(list[j], list[min])) min = j;
      }
      if(min != i) swap(list[i], list[min]);
    }
    #endif
    return;
  }

  //split list in half and recursively sort both
  u32 middle = size / 2;
  sort(list, middle, lessthan);
  sort(list + middle, size - middle, lessthan);

  //left and right are sorted here; perform merge sort
  //use placement new to avoid needing T to be default-constructable
  auto buffer = memory::allocate<T>(size);
  u32 offset = 0, left = 0, right = middle;
  while(left < middle && right < size) {
    if(!lessthan(list[right], list[left])) {
      new(buffer + offset++) T(std::move(list[left++]));
    } else {
      new(buffer + offset++) T(std::move(list[right++]));
    }
  }
  while(left < middle) new(buffer + offset++) T(std::move(list[left++]));
  while(right < size ) new(buffer + offset++) T(std::move(list[right++]));

  for(u32 i = 0; i < size; i++) {
    list[i] = std::move(buffer[i]);
    buffer[i].~T();
  }
  memory::free(buffer);
}

template<typename T> auto sort(T list[], u32 size) -> void {
  return sort(list, size, [](const T& l, const T& r) { return l < r; });
}

}
