/*********************                                                        */
/*! \file tptp.h
 ** \verbatim
 ** Original author: bobot
 ** Major contributors: none
 ** Minor contributors (to current version): ajreynol, mdeters
 ** This file is part of the CVC4 prototype.
 ** Copyright (c) 2009-2012  New York University and The University of Iowa
 ** See the file COPYING in the top-level source directory for licensing
 ** information.\endverbatim
 **
 ** \brief Definitions of TPTP constants.
 **
 ** Definitions of TPTP constants.
 **/

#include "cvc4parser_private.h"

#ifndef __CVC4__PARSER__TPTP_H
#define __CVC4__PARSER__TPTP_H

#include "parser/parser.h"
#include "expr/command.h"
#include <ext/hash_set>
#include <cassert>

namespace CVC4 {

class SExpr;

namespace parser {

class Tptp : public Parser {
  friend class ParserBuilder;

  // In CNF variable are implicitly binded
  // d_freevar collect them
  std::vector< Expr > d_freeVar;
  Expr d_rtu_op;
  Expr d_stu_op;
  Expr d_utr_op;
  Expr d_uts_op;
  // The set of expression that already have a bridge
  std::hash_set<Expr, ExprHashFunction> d_r_converted;
  std::hash_set<Expr, ExprHashFunction> d_s_converted;

  //TPTP directory where to find includes
  // empty if none could be determined
  std::string d_tptpDir;

public:
  bool cnf; //in a cnf formula

  void addFreeVar(Expr var){assert(cnf); d_freeVar.push_back(var); };
  std::vector< Expr > getFreeVar(){
    assert(cnf);
    std::vector< Expr > r;
    r.swap(d_freeVar);
    return r;
  }

  inline Expr convertRatToUnsorted(Expr expr){
    ExprManager * em = getExprManager();

    // Create the conversion function If they doesn't exists
    if(d_rtu_op.isNull()){
      Type t;
      //Conversion from rational to unsorted
      t = em->mkFunctionType(em->realType(), d_unsorted);
      d_rtu_op = em->mkVar("$$rtu",t);
      preemptCommand(new DeclareFunctionCommand("$$rtu", d_rtu_op, t));
      //Conversion from unsorted to rational
      t = em->mkFunctionType(d_unsorted, em->realType());
      d_utr_op = em->mkVar("$$utr",t);
      preemptCommand(new DeclareFunctionCommand("$$utur", d_utr_op, t));
    }
    // Add the inverse in order to show that over the elements that
    // appear in the problem there is a bijection between unsorted and
    // rational
    Expr ret = em->mkExpr(kind::APPLY_UF,d_rtu_op,expr);
    if ( d_r_converted.find(expr) == d_r_converted.end() ){
      d_r_converted.insert(expr);
      Expr eq = em->mkExpr(kind::EQUAL,expr,
                           em->mkExpr(kind::APPLY_UF,d_utr_op,ret));
      preemptCommand(new AssertCommand(eq));
    };
    return ret;
  }

  inline Expr convertStrToUnsorted(Expr expr){
    ExprManager * em = getExprManager();

    // Create the conversion function If they doesn't exists
    if(d_stu_op.isNull()){
      Type t;
      //Conversion from string to unsorted
      t = em->mkFunctionType(em->stringType(), d_unsorted);
      d_stu_op = em->mkVar("$$stu",t);
      preemptCommand(new DeclareFunctionCommand("$$stu", d_stu_op, t));
      //Conversion from unsorted to string
      t = em->mkFunctionType(d_unsorted, em->stringType());
      d_uts_op = em->mkVar("$$uts",t);
      preemptCommand(new DeclareFunctionCommand("$$uts", d_uts_op, t));
    }
    // Add the inverse in order to show that over the elements that
    // appear in the problem there is a bijection between unsorted and
    // rational
    Expr ret = em->mkExpr(kind::APPLY_UF,d_stu_op,expr);
    if ( d_s_converted.find(expr) == d_s_converted.end() ){
      d_s_converted.insert(expr);
      Expr eq = em->mkExpr(kind::EQUAL,expr,
                           em->mkExpr(kind::APPLY_UF,d_uts_op,ret));
      preemptCommand(new AssertCommand(eq));
    };
    return ret;
  }

public:

  //TPTP (CNF and FOF) is unsorted so we define this common type
  Type d_unsorted;

  enum Theory {
    THEORY_CORE,
  };

  enum FormulaRole {
    FR_AXIOM,
    FR_HYPOTHESIS,
    FR_DEFINITION,
    FR_ASSUMPTION,
    FR_LEMMA,
    FR_THEOREM,
    FR_CONJECTURE,
    FR_NEGATED_CONJECTURE,
    FR_UNKNOWN,
    FR_PLAIN,
    FR_FI_DOMAIN,
    FR_FI_FUNCTORS,
    FR_FI_PREDICATES,
    FR_TYPE,
  };


protected:
  Tptp(ExprManager* exprManager, Input* input, bool strictMode = false, bool parseOnly = false);

public:
  /**
   * Add theory symbols to the parser state.
   *
   * @param theory the theory to open (e.g., Core, Ints)
   */
  void addTheory(Theory theory);

  inline void makeApplication(Expr & expr, std::string & name,
                              std::vector<Expr> & args, bool term);

  inline Command* makeCommand(FormulaRole fr, Expr & expr);

  /** Ugly hack because I don't know how to return an expression from a
      token */
  Expr d_tmp_expr;

  /** Push a new stream in the lexer. When EOF is reached the previous stream
      is reused */
  void includeFile(std::string fileName);

private:

  void addArithmeticOperators();
};/* class Tptp */

inline void Tptp::makeApplication(Expr & expr, std::string & name,
                           std::vector<Expr> & args, bool term){
  // We distinguish the symbols according to their arities
  std::stringstream ss;
  ss << name << "_" << args.size();
  name = ss.str();
  if(args.empty()){ // Its a constant
    if(isDeclared(name)){ //already appeared
      expr = getVariable(name);
    } else {
      Type t = term ? d_unsorted : getExprManager()->booleanType();
      expr = mkVar(name,t,true); //levelZero
      preemptCommand(new DeclareFunctionCommand(name, expr, t));
    }
  } else { // Its an application
    if(isDeclared(name)){ //already appeared
      expr = getVariable(name);
    } else {
      std::vector<Type> sorts(args.size(), d_unsorted);
      Type t = term ? d_unsorted : getExprManager()->booleanType();
      t = getExprManager()->mkFunctionType(sorts, t);
      expr = mkVar(name,t,true); //levelZero
      preemptCommand(new DeclareFunctionCommand(name, expr, t));
    }
    expr = getExprManager()->mkExpr(kind::APPLY_UF, expr, args);
  }
};

inline Command* Tptp::makeCommand(FormulaRole fr, Expr & expr){
  switch(fr){
  case FR_AXIOM:
  case FR_HYPOTHESIS:
  case FR_DEFINITION:
  case FR_ASSUMPTION:
  case FR_LEMMA:
  case FR_THEOREM:
  case FR_NEGATED_CONJECTURE:
  case FR_PLAIN:
    // it's a usual assert
    return new AssertCommand(expr);
  case FR_CONJECTURE:
    // something to prove
    return new AssertCommand(getExprManager()->mkExpr(kind::NOT,expr));
  case FR_UNKNOWN:
  case FR_FI_DOMAIN:
  case FR_FI_FUNCTORS:
  case FR_FI_PREDICATES:
  case FR_TYPE:
    return new EmptyCommand("Untreated role");
  }
  assert(false);// unreachable
  return NULL;
}

namespace tptp {
/**
 * Just exists to provide the uintptr_t constructor that ANTLR
 * requires.
 */
struct myExpr : public CVC4::Expr {
  myExpr() : CVC4::Expr() {}
  myExpr(void*) : CVC4::Expr() {}
  myExpr(const Expr& e) : CVC4::Expr(e) {}
  myExpr(const myExpr& e) : CVC4::Expr(e) {}
};/* struct myExpr */

enum NonAssoc {
  NA_IFF,
  NA_IMPLIES,
  NA_REVIMPLIES,
  NA_REVIFF,
  NA_REVOR,
  NA_REVAND,
};

}/* CVC4::parser::tptp namespace */


}/* CVC4::parser namespace */
}/* CVC4 namespace */

#endif /* __CVC4__PARSER__TPTP_INPUT_H */
