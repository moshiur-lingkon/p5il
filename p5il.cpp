#include <stdio.h>
#include <string>
#include <stack>
#include <vector>
#include <iostream>
#include <sstream>
#include <map>

class Expr {
public:
  std::string atom;
  std::vector<Expr> list;
  Expr() {}
  Expr(std::string s) : atom(s), list() {
  }
  bool isAtom() const {
    return !atom.empty();
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

struct Status {
  bool success;
  std::string msg;
};

std::string failedatpos(int i) {
  std::stringstream ss;
  ss << "failed at pos: " << i;
  return ss.str();
}

Status parse(std::string code, Expr& res) {
  code = "(" + code + ")"; // this makes it easy to handle atoms
  std::stack<Expr> stk;
  std::string atom = "";
  size_t i = 0;
  while (i < code.size()) {
    char ch = code[i++];
    if (isspace(ch) || ch == '(' || ch == ')') {
      if (!atom.empty()) {
        if (stk.empty()) {
          std::cout << "haha 1\n";
          return {false, failedatpos(i)};
        }
        stk.top().add(atom);
        atom = "";
      }
    }
    if (ch == '(') {
      stk.push(Expr()); // start a new expr
    } else if (ch == ')') {
      Expr e = stk.top();
      stk.pop();
      if (stk.empty()) {
        res = e;
        break;
      } else {
        stk.top().add(e);
      }
    } else if (!isspace(ch)) {
      atom.push_back(ch);
    }
  }

  while (i < code.size() && isspace(code[i])) { // trailing space
    ++i;
  }
  if (i < code.size()) {
          std::cout << "haha 2\n";
    return {false, failedatpos(i)};
  }
  if (!stk.empty()) {
    return {false, "extra brackets?"};
  }
  if (res.list.size() > 1) {
    return {false, "multiple atoms"};
  }
  res = res.list[0];
  return {true, "parsed successfully"};
}

/*
class LispFunc {
public:
  virtual Expr apply(Expr e) = 0;
};

// (atom? abc) => #t
// (atom? (a b)) => $f
class LispFunc_isAtom : public LispFunc {
public:
  Expr apply(Expr e) {
    if (e.list.size() < 2) {
      return parse("(error \"not enough args for atom?\")");
    }
    if (e.list.size() > 2) {
      return parse("(error \"too many args for atom?\")");
    }
    return e.list[1].isEmpty();
  }
};

std::map<std::string, std::unique_ptr<LispFunc> > ENV;

void prepareEnv() {
  ENV["atom?"] = new LispFunc_isAtom()
}

Expr eval(Expr e) {
  if (e.isAtom()) {
    return e;
  }
  int n = e.list.size();
  for (int i = 0; i < n; ++i) {
    e.list[i] = eval(e.list[i]);
  }
  if (e.list[0].isAtom()) {
    auto fnIter = ENV.find(e.list[0]);
    if (fnIter != ENV.end()) {
      return fnIter.apply(e);
    }
    std::cout << "ERROR: unkonwn operator: " << e.list[0].atom;
    return parse("(error, \"unknown operator\")");
  }
}
*/

int main() {
  std::string code;
  while (std::getline(std::cin, code)) {
    Expr e;
    Status s = parse(code, e);
    std::cout << s.success << ", " << s.msg << std::endl;
    std::cout << e.toStr() << std::endl;
    //Expr val = eval(e);
  }
  return 0;
}
