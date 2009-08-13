%{

%}


%token ID

%%
expr:     
        | rule '=>' action '\n'
        | '\n'
        ; 

rule:     '[' ID ']' rule
        | alt '[' ID ']'
        | alt
        ;

alt:      seq par
        | par
        ;

par:      '0'
        | '1'
        | '?'
        ;

action:   'DROP'
        | 'ALLOW'
        ;
%%
