auto SPC7110::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(ram);

  s(r4801);
  s(r4802);
  s(r4803);
  s(r4804);
  s(r4805);
  s(r4806);
  s(r4807);
  s(r4809);
  s(r480a);
  s(r480b);
  s(r480c);

  s(dcuPending);
  s(dcuMode);
  s(dcuAddress);
  s(dcuOffset);
  s(dcuTile);
  if(decompressor) s(*decompressor);

  s(r4810);
  s(r4811);
  s(r4812);
  s(r4813);
  s(r4814);
  s(r4815);
  s(r4816);
  s(r4817);
  s(r4818);
  s(r481a);

  s(r4820);
  s(r4821);
  s(r4822);
  s(r4823);
  s(r4824);
  s(r4825);
  s(r4826);
  s(r4827);
  s(r4828);
  s(r4829);
  s(r482a);
  s(r482b);
  s(r482c);
  s(r482d);
  s(r482e);
  s(r482f);

  s(mulPending);
  s(divPending);

  s(r4830);
  s(r4831);
  s(r4832);
  s(r4833);
  s(r4834);
}
