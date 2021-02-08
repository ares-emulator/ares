auto EpsonRTC::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(clocks);
  s(seconds);

  s(chipselect);
  s((u32&)state);
  s(mdr);
  s(offset);
  s(wait);
  s(ready);
  s(holdtick);

  s(secondlo);
  s(secondhi);
  s(batteryfailure);

  s(minutelo);
  s(minutehi);
  s(resync);

  s(hourlo);
  s(hourhi);
  s(meridian);

  s(daylo);
  s(dayhi);
  s(dayram);

  s(monthlo);
  s(monthhi);
  s(monthram);

  s(yearlo);
  s(yearhi);

  s(weekday);

  s(hold);
  s(calendar);
  s(irqflag);
  s(roundseconds);

  s(irqmask);
  s(irqduty);
  s(irqperiod);

  s(pause);
  s(stop);
  s(atime);
  s(test);
}
