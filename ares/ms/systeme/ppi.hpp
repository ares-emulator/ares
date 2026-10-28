struct PPI : I8255 {
  string game;
  n8 control;
  u16 x, y, startX, startY, latchX, latchY;
  n4 switches;

  auto power() -> void;
  auto readPortA() -> n8 override;
  auto readPortC() -> n8 override;
  auto writePortC(n8 data) -> void override;
  auto serialize(serializer&) -> void;
};

extern PPI ppi;
