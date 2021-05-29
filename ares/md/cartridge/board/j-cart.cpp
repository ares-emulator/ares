struct JCart : Linear {
  using Linear::Linear;

  auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16 override {
    if(address >= 0x380000) {
      data.bit( 0, 5) = 0b111111;     //controller port 1
      data.bit( 6)    = jcartSelect;  //TH state
      data.bit( 8,13) = 0b111111;     //controller port 2
      data.bit(14)    = 0;            //forced low
    }
    return Linear::read(upper, lower, address, data);
  }

  auto write(n1 upper, n1 lower, n22 address, n16 data) -> void override {
    if(address >= 0x380000) {
      jcartSelect = data.bit(0);
    }
    return Linear::write(upper, lower, address, data);
  }

  auto serialize(serializer& s) -> void override {
    Linear::serialize(s);
    s(jcartSelect);
  }

  n1 jcartSelect = 0;
};
