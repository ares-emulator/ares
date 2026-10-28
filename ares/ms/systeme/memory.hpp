namespace SystemE {

struct Memory {
  auto read(n16 address) -> n8;
  auto write(n16 address, n8 data) -> void;
  auto in(n16 address) -> n8;
  auto out(n16 address, n8 data) -> void;
};

extern Memory memory;

}
