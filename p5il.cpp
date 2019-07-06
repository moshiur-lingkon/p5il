#include <stdio.h>
#include <string>
#include <stack>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <map>
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
    std::string tmpStr = other.atom;
    atom = tmpStr;
    std::vector<Expr> tmpVec = other.list;
    list = tmpVec;
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
      ret.list.push_back(list[i]);
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

class LispFunc {
public:
  virtual Expr apply(Expr e) = 0;
  virtual ~LispFunc() {}
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

class LF_quote : public LispFunc {
public:
  Expr apply(Expr e) {
    if (e.list.size() != 1) {
      return parse("(badexpr bad-argnum-for-quote)");
    }
    return e.list[0];
  }
};

class LF_addNum : public LispFunc {
public:
  Expr apply(Expr e) {
    Expr ret;
    int sum = 0;
    for (auto se : e.list) {
      sum += std::atoi(se.atom.c_str());
    }
    ret.atom = std::to_string(sum);
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

std::map<std::string, std::unique_ptr<LispFunc> > ENV;

#define ADD_FUNC(X,CLS) {ENV[X].reset(new CLS());}

void prepareEnv() {
  ADD_FUNC("atom?", LF_isAtom);
  ADD_FUNC("quote", LF_quote);
  ADD_FUNC("eq?", LF_equal);
  ADD_FUNC("+", LF_addNum);
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
  //std::cout<<e.list[0].toStr()<<std::endl;
  return !e.isAtom() && e.list[0].atom == "lambda";
}

Expr eval(Expr e) {
std::cout<<":::1:: " << e.toStr()<<"\n";
  if (e.isAtom()) {
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
    if (condition.atom == "#t") {
      return eval(e.list[2]);
    } else {
      return eval(e.list[3]);
    }
  }
  if (isLambda(e.list[0])) {
    std::cout<<"isLambda!"<<std::endl;
    Expr params = e.list[0].list[1];
    Expr args = e.tail();
    Expr expr = e.list[0].list[2];
    Expr args2;
    for (int i = 0; i < args.size(); ++i) {
      args2.add(eval(args.list[i]));
    }
    if (params.size() != args2.size()) {
      std::cout<<"WTF!!! " << params.size() << ", " << args2.size() << "\n";
    }
    puts("1");
    std::map<std::string, Expr> vals;
    for (int i = 0; i < params.size(); ++i) {
      vals[params.list[i].atom] = args2.list[i];
      //vals.insert(std::make_pair(params.list[i].atom, args2.list[i]));
    }
    puts("2");
    Expr e2 = replace(vals, expr);
    return eval(e2);
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
    return parse("(badexpr unknown-operator)");
  }
  return e;
}

int main() {
  prepareEnv();
  std::string code;
  while (true) {
    std::cout << ">>> ";
    std::getline(std::cin, code);
    if (code == "exit") {
      break;
    }
    Expr e = parse(code);
    //std::cout << "parsed: " << e.toStr() << std::endl;
    Expr val = eval(e);
    std::cout << val.toStr() << std::endl;
  }
  return 0;
}
