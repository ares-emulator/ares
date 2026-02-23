const u32 SharpRTC::daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

auto SharpRTC::tickSecond() -> void {
  if(++second < 60) return;
  second = 0;
  tickMinute();
}

auto SharpRTC::tickMinute() -> void {
  if(++minute < 60) return;
  minute = 0;
  tickHour();
}

auto SharpRTC::tickHour() -> void {
  if(++hour < 24) return;
  hour = 0;
  tickDay();
}

auto SharpRTC::tickDay() -> void {
  u32 days = daysInMonth[(month - 1) % 12];

  //add one day in February for leap years
  if(month == 2) {
         if(year % 400 == 0) days++;
    else if(year % 100 == 0);
    else if(year %   4 == 0) days++;
  }

  if(day++ < days) return;
  day = 1;
  tickMonth();
}

auto SharpRTC::tickMonth() -> void {
  if(month++ < 12) return;
  month = 1;
  tickYear();
}

auto SharpRTC::tickYear() -> void {
  year++;
  year = (n12)year;
}

//returns day of week for specified date
//eg 0 = Sunday, 1 = Monday, ... 6 = Saturday
//usage: calculate_weekday(2008, 1, 1) returns weekday of January 1st, 2008
auto SharpRTC::calculateWeekday(u32 year, u32 month, u32 day) -> u32 {
  u32 y = 1000, m = 1;  //SharpRTC epoch is 1000-01-01
  u32 sum = 0;          //number of days passed since epoch

  year = max(1000, year);
  month = max(1, min(12, month));
  day = max(1, min(31, day));

  while(y < year) {
    bool leapyear = false;
    if(y % 4 == 0) {
      leapyear = true;
      if(y % 100 == 0 && y % 400 != 0) leapyear = false;
    }
    sum += 365 + leapyear;
    y++;
  }

  while(m < month) {
    u32 days = daysInMonth[(m - 1) % 12];
    bool leapyearmonth = false;
    if(days == 28) {
      if(y % 4 == 0) {
        leapyearmonth = true;
        if(y % 100 == 0 && y % 400 != 0) leapyearmonth = false;
      }
    }
    sum += days + leapyearmonth;
    m++;
  }

  sum += day - 1;
  return (sum + 3) % 7;  //1000-01-01 was a Wednesday
}
