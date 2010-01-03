%{
        #include <stdio.h>
        #include "printing.h"
        #include "lex.yy.c"
        #define YYDEBUG 0
        
        void yyerror(const char *str) 
        {
                Error("Error: %s\n.Could not parse policy file. Line %d, token %s\n",
                      str, yylineno, yytext);
        }
%}

%token LCARET RCARET LBRACKET RBRACKET LPAREN RPAREN EQUAL ALTERNATE
%token REPEAT CONJUNCT DASH COLON NEWLINE MASKBITS NAT ENDOFFILE

%start polfile

%%

 /* The entire policy file */
polfile : NAT pollist {return 0;}
        ;

/* The list of policies */
pollist : policy pollist
        | 
        ;

/* An individual policy in the list */
policy  : LCARET NAT COLON NAT RCARET EQUAL dnfl NEWLINE
        | LCARET NAT COLON NAT RCARET EQUAL dnfl CONJUNCT policy
        ;

/* The top level disjunctive list for a rule*/
dnfl    : cnfl ALTERNATE dnfl
        | cnfl 
        ;

/* The top level conjunctive list for a rule*/
cnfl    : cnf cnfl
        | cnf
        ;

/* A top level conjunctive term (must be in parens) */
cnf     : LPAREN altl RPAREN
        | LPAREN altl RPAREN REPEAT NAT
        ;

/* The alternation list */
altl    : alt ALTERNATE altl
        | alt
        ;

/* What can be alternated over */
alt     : LBRACKET NAT DASH NAT RBRACKET NAT
        | atconjl
        ;

/* The atomic conjunctive list (only repetition and conjunction allowed below
 * this level) */
atconjl : atcon atconjl
        | atcon
        ; 

/* An atomic conjunction list item */
atcon   : MASKBITS
        | LPAREN MASKBITS RPAREN REPEAT NAT
        ;
