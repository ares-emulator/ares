#pragma once

//priority queue implementation using binary min-heap array:
//O(1)     find
//O(log n) append
//O(log n) remove

#include <nall/function.hpp>
#include <nall/serializer.hpp>

namespace nall {

template<typename T> struct priority_queue;

template<typename T, u32 Size>
struct priority_queue<T[Size]> {
  explicit operator bool() const {
    return size != 0;
  }

  auto reset() -> void {
    clock = 0;
    size = 0;
  }

  template<typename F>
  auto step(u32 clocks, const F& callback) -> void {
    clock += clocks;
    while(size && ge(clock, heap[0].clock)) {
      callback(remove());
    }
  }

  auto append(u32 clock, T event) -> bool {
    if(size >= Size) return false;

    u32 child = size++;
    clock += this->clock;

    while(child) {
      u32 parent = (child - 1) >> 1;
      if(ge(clock, heap[parent].clock)) break;

      heap[child].clock = heap[parent].clock;
      heap[child].event = heap[parent].event;
      child = parent;
    }

    heap[child].clock = clock;
    heap[child].event = event;
    return true;
  }

  auto remove() -> T {
    T event = heap[0].event;
    u32 parent = 0;
    u32 clock = heap[--size].clock;

    while(true) {
      u32 child = (parent << 1) + 1;
      if(child >= size) break;

      if(child + 1 < size && ge(heap[child].clock, heap[child + 1].clock)) child++;
      if(ge(heap[child].clock, clock)) break;

      heap[parent].clock = heap[child].clock;
      heap[parent].event = heap[child].event;
      parent = child;
    }

    heap[parent].clock = clock;
    heap[parent].event = heap[size].event;
    return event;
  }

  auto serialize(serializer& s) -> void {
    s(clock);
    s(size);
    for(auto& entry : heap) {
      s(entry.clock);
      s(entry.event);
    }
  }

private:
  //returns true if x is greater than or equal to y
  auto ge(u32 x, u32 y) -> bool {
    return x - y < 0x7fffffff;
  }

  u32 clock = 0;
  u32 size = 0;
  struct Entry {
    u32 clock;
    T   event;
  } heap[Size];
};

}
