struct Controller {
  Node::Peripheral node;
  Node::Input::Button top;
  Node::Input::Button bottom;
  Node::Input::Button one;
  Node::Input::Button two;
  Node::Input::Button three;
  Node::Input::Button four;
  Node::Input::Button five;
  Node::Input::Button six;
  Node::Input::Button seven;
  Node::Input::Button eight;
  Node::Input::Button nine;
  Node::Input::Button star;
  Node::Input::Button zero;
  Node::Input::Button pound;
  Node::Input::Button start;
  Node::Input::Button pause;
  Node::Input::Button reset;

  Controller(Node::Port parent, string name);
  virtual ~Controller() = default;

  virtual auto poll() -> void;
  virtual auto axis(u32 index, bool powered) -> s16 { return 0; }
  virtual auto keypad(n4 code) -> bool;
  virtual auto topFire() -> bool;
  virtual auto bottomFire() -> bool;

protected:
  auto appendControls() -> void;

private:
  auto pressed(Node::Input::Button input) -> bool;

  bool bottomFireLevel = false;
};

#include "port.hpp"
#include "standard/standard.hpp"
#include "trak-ball/trak-ball.hpp"
