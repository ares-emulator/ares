struct Aleck64Controls : Controller {
  Aleck64Controls(Node::Port);
  ~Aleck64Controls();
  auto comm(n8 send, n8 recv, n8 input[], n8 output[]) -> n2 override;
  auto read() -> n32 override;

  int playerIndex;
};

