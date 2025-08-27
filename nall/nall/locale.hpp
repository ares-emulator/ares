#pragma once

namespace nall {

struct Locale {
  struct Dictionary {
    string location;
    string language;
    Markup::Node document;
  };

  auto scan(string pathname) -> void {
    dictionaries.clear();
    selected.reset();
    for(auto filename : directory::icontents(pathname, "*.bml")) {
      Dictionary dictionary;
      dictionary.location = {pathname, filename};
      dictionary.document = BML::unserialize(string::read(dictionary.location));
      dictionary.language = dictionary.document["locale/language"].text();
      dictionaries.push_back(dictionary);
    }
  }

  auto available() const -> std::vector<string> {
    std::vector<string> result;
    for(auto& dictionary : dictionaries) {
      result.push_back(dictionary.language);
    }
    return result;
  }

  auto select(string option) -> bool {
    selected.reset();
    for(auto& dictionary : dictionaries) {
      if(option == Location::prefix(dictionary.location) || option == dictionary.language) {
        selected = dictionary;
        return true;
      }
    }
    return false;
  }

  template<typename... P>
  auto operator()(string ns, string input, P&&... p) const -> string {
    std::vector<string> arguments{std::forward<P>(p)...};
    if(selected) {
      for(auto node : selected().document) {
        if(node.name() == "namespace" && node.text() == ns) {
          for(auto map : node) {
            if(map.name() == "map" && map["input"].text() == input) {
              input = map["value"].text();
              break;
            }
          }
        }
      }
    }
    for(u32 index : range(arguments.size())) {
      auto placeholderText = string{ "{", index, "}" };
      input.replace(placeholderText, arguments[index]);
    }
    return input;
  }

  struct Namespace {
    Namespace(Locale& _locale, string _namespace) : _locale(_locale), _namespace(_namespace) {}

    template<typename... P>
    auto operator()(string input, P&&... p) const -> string {
      return _locale(_namespace, input, std::forward<P>(p)...);
    }

    template<typename... P>
    auto tr(string input, P&&... p) const -> string {
      return _locale(_namespace, input, std::forward<P>(p)...);
    }

  private:
    Locale& _locale;
    string _namespace;
  };

private:
  std::vector<Dictionary> dictionaries;
  maybe<Dictionary&> selected;
};

}
