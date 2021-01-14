auto MSM5205::serialize(serializer& s) -> void {
  s(io.reset);
  s(io.width);
  s(io.scaler);
  s(io.data);
  s(io.sample);
  s(io.step);
}
