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
  auto read() -> n8;
  auto write(n8) -> void;

  n1  booted;
  n16 resetVector;
  n8  ram[122];

  nall::library link;
  function<void (
    function<bool ()>,     //quit
    function<void (u32)>,  //usleep
    function<bool ()>,     //readable
    function<bool ()>,     //writable
    function<n8 ()>,       //read
    function<void (n8)>    //write
  )> linkInit;
  function<void (vector<string>)> linkMain;

  vector<n8> snesBuffer;  //SNES -> Link
  vector<n8> linkBuffer;  //Link -> SNES
};
