%{
#include "parser.h"
#include "lang.h"
#include "compiler.h"

#define YY_USER_ACTION  yyextra->size = yyleng; yyextra->col += yyleng;
%}

%option outfile="scanner.c" header-file="scanner.h"

%option warn
%option reentrant
%option noyywrap
%option yylineno
%option outfile="scanner.c" header-file="scanner.h"
%option extra-type="ExtraData*"

%x STR

%%

\-\-.* { /* munch up the comments */ }

"do"      { return DO_BLOCK; }
"end"     { return END_BLOCK; }
"if"      { return IF; }
"else"    { return ELSE; }
"elseif"  { return ELSE_IF; }
"break"   { return BREAK; }
"put"     { return PUT; }
"get"     { return GET; }
"send"    { return SEND; }
"print"   { return PRINT; }
"return"  { return RETURN; }
"while"   { return WHILE; }
"&&"      { return EXPR_AND; }
"||"      { return EXPR_OR; }
"theme"   { return THEME; }
"sprite:" { return SPRITE; }
"pad"     { return PAD; }

"i16"     { return I16; }
"i8"      { return I8; }
"i16s"    { return I16S; }
"i8s"     { return I8S; }
"Char"    { return CHAR; }
"Chars"    { return CHARS; }

"#"([0-9a-f]){3} { return COLOR_CODE; }
"$" { return GLOBAL; }

"(" { return OPEN_PAREN; }
")" { return CLOSE_PAREN; }
"[" { return OPEN_SQUARE_BRACKET; }
"]" { return CLOSE_SQUARE_BRACKET; }


"'" { return SINGLE_QUOTE; }
"fn" { return FN; }

"+"  { return PLUS; }
"-"  { return MINUS; }
"*"  { return TIMES; }
"."     { return DOT; }
"/"  { return SLASH; }
"%"  { return MODULO; }
"::" { return DEFINE; }
":" { return COLON; }
"="  { return EQUALS; }
"==" { return EQUALITY; }
"!=" { return NOT_EQUALITY; }
">"  { return GREATER_THAN; }
"<"  { return LESS_THAN; }
","  { return COMMA; }

\"\" { return EMPTY_STRING; }

\"   { BEGIN(STR); }
<STR>[^\"]+  { return STRING; }
<STR>\" { BEGIN(INITIAL); }

[0-9]+ { return INT_LITERAL; }
[a-zA-Z][a-zA-Z0-9_]* { return IDENTIFIER; }

.
\n { yyextra->line++; yyextra->col = 1; }

%%
