%option yylineno

%{

typedef YYTOKENTYPE enum {
        LCARET, RCARET, LBRACKET, RBRACKET, LPAREN, RPAREN, EQUAL, ALTERNATE,
        REPEAT, CONJUNCT, DASH, COLON, NEWLINE, MASKBITS, NAT, ENDOFFILE
};

%}

%%

"<"        {return LCARET;}
">"        {return RCARET;}
"["        {return LBRACKET;}
"]"        {return RBRACKET;}
"("        {return LPAREN;}
")"        {return RPAREN;}
"="        {return EQUAL;}
"|"        {return ALTERNATE;}
"^"        {return REPEAT;}
"&"        {return CONJUNCT;}
"-"        {return DASH;}
":"        {return COLON;}
"\n"       {return NEWLINE;}
"#"        {char c = input();
            while(c != '#' && c != EOF){
                c = input();
            }
           }

[Ol?]+     {return MASKBITS;}
[0-9]+     {return NAT;}
<<EOF>>    {return ENDOFFILE;}
[ \t\r]+   { /* insignificant whitespace */ }

.         {
             Error("Invalid character in policy file on line %d : '%c'\n",
                   yylineno, yytext);
          }

%%
