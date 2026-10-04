auto BusQueue::reset() -> void {
  head = 0;
  count = 0;
  nextAddress = 0;
  timestamp = 0;
}

auto BusQueue::push(Transaction transaction) -> bool {
  if(count) {
    auto& previous = queue[(head + count - 1) % Capacity];
    if(previous.address == transaction.address && previous.mask == transaction.mask) {
      previous = transaction;
      return true;
    }
  }
  if(count == Capacity) return false;
  queue[(head + count++) % Capacity] = transaction;
  return true;
}

auto BusQueue::inject(u8 data) -> bool {
  return injectAt(nextAddress, data);
}

auto BusQueue::injectAt(u16 address, u8 data) -> bool {
  auto result = push({(u16)(address & 0x1fff), 0x1fff, data, timestamp, false});
  if(result) nextAddress = (address + 1) & 0x1fff;
  return result;
}

auto BusQueue::stuff(u16 address, u8 data) -> bool {
  return push({(u16)(address & 0x1fff), 0x1fff, data, timestamp, false});
}

auto BusQueue::release(u16 address, u16 mask) -> bool {
  mask &= 0x1fff;
  return push({(u16)(address & mask), mask, 0, timestamp, true});
}

auto BusQueue::consume(u16 address, u64 now, Transaction& result) -> bool {
  if(!count) return false;
  auto& next = queue[head];
  if((address & next.mask & 0x1fff) != next.address || next.timestamp > now) return false;
  result = next;
  head = (head + 1) % Capacity;
  count--;
  return true;
}

auto BusQueue::serialize(serializer& s) -> void {
  s(count);
  s(nextAddress);
  s(timestamp);
  if(s.writing()) {
    for(u32 index = 0; index < count; index++) {
      auto& transaction = queue[(head + index) % Capacity];
      s(transaction.address);
      s(transaction.mask);
      s(transaction.data);
      s(transaction.timestamp);
      s(transaction.yield);
    }
  } else {
    head = 0;
    count = min(count, Capacity);
    for(u32 index = 0; index < count; index++) {
      auto& transaction = queue[index];
      s(transaction.address);
      s(transaction.mask);
      s(transaction.data);
      s(transaction.timestamp);
      s(transaction.yield);
    }
  }
}
