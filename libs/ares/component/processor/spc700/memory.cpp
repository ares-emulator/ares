inline auto SPC700::fetch() -> n8 {
  return read(PC++);
}

inline auto SPC700::load(n8 address) -> n8 {
  return read(PF << 8 | address);
}

inline auto SPC700::store(n8 address, n8 data) -> void {
  return write(PF << 8 | address, data);
}

inline auto SPC700::pull() -> n8 {
  return read(1 << 8 | ++S);
}

inline auto SPC700::push(n8 data) -> void {
  return write(1 << 8 | S--, data);
}
