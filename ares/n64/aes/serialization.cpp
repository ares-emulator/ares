auto AES::serialize(serializer& s) -> void {
  s(D);
  s(mIV);
  s(mKey);
}