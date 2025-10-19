#pragma once

namespace nall::JSON {

struct ManagedNode;
using SharedNode = std::shared_ptr<ManagedNode>;

struct ManagedNode : Markup::ManagedNode {
protected:
  auto isDigit(char c) const -> bool {
    return c - '0' < 10u;
  }

  auto isHex(char c) const -> bool {
    return c - '0' < 10u || c - 'A' < 6u || c - 'a' < 6u;
  }

  auto isWhitespace(char c) const -> bool {
    if(c ==  ' ' || c == '\t') return true;
    if(c == '\r' || c == '\n') return true;
    return false;
  }

  auto skipWhitespace(const char*& p) const -> void {
    while(isWhitespace(*p)) p++;
  }

  auto parseMember(const char*& p) -> void {
    skipWhitespace(p);
    parseString(_name, p);
    skipWhitespace(p);
    if(*p++ != ':') throw "Expected ':'";
    parseElement(p);
  }

  auto parseObject(const char*& p) -> void {
    if(*p++ != '{') throw "Expected '{'";
    skipWhitespace(p);
    if(*p == '}') {
      p++;
      return;
    }
    while(true) {
      SharedNode node = std::make_shared<ManagedNode>();
      node->parseMember(p);
      _children.push_back(node);
      if(*p != ',') break;
      p++;
    }
    if(*p++ != '}') throw "Expected '}'";
  }

  auto parseArray(const char*& p) -> void {
    if(*p++ != '[') throw "Expected '['";
    skipWhitespace(p);
    if(*p == ']') {
      p++;
      return;
    }
    for(u32 index = 0;; index++) {
      SharedNode node = std::make_shared<ManagedNode>();
      node->_name = index;
      node->parseElement(p);
      _children.push_back(node);
      if(*p != ',') break;
      p++;
    }
    if(*p++ != ']') throw "Expected ']'";
  }

  auto utf8Encode(std::vector<char>& output, u32 c) -> void {
    if(c <= 0x00007f) {
      output.push_back(c);
      return;
    }
    if(c <= 0x0007ff) {
      output.push_back(0b110'00000 | c >>  6 & 0b000'11111);
      output.push_back(0b10'000000 | c >>  0 & 0b00'111111);
      return;
    }
    if(c <= 0x00ffff) {
      output.push_back(0b1110'0000 | c >> 12 & 0b0000'1111);
      output.push_back(0b10'000000 | c >>  6 & 0b00'111111);
      output.push_back(0b10'000000 | c >>  0 & 0b00'111111);
      return;
    }
    if(c <= 0x10ffff) {
      output.push_back(0b11110'000 | c >> 18 & 0b00000'111);
      output.push_back(0b10'000000 | c >> 12 & 0b00'111111);
      output.push_back(0b10'000000 | c >>  6 & 0b00'111111);
      output.push_back(0b10'000000 | c >>  0 & 0b00'111111);
      return;
    }
    throw "Illegal code point";
  }

  auto parseString(string& target, const char*& p) -> void {
    std::vector<char> output;

    if(*p++ != '"') throw "Expected opening '\"'";
    while(*p && u8(*p) >= ' ' && *p != '"') {
      if(*p == '\\') {
        p++;
        switch(*p) {
          case '"':
          case '\\':
          case '/': output.push_back(*p++); break;
          case 'b': output.push_back('\b'); p++; break;
          case 'f': output.push_back('\f'); p++; break;
          case 'n': output.push_back('\n'); p++; break;
          case 'r': output.push_back('\r'); p++; break;
          case 't': output.push_back('\t'); p++; break;
          case 'u': {
            p++;
            char codepoint[5];
            for(auto n : range(4)) {
              if(!isHex(*p)) throw "Expected hex digit";
              codepoint[n] = *p++;
            }
            codepoint[4] = 0;
            //not implemented: combining surrogate pairs
            utf8Encode(output, toHex(codepoint));
          } break;
          default: throw "Expected escape sequence";
        }
      } else {
        output.push_back(*p++);
      }
    }
    if(*p++ != '\"') throw "Expected closing '\"'";

    target.resize(output.size());
    memory::copy(target.get(), output.data(), output.size());
  }

  auto parseNumber(const char*& p) -> void {
    const char* b = p;

    //integer
    if(*p == '-') p++;
    if(!isDigit(*p)) throw "Expected digit";
    if(*p++ != '0') {
      while(isDigit(*p)) p++;
    }

    //fraction
    if(*p == '.') {
      p++;
      if(!isDigit(*p++)) throw "Expected digit";
      while(isDigit(*p)) p++;
    }

    //exponent
    if(*p == 'E' || *p == 'e') {
      p++;
      if(*p == '+' || *p == '-') p++;
      if(!isDigit(*p++)) throw "Expected digit";
      while(isDigit(*p)) p++;
    }

    u32 length = p - b;
    _value.resize(length);
    memory::copy(_value.get(), b, length);
  }

  auto parseLiteral(const char*& p) -> void {
    if(!memory::compare(p, "null",  4)) { _value = "null";  p += 4; return; }
    if(!memory::compare(p, "true",  4)) { _value = "true";  p += 4; return; }
    if(!memory::compare(p, "false", 5)) { _value = "false"; p += 5; return; }
    throw "Expected literal";
  }

  auto parseValue(const char*& p) -> void {
    if(*p == '{') return parseObject(p);
    if(*p == '[') return parseArray(p);
    if(*p == '"') return parseString(_value, p);
    if(*p == '-' || isDigit(*p)) return parseNumber(p);
    if(*p == 't' || *p == 'f' || *p == 'n') return parseLiteral(p);
    throw "Unexpected character";
  }

  auto parseElement(const char*& p) -> void {
    skipWhitespace(p);
    parseValue(p);
    skipWhitespace(p);
  }

  friend auto unserialize(const string&) -> Markup::Node;
};

inline auto unserialize(const string& markup) -> Markup::Node {
  SharedNode node = std::make_shared<ManagedNode>();
  try {
    const char* p = markup;
    node->parseElement(p);
    if(*p) throw "Unexpected trailing data";
  } catch(const char* error) {
    node.reset();
  }
  return Markup::SharedNode(node);
}

}
