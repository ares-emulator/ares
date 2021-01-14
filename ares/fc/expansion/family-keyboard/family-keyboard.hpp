struct FamilyKeyboard : Expansion {
  struct Key {
    Node::Input::Button f1, f2, f3, f4, f5, f6, f7, f8;
    Node::Input::Button one, two, three, four, five, six, seven, eight, nine, zero, minus, power, yen, stop;
    Node::Input::Button escape, q, w, e, r, t, y, u, i, o, p, at, lbrace, enter;
    Node::Input::Button control, a, s, d, f, g, h, j, k, l, semicolon, colon, rbrace, kana;
    Node::Input::Button lshift, z, x, c, v, b, n, m, comma, period, slash, underscore, rshift;
    Node::Input::Button graph, spacebar;
    Node::Input::Button home, insert, backspace;
    Node::Input::Button up, down, left, right;
  } key;

  FamilyKeyboard(Node::Port);
  auto read1() -> n1 override;
  auto read2() -> n5 override;
  auto write(n3 data) -> void override;

private:
  n3 latch;
  n1 column;
  n4 row;
};
