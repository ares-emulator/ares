auto S3511A::serialize(serializer& s) -> void {
  s(std::span<n8>{data, size});

  s(cs);
  s(sio);
  s(sck);
  s(inBuffer);
  s(outBuffer);
  s(shift);
  s(index);
  s(rwSelect);
  s(regSelect);
  s(cmdLatched);
  s(counter);
}
