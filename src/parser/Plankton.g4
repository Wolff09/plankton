grammar Plankton;


/***********************************************/
/************ Parser rules: programs ***********/
/***********************************************/

program : option* (
            structs+=structDecl
          | vars+=varDeclList ';'
          | funcs+=function
          | ctns+=containsPredicate
          | outf+=outflowPredicate
          | sinv+=sharedInvariant
          | ninv+=nodeInvariant
        )* EOF ;

//
// Options
//

option : '#' ident=Identifier str=String ;

//
// Types
//

structDecl : 'struct' name=Identifier '{' fieldDecl* '}' (';')? ;

type : BoolType             #typeBool
     | IntType              #typeInt
     | DataType             #typeData
     | name=Identifier '*'  #typePtr
     ;

typeList : VoidType
         | types+=type
         | '<' types+=type (',' types+=type)+ '>'
         ;

fieldDecl : type name=Identifier ';' ;

//
// Functions, statements, commands
//

varDecl : type name=Identifier ;

varDeclList : type names+=Identifier ( ',' names+=Identifier )* ;

function :          types=typeList name=Identifier args=argDeclList body=scope  #functionInterface
         | 'inline' types=typeList name=Identifier args=argDeclList body=scope  #functionMacro
         |          VoidType       '__init__'      '(' ')'          body=scope  #functionInit
         ;

argDeclList : '(' (args+=varDecl (',' args+=varDecl)*)? ')' ;

block : statement  #blockStmt
      | scope      #blockScope
      ;

scope : '{' (varDeclList ';')* statement* '}' ;

statement : 'choose' scope+                                               #stmtChoose
          | 'loop' scope                                                  #stmtLoop
          | 'atomic' body=block                                           #stmtAtomic
          | 'if' '(' expr=condition ')' bif=block ('else' belse=block)?   #stmtIf
          | 'while' '(' expr=logicCondition ')' body=block                #stmtWhile
          | 'do' body=block 'while' '(' expr=logicCondition ')' ';'       #stmtDo
          | command ';'                                                   #stmtCom
          ;

command : 'skip'                                            #cmdSkip
        | lhs=varList '=' rhs=exprList                      #cmdAssignStack
        | lhs=derefList '=' rhs=simpleExprList              #cmdAssignHeap
        | lhs=Identifier '=' rhs=logicCondition             #cmdAssignCondition
        | (lhs=Identifier '=')? cas                         #cmdCas
        | lhs=Identifier '=' 'malloc'                       #cmdMalloc
        | 'assume' '(' logicCondition ')'                   #cmdAssume
        | 'assert' '(' logicCondition ')'                   #cmdAssert
        | (lhs=varList '=')? name=Identifier args=argList   #cmdCall
        | 'break'                                           #cmdBreak
        | 'return'                                          #cmdReturnVoid
        | 'return' simpleExprList                           #cmdReturnList
        | 'return' logicCondition                           #cmdReturnExpr
        ;

cas : 'CAS' '(' dst=varList ',' cmp=simpleExprList ',' src=simpleExprList ')'    #casStack
    | 'CAS' '(' dst=derefList ',' cmp=simpleExprList ',' src=simpleExprList ')'  #casHeap
    ;

//
// Expressions, conditions
//

exprList : elems+=expression
         | '<' elems+=expression (',' elems+=expression)+ '>'
         ;

simpleExprList : elems+=simpleExpression
               | '<' elems+=simpleExpression (',' elems+=simpleExpression)+ '>'
               ;

varList : elems+=Identifier
        | '<' elems+=Identifier (',' elems+=Identifier)+ '>'
        ;

derefList : elems+=deref
          | '<' elems+=deref (',' elems+=deref)+ '>'
          ;

argList : '(' (elems+=simpleExpression (',' elems+=simpleExpression)*)? ')' ;

value : Null    #valueNull
      | True    #valueTrue
      | False   #valueFalse
      | Maxval  #valueMax
      | Minval  #valueMin
      ;

simpleExpression : name=Identifier  #exprSimpleIdentifier
                 | value            #exprSimpleValue
                 ;

deref : name=Identifier '->' field=Identifier;

expression : simpleExpression | deref ;

condition : logicCondition  #condLogic
          | cas             #condCas
          ;

simpleCondition : expression      #condSimpleExpr
                | Neg expression  #condSimpleNegation
                ;

binaryCondition : simpleCondition                              #condBinarySimple
                | lhs=simpleCondition Eq rhs=simpleCondition   #condBinaryEq
                | lhs=simpleCondition Neq rhs=simpleCondition  #condBinaryNeq
                | lhs=simpleCondition Lt rhs=simpleCondition   #condBinaryLt
                | lhs=simpleCondition Lte rhs=simpleCondition  #condBinaryLte
                | lhs=simpleCondition Gt rhs=simpleCondition   #condBinaryGt
                | lhs=simpleCondition Gte rhs=simpleCondition  #condBinaryGte
                ;

logicCondition : binaryCondition                                       #condLogicBinary
               | elems+=binaryCondition (And elems+=binaryCondition)+  #condLogicAnd
               | elems+=binaryCondition (Or elems+=binaryCondition)+   #condLogicOr
               ;

//
// Flows, invariants
//

containsPredicate : 'def' Contains
                    '(' nodeType=type nodeName=Identifier ',' valueType=type valueName=Identifier ')'
                    '{' formula '}'
                  ;

outflowPredicate : 'def' Outflow '[' field=Identifier ']'
                   '(' nodeType=type nodeName=Identifier ',' valueType=type valueName=Identifier ')'
                   '{' formula '}'
                 ;

sharedInvariant : 'def' Invariant '[' name=Identifier ']' '(' ')' '{' invariant '}'
                  ;

nodeInvariant : 'def' Invariant '[' isShared=Shared | isLocal=Local ']'
                '(' nodeType=type nodeName=Identifier ')'
                '{' invariant '}'
              ;


/***********************************************/
/************* Parser rules: logic *************/
/***********************************************/

formula : conjuncts+=axiom
        | '(' conjuncts+=axiom ')'
        | conjuncts+=axiom ( And conjuncts+=axiom )+
        | '(' conjuncts+=axiom ( And conjuncts+=axiom )+ ')'
        ;

axiom : binaryCondition                                                                           #axiomCondition
      | name=Identifier '->' FlowField Eq '0'                                                     #axiomFlowEmpty
      | name=Identifier '->' FlowField Neq '0'                                                    #axiomFlowNonEmpty
      | member=Identifier In name=Identifier '->' FlowField                                       #axiomFlowValue
      | '[' low=simpleExpression ',' high=simpleExpression ']' In name=Identifier '->' FlowField  #axiomFlowRange
      ;

separatingImplication : rhs=formula
                      | lhs=formula Imp rhs=formula
                      | '(' lhs=formula Imp rhs=formula ')'
                      ;

invariant : separatingImplication ( And separatingImplication )* ;


/***********************************************/
/***************** Lexer rules *****************/
/***********************************************/

Inline : 'inline' ;
Extern : 'extern' ;

VoidType : 'void' ;
DataType : 'data_t' ;
BoolType : 'bool' ;
IntType  : 'int' ;

Null   : 'NULL' ;
//Empty  : 'EMPTY' ;
True   : 'true' ;
False  : 'false' ;
//Ndet   : '*' ;
Maxval : 'MAX_VAL' ;
Minval : 'MIN_VAL' ;

Eq  : '==' ;
Neq : '!=' ;
Lt  : '<' ;
Lte : '<=' ;
Gt  : '>' ;
Gte : '>=' ;
And : '&&' ;
Or  : '||' ;
Imp : '=>' ;
Neg : '!' ;

Contains : '@contains';
Outflow : '@outflow';
Invariant : '@invariant';
Shared : 'shared';
Local : 'local';
In : 'in';
FlowField : '_flow';

Identifier : Letter ( ('-' | '_')* (Letter | '0'..'9') )* ;
fragment Letter : [a-zA-Z] ;
// Identifier : [a-zA-Z][a-zA-Z0-9_]* ;

String : '"' ~[\r\n]* '"'
       | '\'' ~[\r\n]* '\'' ;

WS    : [ \t\r\n]+ -> skip ; // skip spaces, tabs, newlines

COMMENT      : '/*' .*? '*/' -> channel(HIDDEN) ;
LINE_COMMENT : '//' ~[\r\n]* -> channel(HIDDEN) ;

// UNMATCHED          : .  -> channel(HIDDEN);
