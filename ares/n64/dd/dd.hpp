//Disk Drive

struct DD {
  Node::Component node;
  Memory iplrom;
  Memory c2s;
  Memory ds;
  Memory ms;

  //dd.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto power(bool reset) -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, uint32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern DD dd;
