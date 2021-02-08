struct Satellaview : Expansion {
  Satellaview(Node::Port);
  ~Satellaview();

  auto read(n24 address, n8 data) -> n8;
  auto write(n24 address, n8 data) -> void;

private:
  struct {
    n8 r2188, r2189, r218a, r218b;
    n8 r218c, r218d, r218e, r218f;
    n8 r2190, r2191, r2192, r2193;
    n8 r2194, r2195, r2196, r2197;
    n8 r2198, r2199, r219a, r219b;
    n8 r219c, r219d, r219e, r219f;

    n8 rtcCounter;
    n8 rtcHour;
    n8 rtcMinute;
    n8 rtcSecond;
  } regs;
};
