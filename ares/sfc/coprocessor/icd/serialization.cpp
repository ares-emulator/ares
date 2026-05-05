auto ICD::serialize(serializer& s) -> void {
  Thread::serialize(s);
  GameBoy::system.serialize(s, scheduler.getSynchronize());

  s(joypID);
  s(pulseLock);
  s(strobeLock);
  s(packetLock);
  s(packetOffset);
  s(packetReady);
  s(bitData);
  s(bitOffset);
  s(previousP15);

  s(output);
  s(readBank);
  s(readAddress);
  s(writeBank);

  s(r6003);
  s(r6004);
  s(r6005);
  s(r6006);
  s(r6007);
  s(r7000);
  s(mltReq);

  s(hcounter);
  s(vcounter);
}
