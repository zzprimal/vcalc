grammar VCalc;

tokens {
  BLOCK,
  IF_BLOCK,
  LOOP_BLOCK,
  DECL,
  ASSIGN,
  PRINT,
  EXPR,
  INDEX,
  GENERATOR,
  FILTER
}

// parser rules
file: block EOF;

expr
    : '(' expr ')'
    | expr '[' expr ']'
    | expr DOTS expr
    | expr (DIV | MUL) expr
    | expr (ADD | SUB) expr
    | expr (LESS | GREATER) expr
    | expr (LOGEQ | LOGNEQ) expr
    | generator
    | filter
    | INT
    | ID
    ;

generator: '[' ID 'in' expr '|' expr ']';

filter: '[' ID 'in' expr '&' expr ']';

statement
    : declaration ';'
    | assignment ';'
    | if_stat ';'
    | loop ';'
    | print ';'
    ;

block : statement*;

declaration : TYPE ID ('=' expr)? ;

assignment : ID '=' expr;

if_stat : 'if' '(' expr ')' block 'fi';

loop: 'loop' '(' expr ')' block 'pool';

print: 'print(' expr ')';


// lexer rules
TYPE: 'int' | 'vector';
ID : ALPHA(ALPHA|DIGIT)*;
INT : DIGIT+;
MUL: '*';
DIV: '/';
ADD: '+';
SUB: '-';
LESS: '<';
GREATER: '>';
LOGEQ: '==';
LOGNEQ: '!=';
DOTS: '..';

fragment
ALPHA: [a-zA-Z];
fragment
DIGIT: [0-9];

// Skip whitespace
WS : [ \t\r\n]+ -> skip ;

// Comments
COMMENT:  '//' ~( '\r' | '\n' )* -> skip;
