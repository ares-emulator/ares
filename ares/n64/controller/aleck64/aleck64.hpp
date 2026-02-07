struct Aleck64Controls : Controller {
  Node::Input::Axis axis;

  Aleck64Controls(Node::Port);
  ~Aleck64Controls();
  auto comm(n8 send, n8 recv, n8 input[], n8 output[]) -> n2 override;
  auto read() -> n32 override;

  int playerIndex;

  enum class OutputStyle : int {
    CustomVirtualOctagon,
    CustomCircle,
    CustomMorphedOctagon,
    DiagonalCircle,
    VirtualOctagon,
    MaxCircle,
    CardinalCircle,
    MorphedOctagon,
    MaxVirtualSquare,
    MaxMorphedSquare,
  } outputStyle = OutputStyle::VirtualOctagon;

  enum class Response : int {
    Linear,
    RelaxedToAggressive,
    RelaxedToLinear,
    LinearToRelaxed,
    AggressiveToRelaxed,
    AggressiveToLinear,
    LinearToAggressive,
  } response = Response::Linear;
};
