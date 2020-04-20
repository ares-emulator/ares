auto MSM5205::serialize(serializer& s) -> void {
  s.integer(io.reset);
  s.integer(io.width);
  s.integer(io.scaler);
  s.integer(io.data);
  s.integer(io.sample);
  s.integer(io.step);
}
