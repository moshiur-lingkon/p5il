#include <stdio.h>
#include <string>
#include <stack>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <map>
#include <memory>

class Expr {
public:
  std::string atom;
  std::vector<Expr> list;

  Expr() : atom(), list() {
  }

  Expr(std::string s) : atom(s), list() {
  }

  Expr& operator = (const Expr& other) {
    std::string tmpStr = other.atom;
    atom = tmpStr;
    std::vector<Expr> tmpVec = other.list;
    list = tmpVec;
    return *this;
  }

  Expr(const Expr& other) {
    *this = other;
  }

  bool isAtom() const {
    return !atom.empty();
  }

  bool isBadExpr() const {
    return list.size() > 0 && list[0].isAtom() && list[0].atom == "badexpr";
  }

  Expr tail() const {
    Expr ret;
    for (int i = 1; i < list.size(); ++i) {
      ret.list.push_back(list[i]);
    }
    return ret;
  }

  void add(Expr e) {
    list.push_back(e);
  }

  void add(std::string s) {
    list.push_back(Expr(s));
  }

  std::string toStr() const {
    if (isAtom()) {
      return atom;
    }
    std::stringstream ss;
    ss<<"(";
    bool firstelem = true;
    for (auto e : list) {
      if (!firstelem) {
        ss<<" ";
      }
      firstelem = false;
      ss<<e.toStr();
    }
    ss<<")";
    return ss.str();
  }
};

std::string failedatpos(int i) {
  std::stringstream ss;
  ss << "failed-at-pos:" << i;
  return ss.str();
}

Expr parse(std::string code) {
  Expr res;
  code = "(" + code + ")"; // this makes it easy to handle atoms
  std::stack<Expr> stk;
  std::stringstream atom;
  size_t i = 0;
  while (i < code.size()) {
    char ch = code[i++];

    //TODO: support quoted strings
    if (isspace(ch) || ch == '(' || ch == ')') {
      if (!atom.str().empty()) {
        if (stk.empty()) {
          return parse("(badexpr " + failedatpos(i-1) + ")");
        }
        //std::cout<<"adding atom: " << atom.str() << std::endl;
        stk.top().add(atom.str());
        atom.str("");
      }
    }
    if (ch == '(') {
      stk.push(Expr()); // start a new expr
      //std::cout<<"starting new expr\n";
    } else if (ch == ')') {
      Expr e(stk.top());
      stk.pop();
      if (stk.empty()) {
        //std::cout<<"end\n";
        res = e;
        break;
      } else {
        //std::cout<<"putting under parent: " << stk.top().toStr() << " -> " << e.toStr() << std::endl;
        stk.top().add(e);
      }
    } else if (!isspace(ch)) {
      atom << ch;
    }
  }

  if (i < code.size()) {
    return parse("(badexpr " + failedatpos(i) + ")");
  }
  if (!stk.empty()) {
    return parse("(badexpr extra-brackets?)");
  }
  if (res.list.size() > 1) {
    return parse("(badexpr multiple-atoms)");
  }
  res = res.list.size() > 0 ? res.list[0] : Expr();
  return res;
}

static const Expr CONST_TRUE = parse("#t");
static const Expr CONST_FALSE = parse("#f");

class LispFunc {
public:
  virtual Expr apply(Expr e) = 0;
  virtual ~LispFunc() {}
};

// (atom? abc) => #t
// (atom? (a b)) => $f
class LispFunc_isAtom : public LispFunc {
public:
  Expr apply(Expr e) {
    if (e.list.size() != 1) {
      return parse("(badexpr \"bad-argnum-for-atom?\")");
    }
    return e.list[0].isAtom() ? CONST_TRUE : CONST_FALSE;
  }

  ~LispFunc_isAtom() {}
};

class LispFunc_quote : public LispFunc {
public:
  Expr apply(Expr e) {
    if (e.list.size() != 1) {
      return parse("(badexpr \"bad-argnum-for-quote");
    }
    return e.list[0];
  }
};

class LispFunc_addNum : public LispFunc {
public:
  Expr apply(Expr e) {
    Expr ret;
    ret.atom = std::to_string(
        std::atoi(e.list[0].atom.c_str()) +
        std::atoi(e.list[1].atom.c_str()) );
    return ret;
  }
};

std::map<std::string, std::unique_ptr<LispFunc> > ENV;

void prepareEnv() {
  ENV["atom?"].reset(new LispFunc_isAtom());
  ENV["quote"].reset(new LispFunc_quote());
  ENV["+"].reset(new LispFunc_addNum());
}

Expr eval(Expr e) {
  if (e.isAtom()) {
    return e;
  }
  int n = e.list.size();
  if (n == 0) { // empty list
    return e;
  }
  if (e.list[0].atom != "quote") {
    for (int i = 0; i < n; ++i) {
      Expr val = eval(e.list[i]);
      if (val.isBadExpr()) {
        return val;
      }
      e.list[i] = val;
    }
  }

  if (e.list[0].isAtom()) {
    auto fnIter = ENV.find(e.list[0].atom);
    if (fnIter != ENV.end()) {
      return fnIter->second->apply(e.tail());
    }
    //std::cout << "ERROR: unkonwn operator: " << e.list[0].atom << std::endl;
    return parse("(badexpr \"unknown-operator\")");
  }
  return e;
}

int main() {
  prepareEnv();
  std::string code;
  while (std::getline(std::cin, code)) {
    Expr e = parse(code);
    std::cout << "parsed: " << e.toStr() << std::endl;
    Expr val = eval(e);
    std::cout << "evaled: " << val.toStr() << std::endl;
  }
  return 0;
}
