#pragma once

//suffix array construction via induced sorting
//many thanks to Screwtape for the thorough explanation of this algorithm
//this implementation would not be possible without his help

namespace nall {

//note that induced_sort will return an array of size+1 characters,
//where the first character is the empty suffix, equal to size

template<typename T>
inline auto induced_sort(array_view<T> data, const u32 characters = 256) -> vector<s32> {
  const u32 size = data.size();
  if(size == 0) return vector<s32>{0};  //required to avoid out-of-bounds accesses
  if(size == 1) return vector<s32>{1, 0};  //not strictly necessary; but more performant

  vector<bool> types;  //0 = S-suffix (sort before next suffix), 1 = L-suffix (sort after next suffix)
  types.resize(size + 1);

  types[size - 0] = 0;  //empty suffix is always S-suffix
  types[size - 1] = 1;  //last suffix is always L-suffix compared to empty suffix
  for(u32 n : reverse(range(size - 1))) {
    if(data[n] < data[n + 1]) {
      types[n] = 0;  //this suffix is smaller than the one after it
    } else if(data[n] > data[n + 1]) {
      types[n] = 1;  //this suffix is larger than the one after it
    } else {
      types[n] = types[n + 1];  //this suffix will be the same as the one after it
    }
  }

  //left-most S-suffix
  auto isLMS = [&](s32 n) -> bool {
    if(n == 0) return 0;  //no character to the left of the first suffix
    return !types[n] && types[n - 1];  //true if this is the start of a new S-suffix
  };

  //test if two LMS-substrings are equal
  auto isEqual = [&](s32 lhs, s32 rhs) -> bool {
    if(lhs == size || rhs == size) return false;  //no other suffix can be equal to the empty suffix

    for(u32 n = 0;; n++) {
      bool lhsLMS = isLMS(lhs + n);
      bool rhsLMS = isLMS(rhs + n);
      if(n && lhsLMS && rhsLMS) return true;  //substrings are identical
      if(lhsLMS != rhsLMS) return false;  //length mismatch: substrings cannot be identical
      if(data[lhs + n] != data[rhs + n]) return false;  //character mismatch: substrings are different
    }
  };

  //determine the sizes of each bucket: one bucket per character
  vector<u32> counts;
  counts.resize(characters);
  for(u32 n : range(size)) counts[data[n]]++;

  //bucket sorting start offsets
  vector<u32> heads;
  heads.resize(characters);

  u32 headOffset;
  auto getHeads = [&] {
    headOffset = 1;
    for(u32 n : range(characters)) {
      heads[n] = headOffset;
      headOffset += counts[n];
    }
  };

  //bucket sorting end offsets
  vector<u32> tails;
  tails.resize(characters);

  u32 tailOffset;
  auto getTails = [&] {
    tailOffset = 1;
    for(u32 n : range(characters)) {
      tailOffset += counts[n];
      tails[n] = tailOffset - 1;
    }
  };

  //inaccurate LMS bucket sort
  vector<s32> suffixes;
  suffixes.resize(size + 1, (s32)-1);

  getTails();
  for(u32 n : range(size)) {
    if(!isLMS(n)) continue;  //skip non-LMS-suffixes
    suffixes[tails[data[n]]--] = n;  //advance from the tail of the bucket
  }

  suffixes[0] = size;  //the empty suffix is always an LMS-suffix, and is the first suffix

  //sort all L-suffixes to the left of LMS-suffixes
  auto sortL = [&] {
    getHeads();
    for(u32 n : range(size + 1)) {
      if(suffixes[n] == -1) continue;  //offsets may not be known yet here ...
      auto l = suffixes[n] - 1;
      if(l < 0 || !types[l]) continue;  //skip S-suffixes
      suffixes[heads[data[l]]++] = l;  //advance from the head of the bucket
    }
  };

  auto sortS = [&] {
    getTails();
    for(u32 n : reverse(range(size + 1))) {
      auto l = suffixes[n] - 1;
      if(l < 0 || types[l]) continue;  //skip L-suffixes
      suffixes[tails[data[l]]--] = l;  //advance from the tail of the bucket
    }
  };

  sortL();
  sortS();

  //analyze data for the summary suffix array
  vector<s32> names;
  names.resize(size + 1, (s32)-1);

  u32 currentName = 0;  //keep a count to tag each unique LMS-substring with unique IDs
  auto lastLMSOffset = suffixes[0];  //location in the original data of the last checked LMS suffix
  names[lastLMSOffset] = currentName;  //the first LMS-substring is always the empty suffix entry, at position 0

  for(u32 n : range(1, size + 1)) {
    auto offset = suffixes[n];
    if(!isLMS(offset)) continue;  //only LMS suffixes are important

    //if this LMS suffix starts with a different LMS substring than the last suffix observed ...
    if(!isEqual(lastLMSOffset, offset)) currentName++;  //then it gets a new name
    lastLMSOffset = offset;  //keep track of the new most-recent LMS suffix
    names[lastLMSOffset] = currentName;  //store the LMS suffix name where the suffix appears at in the original data
  }

  vector<s32> summaryOffsets;
  vector<s32> summaryData;
  for(u32 n : range(size + 1)) {
    if(names[n] == -1) continue;
    summaryOffsets.append(n);
    summaryData.append(names[n]);
  }
  u32 summaryCharacters = currentName + 1;  //zero-indexed, so the total unique characters is currentName + 1

  //make the summary suffix array
  vector<s32> summaries;
  if(summaryData.size() == summaryCharacters) {
    //simple bucket sort when every character in summaryData appears only once
    summaries.resize(summaryData.size() + 1, (s32)-1);
    summaries[0] = summaryData.size();  //always include the empty suffix at the beginning
    for(s32 x : range(summaryData.size())) {
      s32 y = summaryData[x];
      summaries[y + 1] = x;
    }
  } else {
    //recurse until every character in summaryData is unique ...
    summaries = induced_sort<s32>({summaryData.data(), summaryData.size()}, summaryCharacters);
  }

  suffixes.fill(-1);  //reuse existing buffer for accurate sort

  //accurate LMS sort
  getTails();
  for(u32 n : reverse(range(2, summaries.size()))) {
    auto index = summaryOffsets[summaries[n]];
    suffixes[tails[data[index]]--] = index;  //advance from the tail of the bucket
  }
  suffixes[0] = size;  //always include the empty suffix at the beginning

  sortL();
  sortS();

  return suffixes;
}

}
