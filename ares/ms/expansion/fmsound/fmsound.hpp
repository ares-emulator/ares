struct FMSoundUnit : Expansion {
  FMSoundUnit(Node::Port);
  ~FMSoundUnit();

  auto serialize(serializer& s) -> void override;
};
