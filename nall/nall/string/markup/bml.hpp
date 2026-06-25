#pragma once

//BML v1.0 parser
//revision 0.04

namespace nall::BML {

//metadata is used to store nesting level

struct ManagedNode;
using SharedNode = std::shared_ptr<ManagedNode>;

struct ManagedNode : Markup::ManagedNode {
protected:
  //test to verify if a valid ASCII character for a node name
  auto validASCII(u8 c) const -> bool {  //A-Z, a-z, 0-9, +-.
    return c - 'A' < 26u || c - 'a' < 26u || c - '0' < 10u || c - '+' < 4u;
  }

  auto validContinuation(u8 c) const -> bool {
    return c >= 0x80 && c <= 0xbf;
  }

  //determine name length while validating any UTF-8 characters
  auto parseNameLength(const char* p) const -> u32 {
    u32 length = 0;
    while(p[length]) {
      auto c = (u8)p[length];
      if(validASCII(c)) {
        length++;
        continue;
      }

      if(c < 0x80) break;

      //U+0080..U+07FF
      if(c >= 0xc2 && c <= 0xdf) {
        if(!validContinuation((u8)p[length + 1])) throw "Invalid UTF-8 node name";
        length += 2;
        continue;
      }

      //U+0800..U+0FFF; avoid overlong encodings
      if(c == 0xe0) {
        auto c1 = (u8)p[length + 1];
        if(c1 < 0xa0 || c1 > 0xbf) throw "Invalid UTF-8 node name";
        auto c2 = (u8)p[length + 2];
        if(!validContinuation(c2)) throw "Invalid UTF-8 node name";
        length += 3;
        continue;
      }

      //U+1000..U+CFFF, U+E000..U+FFFF; reject surrogates
      if((c >= 0xe1 && c <= 0xec) || (c >= 0xee && c <= 0xef)) {
        auto c1 = (u8)p[length + 1];
        if(!validContinuation(c1)) throw "Invalid UTF-8 node name";
        auto c2 = (u8)p[length + 2];
        if(!validContinuation(c2)) throw "Invalid UTF-8 node name";
        length += 3;
        continue;
      }

      //U+D000..U+D7FF; reject U+D800..U+DFFF surrogates
      if(c == 0xed) {
        auto c1 = (u8)p[length + 1];
        if(c1 < 0x80 || c1 > 0x9f) throw "Invalid UTF-8 node name";
        auto c2 = (u8)p[length + 2];
        if(!validContinuation(c2)) throw "Invalid UTF-8 node name";
        length += 3;
        continue;
      }

      //U+10000..U+3FFFF; avoid overlong encodings
      if(c == 0xf0) {
        auto c1 = (u8)p[length + 1];
        if(c1 < 0x90 || c1 > 0xbf) throw "Invalid UTF-8 node name";
        auto c2 = (u8)p[length + 2];
        if(!validContinuation(c2)) throw "Invalid UTF-8 node name";
        auto c3 = (u8)p[length + 3];
        if(!validContinuation(c3)) throw "Invalid UTF-8 node name";
        length += 4;
        continue;
      }

      //U+40000..U+FFFFF
      if(c >= 0xf1 && c <= 0xf3) {
        auto c1 = (u8)p[length + 1];
        if(!validContinuation(c1)) throw "Invalid UTF-8 node name";
        auto c2 = (u8)p[length + 2];
        if(!validContinuation(c2)) throw "Invalid UTF-8 node name";
        auto c3 = (u8)p[length + 3];
        if(!validContinuation(c3)) throw "Invalid UTF-8 node name";
        length += 4;
        continue;
      }

      //U+100000..U+10FFFF
      if(c == 0xf4) {
        auto c1 = (u8)p[length + 1];
        if(c1 < 0x80 || c1 > 0x8f) throw "Invalid UTF-8 node name";
        auto c2 = (u8)p[length + 2];
        if(!validContinuation(c2)) throw "Invalid UTF-8 node name";
        auto c3 = (u8)p[length + 3];
        if(!validContinuation(c3)) throw "Invalid UTF-8 node name";
        length += 4;
        continue;
      }

      throw "Invalid UTF-8 node name";
    }

    return length;
  }

  //determine indentation level, without incrementing pointer
  auto readDepth(const char* p) -> u32 {
    u32 depth = 0;
    while(p[depth] == '\t' || p[depth] == ' ') depth++;
    return depth;
  }

  //determine indentation level
  auto parseDepth(const char*& p) -> u32 {
    u32 depth = readDepth(p);
    p += depth;
    return depth;
  }

  //read name
  auto parseName(const char*& p) -> void {
    u32 length = parseNameLength(p);
    if(length == 0) throw "Invalid node name";
    _name = slice(p, 0, length);
    p += length;
  }

  auto parseData(const char*& p, string_view spacing) -> void {
    if(*p == '=' && *(p + 1) == '\"') {
      u32 length = 2;
      while(p[length] && p[length] != '\n' && p[length] != '\"') length++;
      if(p[length] != '\"') throw "Unescaped value";
      _value = {slice(p, 2, length - 2), "\n"};
      p += length + 1;
    } else if(*p == '=') {
      u32 length = 1;
      while(p[length] && p[length] != '\n' && p[length] != '\"' && p[length] != ' ') length++;
      if(p[length] == '\"') throw "Illegal character in value";
      _value = {slice(p, 1, length - 1), "\n"};
      p += length;
    } else if(*p == ':') {
      u32 length = 1;
      while(p[length] && p[length] != '\n') length++;
      _value = {slice(p, 1, length - 1).trimLeft(spacing, 1L), "\n"};
      p += length;
    }
  }

  //read all attributes for a node
  auto parseAttributes(const char*& p, string_view spacing) -> void {
    while(*p && *p != '\n') {
      if(*p != ' ') throw "Invalid node name";
      while(*p == ' ') p++;  //skip excess spaces
      if(*(p + 0) == '/' && *(p + 1) == '/') break;  //skip comments

      SharedNode node = std::make_shared<ManagedNode>();
      u32 length = parseNameLength(p);
      if(length == 0) throw "Invalid attribute name";
      node->_name = slice(p, 0, length);
      node->parseData(p += length, spacing);
      node->_value.trimRight("\n", 1L);
      _children.push_back(node);
    }
  }

  //read a node and all of its child nodes
  auto parseNode(const std::vector<string>& text, u32& y, string_view spacing) -> void {
    const char* p = text[y++];
    _metadata = parseDepth(p);
    parseName(p);
    parseData(p, spacing);
    parseAttributes(p, spacing);

    while(y < text.size()) {
      u32 depth = readDepth(text[y]);
      if(depth <= _metadata) break;

      if(text[y][depth] == ':') {
        _value.append(slice(text[y++], depth + 1).trimLeft(spacing, 1L), "\n");
        continue;
      }

      SharedNode node = std::make_shared<ManagedNode>();
      node->parseNode(text, y, spacing);
      _children.push_back(node);
    }

    _value.trimRight("\n", 1L);
  }

  //read top-level nodes
  auto parse(string document, string_view spacing) -> void {
    //in order to simplify the parsing logic; we do an initial pass to normalize the data
    //the below code will turn '\r\n' into '\n'; skip empty lines; and skip comment lines
    char* p = document.get(), *output = p;
    while(*p) {
      char* origin = p;
      bool empty = true;
      while(*p) {
        //scan for first non-whitespace character. if it's a line feed or comment; skip the line
        if(p[0] == ' ' || p[0] == '\t') { p++; continue; }
        empty = p[0] == '\r' || p[0] == '\n' || (p[0] == '/' && p[1] == '/');
        break;
      }
      while(*p) {
        if(p[0] == '\r') p[0] = '\n';  //turns '\r\n' into '\n\n' (second '\n' will be skipped)
        if(*p++ == '\n') break;        //include '\n' in the output to be copied
      }
      if(empty) continue;

      memory::move(output, origin, p - origin);
      output += p - origin;
    }
    document.resize(document.size() - (p - output)).trimRight("\n");
    if(document.size() == 0) return;  //empty document

    auto text = nall::split(document, "\n");
    u32 y = 0;
    while(y < text.size()) {
      SharedNode node(new ManagedNode);
      node->parseNode(text, y, spacing);
      if(node->_metadata > 0) throw "Root nodes cannot be indented";
      _children.push_back(node);
    }
  }

  friend auto unserialize(const string&, string_view) -> Markup::Node;
};

inline auto unserialize(const string& markup, string_view spacing = {}) -> Markup::Node {
  SharedNode node = std::make_shared<ManagedNode>();
  try {
    node->parse(markup, spacing);
  } catch(const char* error) {
    node.reset();
  }
  return Markup::SharedNode(node);
}

inline auto serialize(const Markup::Node& node, string_view spacing = {}, u32 depth = 0) -> string {
  if(!node.name()) {
    string result;
    for(auto leaf : node) {
      result.append(serialize(leaf, spacing, depth));
    }
    return result;
  }

  string padding;
  padding.resize(depth * 2);
  padding.fill(' ');

  std::vector<string> lines;
  if(auto value = node.value()) lines = nall::split(value, "\n");

  string result;
  result.append(padding);
  result.append(node.name());
  if(lines.size() == 1) result.append(":", spacing, lines[0]);
  result.append("\n");
  if(lines.size() > 1) {
    padding.append("  ");
    for(auto& line : lines) {
      result.append(padding, ":", spacing, line, "\n");
    }
  }
  for(auto leaf : node) {
    result.append(serialize(leaf, spacing, depth + 1));
  }
  return result;
}

}
