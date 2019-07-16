#include <stdio.h>
#include <string>
#include <stack>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <map>
#include <set>
#include <memory>

class Expr;
Expr parse(std::string code);

class Expr {
public:
  std::string atom;
  std::vector<Expr> list;

  Expr() : atom(""), list() {
  }

  Expr(std::string s) : atom(s), list() {
  }

  Expr& operator = (const Expr& other) {
    atom = other.atom;
    list = other.list;
    return *this;
  }

  Expr(const Expr& other) {
    *this = other;
  }

  bool operator == (const Expr& other) {
    if (atom  != other.atom) return false;
    if (list.size() != other.list.size()) {
      return false;
    }
    for (size_t i = 0; i < list.size(); ++i) {
      if (!(list[i] == other.list[i])) {
        return false;
      }
    }
    return true;
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
      ret.add(list[i]);
    }
    return ret;
  }

  Expr head() const {
    if (isAtom()) {
      return parse("(badexpr no-head-of-atom)");
    }
    if (list.size() == 0) {
      return parse("(badexpr no-head-of-empty-list)");
    }
    return list[0];
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

  size_t size() const {
    return list.size();
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
static const Expr EXIT_FUNC = parse("(exit)");

class LispFunc {
public:
  virtual Expr apply(Expr e) = 0;
  virtual ~LispFunc() {}
};

class LF_cons : public LispFunc {
public:
  Expr apply(Expr e) {
    if (e.size() != 2) {
      return parse("(badexpr bad-argnum-for-cons)");
    }
    Expr ret;
    ret.add(e.list[0]);
    if (e.list[1].isAtom()) {
      ret.add(e.list[1].atom);
    } else {
      for (auto elem : e.list[1].list) {
        ret.add(elem);
      }
    }
    return ret;
  }
};

class LF_head : public LispFunc {
public:
  Expr apply(Expr e) {
    if (e.size() == 0) {
      return parse("(badexpr bad-argnum-for-head)");
    }
    if (e.list[0].isAtom()) {
      return parse("(badexpr atom-has-no-head)");
    }
    return e.list[0].head();
  }
};

class LF_tail : public LispFunc {
public:
  Expr apply(Expr e) {
    if (e.size() == 0) {
      return parse("(badexpr bad-argnum-for-tail)");
    }
    if (e.list[0].isAtom()) {
      return parse("(badexpr atom-has-no-tail)");
    }
    return e.list[0].tail();
  }
};

// (atom? abc) => #t
// (atom? (a b)) => $f
class LF_isAtom : public LispFunc {
public:
  Expr apply(Expr e) {
    if (e.list.size() != 1) {
      return parse("(badexpr bad-argnum-for-atom?)");
    }
    return e.list[0].isAtom() ? CONST_TRUE : CONST_FALSE;
  }

  ~LF_isAtom() {}
};

class LF_addNum : public LispFunc {
public:
  Expr apply(Expr e) {
    Expr ret;
    long long sum = 0;
    for (auto se : e.list) {
      sum += std::atoll(se.atom.c_str());
    }
    ret.atom = std::to_string(sum);
    return ret;
  }
};

class LF_mulNum : public LispFunc {
public:
  Expr apply(Expr e) {
    Expr ret;
    long long mul = 1;
    for (auto se : e.list) {
      mul *= std::atoll(se.atom.c_str());
    }
    ret.atom = std::to_string(mul);
    return ret;
  }
};

class LF_equal : public LispFunc {
public:
  Expr apply(Expr e) {
    Expr left = e.list[0];
    Expr right = e.list[1];
    return left == right ? CONST_TRUE : CONST_FALSE;
  }
};

class LF_or : public LispFunc {
public:
  Expr apply(Expr e) {
    for (size_t i = 0; i < e.size(); ++i) {
      if (e.list[i] == CONST_TRUE) {
        return CONST_TRUE;
      }
    }
    return CONST_FALSE;
  }
};

class LF_and : public LispFunc {
public:
  Expr apply(Expr e) {
    for (size_t i = 0; i < e.size(); ++i) {
      if (e.list[i] == CONST_FALSE) {
        return CONST_FALSE;
      }
    }
    return CONST_TRUE;
  }
};

class LF_not : public LispFunc {
public:
  Expr apply(Expr e) {
    if (e == CONST_TRUE) {
      return CONST_FALSE;
    } else if (e == CONST_FALSE) {
      return CONST_TRUE;
    } else {
      return parse("(badexpr not-boolean)");
    }
  }
};

struct {
  std::map<std::string, std::unique_ptr<LispFunc> > funcs;
  std::map<std::string, Expr> defs;
} ENV;

Expr eval(Expr);

class LF_defReplacer : public LispFunc {
public:
  Expr defVal;
  LF_defReplacer(Expr e) : defVal(e) {
  }
  Expr apply(Expr e) {
    return defVal;
  }
};

#define ADD_FUNC(X,CLS) {ENV.funcs[X].reset(new CLS());}

void loadPrimitives() {
  ADD_FUNC("atom?", LF_isAtom);
  ADD_FUNC("head", LF_head);
  ADD_FUNC("tail", LF_tail);
  ADD_FUNC("cons", LF_cons);

  ADD_FUNC("eq?", LF_equal);
  ADD_FUNC("or", LF_or);
  ADD_FUNC("and", LF_and);
  ADD_FUNC("not", LF_not);
  ADD_FUNC("+", LF_addNum);
  ADD_FUNC("*", LF_mulNum);
  ADD_FUNC("not", LF_not);
  // special:
  // "if"
  // "def"
  // "quote"
  // "lambda"
}

Expr replace(std::map<std::string, Expr> vals, Expr e) {
  Expr ret;
  if (e.isAtom()) {
    ret = e;
    auto v = vals.find(e.atom);
    if (v != vals.end()) {
      ret = v->second;
    }
    return ret;
  } else {
    for (int i = 0; i < e.size(); ++i) {
      ret.add(replace(vals, e.list[i]));
    }
    return ret;
  }
}

bool isLambda(Expr e) {
  return !e.isAtom() && e.size() == 3 &&
    (e.list[0].atom == "lambda" || e.list[0].atom == "\\") && !e.list[1].isAtom();
}


Expr eval(Expr e) {
  if (e.isAtom()) {
    auto it = ENV.defs.find(e.atom);
    if (it != ENV.defs.end()) {
      return it->second;
    }
    return e;
  }
  int n = e.list.size();
  if (n == 0) { // empty list
    return e;
  }
  if (e.list[0].atom == "if") {
    if (e.list.size() != 4) {
      return parse("(badexpr bad-argnum-for-if)");
    }
    Expr condition = eval(e.list[1]);
    if (condition == CONST_TRUE) {
      return eval(e.list[2]);
    } else {
      return eval(e.list[3]);
    }
  }
  if (e.list[0].atom == "def") {
    if (e.list.size() != 3) {
      return parse("(badexpr bad-argnum-for-def)");
    }
    if (!e.list[1].isAtom()) {
      return parse("(badexpr def-arg-should-be-atom)");
    }
    ENV.defs[e.list[1].atom] = eval(e.list[2]);
    return Expr();
  }
  if (e.list[0].atom == "quote") {
    if (e.size() != 2) {
      return parse("(badexpr bad-argnum-for-quote");
    }
    return e.list[1];
  }
  if (isLambda(e)) {
    return e;
  }
  e.list[0] = eval(e.list[0]);
  if (isLambda(e.list[0])) {
    Expr params = e.list[0].list[1];
    Expr args = e.tail();
    Expr expr = e.list[0].list[2];
    if (params.size() != args.size()) {
      return parse("(badexpr params-size-args-size-mismatch)");
    }
    std::map<std::string, Expr> vals;
    for (int i = 0; i < params.size(); ++i) {
      vals[params.list[i].atom] = args.list[i];
    }
    Expr e2 = replace(vals, expr);
    return eval(e2);
  }
  if (e.list[0].isAtom()) {
    for (size_t i = 1; i < e.list.size(); ++i) {
      e.list[i] = eval(e.list[i]);
      if (e.list[i].isBadExpr()) {
        return e.list[i];
      }
    }
    auto fnIter = ENV.funcs.find(e.list[0].atom);
    if (fnIter != ENV.funcs.end()) {
      return fnIter->second->apply(e.tail());
    }
    return parse("(badexpr unknown-operator-" + e.list[0].atom +")");
  } else {
    return parse("(badexpr operator-not-atom-or-lambda)");
  }
}

void printEnv() {
  std::cout << "ENV: funcs: {";
  for (auto it = ENV.funcs.begin(); it != ENV.funcs.end(); ++it) {
    std::cout << it->first << ", ";
  }
  std::cout << "} defs: {";
  for (auto it = ENV.defs.begin(); it != ENV.defs.end(); ++it) {
    std::cout << it->first << ", ";
  }
  std::cout << "}\n";
}

int main() {
  loadPrimitives();
  while (true) {
    std::string code;
    std::cout << ">>> ";
    std::getline(std::cin, code);
    Expr e = parse(code);
    while (e.isBadExpr()) {
      std::string morelines;
      std::getline(std::cin, morelines);
      code += " " + morelines;
      e = parse(code);
    }
    if (e == EXIT_FUNC) {
      break;
    }
    //std::cout << "parsed: " << e.toStr() << std::endl;
    Expr val = eval(e);
    std::cout << "= " << val.toStr() << std::endl;
    //printEnv();
  }
  return 0;
}
