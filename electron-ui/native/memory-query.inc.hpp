// Included inside MemoryTools: bounded native candidate sets over immutable snapshots.
  struct Query {
    uint32_t id, before, after, range, source = 0, cursor = 0, total = 0;
    std::string owner, type, endian, filter, state = "running", error;
    MemoryService::Region* region;
    uint32_t offset, length, width;
    double expected;
    std::vector<uint32_t> offsets;
  };
  std::map<uint32_t, Query> queries;
  uint32_t nextQuery = 0;
  static constexpr uint32_t candidateLimit = 262144, queryLimit = 8;
  static bool matches(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b,
                      size_t offset, unsigned width, const std::string& type,
                      bool little, const std::string& filter, double expected) {
    const bool changed = std::memcmp(a.data() + offset, b.data() + offset, width) != 0;
    if(filter == "changed") return changed;
    if(filter == "unchanged") return !changed;
    const double old = value(a, offset, type, little), now = value(b, offset, type, little);
    if(filter == "equals") return !std::isnan(now) && now == expected;
    if(std::isnan(old) || std::isnan(now)) return false;
    if(filter == "increased") return now > old;
    if(filter == "decreased") return now < old;
    return now - old == expected;
  }
  static std::string matchJson(const Capture& c, const Capture& d, uint32_t range,
                               uint32_t offset, unsigned width, const std::string& type, bool little) {
    const auto& a = c.samples.front().bytes[range];
    const auto& b = d.samples.front().bytes[range];
    return "{\"offset\":" + std::to_string(c.ranges[range].offset + offset)
      + ",\"old\":" + decimal(value(a, offset, type, little)) + ",\"new\":" + decimal(value(b, offset, type, little))
      + ",\"old_hex\":" + controlQuote(hex(a, offset, width)) + ",\"new_hex\":" + controlQuote(hex(b, offset, width)) + "}";
  }
  std::string compatible(uint32_t before, uint32_t after, uint32_t range, const std::string& owner) {
    auto a = captures.find(before), b = captures.find(after);
    if(a == captures.end() || b == captures.end() || a->second.owner != owner || b->second.owner != owner) return "stale_memory_handle";
    const auto& c = a->second; const auto& d = b->second;
    if(c.recording || d.recording || c.samples.empty() || d.samples.empty()) return "immutable_snapshot_required";
    if(range >= c.ranges.size() || range >= d.ranges.size() || c.ranges[range].region != d.ranges[range].region
       || c.ranges[range].offset != d.ranges[range].offset || c.ranges[range].length != d.ranges[range].length) return "incompatible_snapshots";
    return {};
  }
  std::string queryStatus(const Query& q) {
    return "{\"handle\":" + std::to_string(q.id) + ",\"epoch\":" + std::to_string(epoch)
      + ",\"state\":" + controlQuote(q.state) + ",\"scanned\":" + std::to_string(q.cursor)
      + ",\"total\":" + std::to_string(q.total) + ",\"candidate_count\":" + std::to_string(q.offsets.size())
      + ",\"candidate_limit\":" + std::to_string(candidateLimit) + ",\"error\":" + (q.error.empty() ? "null" : controlQuote(q.error)) + "}";
  }
  ControlResult query(const std::string& method, const std::vector<std::string>& a, const std::string& owner) {
    uint32_t id;
    if(method == "memory.query.start") {
      uint32_t before, after, range, source;
      if(a.size() != 8 || !MemoryService::number(a[0], before) || !MemoryService::number(a[1], after)
        || !MemoryService::number(a[2], range) || !MemoryService::number(a[7], source)) return {"invalid_request", {}};
      auto error = compatible(before, after, range, owner); if(!error.empty()) return {error, {}};
      unsigned width = a[3] == "u8" ? 1 : a[3] == "u16" ? 2 : (a[3] == "u32" || a[3] == "f32") ? 4 : 0;
      if(!width || (a[4] != "little" && a[4] != "big") || (a[5] != "changed" && a[5] != "unchanged"
        && a[5] != "increased" && a[5] != "decreased" && a[5] != "delta" && a[5] != "equals")) return {"invalid_request", {}};
      double expected; try {size_t end; expected = std::stod(a[6], &end); if(end != a[6].size() || !std::isfinite(expected)) return {"invalid_request", {}};} catch(...) {return {"invalid_request", {}};}
      const auto& r = captures.at(before).ranges[range];
      if(r.offset % width || r.length % width) return {"unaligned_range", {}};
      if(source) {
        auto s = queries.find(source);
        if(s == queries.end() || s->second.owner != owner) return {"stale_query_handle", {}};
        const auto& q = s->second;
        if(q.state != "done") return {"candidate_set_not_complete", {}};
        if(q.region != r.region || q.offset != r.offset || q.length != r.length || q.type != a[3] || q.endian != a[4]) return {"incompatible_candidates", {}};
      }
      if(queries.size() >= queryLimit || nextQuery == UINT32_MAX) return {"query_budget_exhausted", {}};
      Query q; q.id = ++nextQuery; q.before = before; q.after = after; q.range = range; q.source = source;
      q.owner = owner; q.region = r.region; q.offset = r.offset; q.length = r.length; q.width = width;
      q.type = a[3]; q.endian = a[4]; q.filter = a[5]; q.expected = expected;
      // Fixed reservation: 1 MiB per query, 8 MiB globally. No growth/reallocation.
      q.total = source ? queries.at(source).offsets.size() : q.length / q.width;
      q.offsets.reserve(candidateLimit);
      auto [it, inserted] = queries.emplace(q.id, std::move(q));
      return {{}, queryStatus(it->second)};
    }
    if(a.empty() || !MemoryService::number(a[0], id)) return {"invalid_request", {}};
    auto found = queries.find(id);
    if(found == queries.end() || found->second.owner != owner) return {"stale_query_handle", {}};
    auto& q = found->second;
    if(method == "memory.query.release") {
      if(a.size() != 1) return {"invalid_request", {}};
      for(const auto& [other, dependent] : queries) if(dependent.source == id && dependent.state == "running") return {"query_in_use", {}};
      // Completed descendants no longer depend on the input candidate set.
      for(auto& [other, dependent] : queries) if(dependent.source == id) dependent.source = 0;
      queries.erase(found); return {{}, "{\"released\":true}"};
    }
    if(method != "memory.query.advance" && method != "memory.query.page") return {"unavailable", {}};
    if((method == "memory.query.advance" && a.size() != 1) || (method == "memory.query.page" && a.size() != 3)) return {"invalid_request", {}};
    auto error = compatible(q.before, q.after, q.range, owner);
    if(!error.empty()) return {error, {}};
    auto& c = captures.at(q.before); auto& d = captures.at(q.after);
    if(method == "memory.query.advance") {
      if(q.state != "running") return {{}, queryStatus(q)};
      const std::vector<uint32_t>* source = nullptr;
      if(q.source) {auto s = queries.find(q.source); if(s == queries.end()) return {"stale_query_handle", {}}; source = &s->second.offsets;}
      const size_t total = source ? source->size() : q.length / q.width;
      const size_t end = std::min(total, size_t(q.cursor) + 65536 / q.width);
      for(; q.cursor < end; ++q.cursor) {
        uint32_t offset = source ? (*source)[q.cursor] : q.cursor * q.width;
        if(matches(c.samples.front().bytes[q.range], d.samples.front().bytes[q.range], offset, q.width, q.type, q.endian == "little", q.filter, q.expected)) {
          if(q.offsets.size() == candidateLimit) {q.state = "failed"; q.error = "candidate_budget_exhausted"; break;}
          q.offsets.push_back(offset);
        }
      }
      if(q.cursor == total) q.state = "done";
      return {{}, queryStatus(q)};
    }
    uint32_t cursor, limit;
    if(!MemoryService::number(a[1], cursor) || !MemoryService::number(a[2], limit) || !limit || limit > 256) return {"invalid_request", {}};
    if(q.state != "done") return {"candidate_set_not_complete", {}};
    if(cursor > q.offsets.size()) return {"out_of_bounds", {}};
    size_t end = std::min(q.offsets.size(), size_t(cursor) + limit);
    std::string out = "{\"handle\":" + std::to_string(id) + ",\"epoch\":" + std::to_string(epoch) + ",\"matches\":[";
    for(size_t i = cursor; i < end; ++i) {if(i != cursor) out += ','; out += matchJson(c, d, q.range, q.offsets[i], q.width, q.type, q.endian == "little");}
    return {{}, out + "],\"next_cursor\":" + (end < q.offsets.size() ? std::to_string(end) : "null") + "}"};
  }
  ControlResult changes(const std::vector<std::string>& a, const std::string& owner) {
    uint32_t before, after, range, cursor, limit;
    if(a.size() != 6 || !MemoryService::number(a[0], before) || !MemoryService::number(a[1], after)
      || !MemoryService::number(a[2], range) || !MemoryService::number(a[3], cursor) || !MemoryService::number(a[4], limit)
      || !limit || limit > 256 || (a[5] != "run" && a[5] != "line" && a[5] != "page")) return {"invalid_request", {}};
    auto error = compatible(before, after, range, owner); if(!error.empty()) return {error, {}};
    const auto& c = captures.at(before); const auto& d = captures.at(after);
    const auto& x = c.samples.front().bytes[range]; const auto& y = d.samples.front().bytes[range];
    const uint32_t base = c.ranges[range].offset;
    unsigned unit = a[5] == "line" ? 16 : a[5] == "page" ? 4096 : 1;
    if(cursor > x.size() || (unit > 1 && cursor && (base + cursor) % unit)) return {"invalid_request", {}};
    size_t end = std::min(x.size(), size_t(cursor) + 65536), i = cursor; unsigned count = 0;
    if(unit > 1 && end < x.size()) end -= (base + end) % unit;
    std::string out = "{\"semantics\":\"contents\",\"granularity\":" + controlQuote(a[5]) + ",\"changes\":[";
    while(i < end && count < limit) {
      size_t start = i, stop = std::min(x.size(), i + unit - (base + i) % unit);
      if(unit == 1) {if(x[i] == y[i]) {++i; continue;} while(i < end && x[i] != y[i]) ++i; stop = i;}
      else {i = stop; if(!std::memcmp(x.data() + start, y.data() + start, stop - start)) continue;}
      if(count++) out += ',';
      out += "{\"offset\":" + std::to_string(base + start) + ",\"length\":" + std::to_string(stop - start)
        + ",\"continues_before\":" + (unit == 1 && start && x[start-1] != y[start-1] ? "true" : "false")
        + ",\"continues_after\":" + (unit == 1 && stop < x.size() && x[stop] != y[stop] ? "true" : "false") + "}";
    }
    return {{}, out + "],\"next_cursor\":" + (i < x.size() ? std::to_string(i) : "null") + "}"};
  }
