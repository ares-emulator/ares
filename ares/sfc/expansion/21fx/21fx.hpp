struct S21FX : Expansion {
  S21FX(Node::Port);
  ~S21FX();

  auto step(u32 clocks) -> void;
  auto main() -> void;

  auto read(n24 address, n8 data) -> n8;
  auto write(n24 address, n8 data) -> void;

private:
  auto quit() -> bool;
  auto usleep(u32) -> void;
  auto readable() -> bool;
  auto writable() -> bool;
  auto read1() -> n8;
  auto write1(n8) -> void;

  n1  booted;
  n16 resetVector;
  n8  ram[122];

  nall::library link;
  std::function<void (
    std::function<bool ()>,     //quit
    std::function<void (u32)>,  //usleep
    std::function<bool ()>,     //readable
    std::function<bool ()>,     //writable
    std::function<n8 ()>,       //read
    std::function<void (n8)>    //write
  )> linkInit;
  std::function<void (std::vector<string>)> linkMain;

  std::vector<n8> snesBuffer;  //SNES -> Link
  std::vector<n8> linkBuffer;  //Link -> SNES
};
