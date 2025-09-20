#pragma once

namespace nall::Markup {

inline auto ManagedNode::_evaluate(string query) const -> bool {
  if(!query) return true;

  for(auto& rule : nall::split(query, ",")) {
    enum class Comparator : u32 { ID, EQ, NE, LT, LE, GT, GE, NF };
    auto comparator = Comparator::ID;
         if(rule.match("*!=*")) comparator = Comparator::NE;
    else if(rule.match("*<=*")) comparator = Comparator::LE;
    else if(rule.match("*>=*")) comparator = Comparator::GE;
    else if(rule.match ("*=*")) comparator = Comparator::EQ;
    else if(rule.match ("*<*")) comparator = Comparator::LT;
    else if(rule.match ("*>*")) comparator = Comparator::GT;
    else if(rule.match  ("!*")) comparator = Comparator::NF;

    if(comparator == Comparator::ID) {
      if(_find(rule).size()) continue;
      return false;
    }

    if(comparator == Comparator::NF) {
      rule.trimLeft("!", 1L);
      if(_find(rule).size()) return false;
      continue;
    }

    std::vector<string> side;
    switch(comparator) {
    case Comparator::EQ: side = nall::split(rule, "=", 1L); break;
    case Comparator::NE: side = nall::split(rule, "!=", 1L); break;
    case Comparator::LT: side = nall::split(rule, "<", 1L); break;
    case Comparator::LE: side = nall::split(rule, "<=", 1L); break;
    case Comparator::GT: side = nall::split(rule, ">", 1L); break;
    case Comparator::GE: side = nall::split(rule, ">=", 1L); break;
    }

    string data = string{_value}.strip();
    if(!side.empty() && side[0]) {
      auto result = _find(side[0]);
      if(result.size() == 0) return false;
      data = result[0].text();  //strips whitespace so rules can match without requiring it
    }

    switch(comparator) {
    case Comparator::EQ: if(side.size() > 1 && data.match(side[1]) ==  true)      continue; break;
    case Comparator::NE: if(side.size() > 1 && data.match(side[1]) == false)      continue; break;
    case Comparator::LT: if(side.size() > 1 && data.natural()  < side[1].natural()) continue; break;
    case Comparator::LE: if(side.size() > 1 && data.natural() <= side[1].natural()) continue; break;
    case Comparator::GT: if(side.size() > 1 && data.natural()  > side[1].natural()) continue; break;
    case Comparator::GE: if(side.size() > 1 && data.natural() >= side[1].natural()) continue; break;
    }

    return false;
  }

  return true;
}

inline auto ManagedNode::_find(const string& query) const -> std::vector<Node> {
  std::vector<Node> result;

  auto path = nall::split(query, "/");
  string name = path.empty() ? string{} : path.front();
  if(!path.empty()) path.erase(path.begin());
  string rule;
  u32 lo = 0u, hi = ~0u;

  if(name.match("*[*]")) {
    auto p = nall::split(name.trimRight("]", 1L), "[", 1L);
    name = p.empty() ? string{} : p[0];
    if(p.size() > 1 && p[1].find("-")) {
      auto p2 = nall::split(p[1], "-", 1L);
      lo = p2.empty() || !p2[0] ?  0u : p2[0].natural();
      hi = p2.size() <= 1 || !p2[1] ? ~0u : p2[1].natural();
    } else if(p.size() > 1) {
      lo = hi = p[1].natural();
    }
  }

  if(name.match("*(*)")) {
    auto p = nall::split(name.trimRight(")", 1L), "(", 1L);
    name = p.empty() ? string{} : p[0];
    rule = p.size() > 1 ? p[1] : string{};
  }

  u32 position = 0;
  for(auto& node : _children) {
    if(!node->_name.match(name)) continue;
    if(!node->_evaluate(rule)) continue;

    bool inrange = position >= lo && position <= hi;
    position++;
    if(!inrange) continue;

    if(path.empty()) {
      result.push_back(node);
    } else for(auto& item : node->_find(nall::merge(path, "/"))) {
      result.push_back(item);
    }
  }

  return result;
}

//operator[](string)
inline auto ManagedNode::_lookup(const string& path) const -> Node {
  auto result = _find(path);
  return !result.empty() ? result[0] : Node{};

/*//faster, but cannot search
  if(auto position = path.find("/")) {
    auto name = slice(path, 0, *position);
    for(auto& node : _children) {
      if(name == node->_name) {
        return node->_lookup(slice(path, *position + 1));
      }
    }
  } else for(auto& node : _children) {
    if(path == node->_name) return node;
  }
  return {};
*/
}

inline auto ManagedNode::_create(const string& path) -> Node {
  if(auto position = path.find("/")) {
    auto name = slice(path, 0, *position);
    for(auto& node : _children) {
      if(name == node->_name) {
        return node->_create(slice(path, *position + 1));
      }
    }
    _children.push_back(std::make_shared<ManagedNode>(name));
    return _children.back()->_create(slice(path, *position + 1));
  }
  for(auto& node : _children) {
    if(path == node->_name) return node;
  }
  _children.push_back(std::make_shared<ManagedNode>(path));
  return _children.back();
}

}
