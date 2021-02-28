/* DO NOT REMOVE THIS LINE                      */
/*                                              */
/* Notation program                             */
/* @(#)lexer.l	3.9 (C) Henry Thomas   Release 3     Dated 12/10/91 */
/*                                              */
%{
#include <stdio.h>

#include "chesstype.h"
#include"notation.h"

#define NCURLINE 1024
char curline [NCURLINE] ;

#define LARGE_BUF 4096
static char commbuf[LARGE_BUF];

int column = 0;
int lineno = 1;

#ifdef __STDC__
static char * comment(int closing);
#else
static char * comment();
#endif

%}

LEXSHORT [-+!?#]
LEXLONG  [-+=/\\\^\&\*\~<>_|#]

%%
[Oo0]\-?[Oo0](\-?[Oo0])? { /* roque */ ; parse_roque(yytext); }
[0-9]+[\.]?	        { /* move number */ parse_number(yytext); }
[A-Z]?([a-z]([1-8])?)?[-xX]?[a-z][1-8]=?[A-Z]?  { /* move */  
  (void) parse_move(yytext);
  /*; fprintf(stderr,"%s, ",yytext);*/ 
}


{LEXLONG}{LEXLONG}{LEXLONG}     { /* comment */ ; 
				parse_comment(yytext); }
{LEXLONG}{LEXLONG}      { /* comment */ ; 
				   parse_comment(yytext); }
{LEXSHORT}{LEXSHORT}    { /* comment */ ; 
				     parse_comment(yytext); }
{LEXLONG}               { /* comment */ ; 
			   parse_comment(yytext); }
{LEXSHORT}              { /* comment */ ; parse_comment(yytext); }
"\(\.\)"                { /* comment */ ; parse_comment(yytext); }
"\(\*\)"                { /* comment */ ; parse_comment(yytext); }
"\(\^\)"                { /* comment */ ; parse_comment(yytext); }
"\[\+\]"                { /* comment */ ; parse_comment(yytext); }
"\(\)"                  { /* comment */ ; parse_comment(yytext); }
"\[\]"                  { /* comment */ ; parse_comment(yytext); }
"o\.\.o"                { /* comment */ ; parse_comment(yytext); }
"oo"                    { /* comment */ ; parse_comment(yytext); }
"o/o"                   { /* comment */ ; parse_comment(yytext); }
"@\^"                   { /* comment */ ; parse_comment(yytext); }
"etc"                   { /* comment */ ; parse_comment("etc"); }
"ep"|"e\.p\."           { /* comment */ ; parse_comment("ep"); }
\$[a-z][a-z][a-z]       { /* game comment */ ; parse_comment(yytext); }
@[a-z]+\{		{ /* keyword with arg */ 
  yytext[yyleng -1] = '\0' ;
  parse_keyword(yytext, comment('}'));
}
@[a-z]+			{ /* keyword without arg */ 
  parse_keyword(yytext,NULL); }
"\["			{ parse_text(comment(']')); }
"\("			{ parse_text(comment(')')); }
"\{"                    { /* enter variation */ ; enter_variation(); }
"\}"			{ /* close variation */ ; exit_variation(); }
[\,;\.]			{ /* skip , ; */ ; }
[ \t\v\n\f]             { /* skip blanks */; }
.                      { /* ignore bad characters */ (void) fprintf(stderr,"I don't understand: %s\n",yytext);}
%%

#ifdef FLEX_SCANNER
#undef yywrap
#endif
#ifdef __STDC__
int yywrap(void)
#else
int yywrap()
#endif
{
        return(1);
}


/* this procedure store comments in the text array commbuf
   Escape char are allowed for putting the end-of-comment symbol
   in the buffer
   */
#ifdef __STDC__
static char * comment(int closing)
#else
static char * comment(closing)
     char closing;
#endif
{
  register char c;
  register int i=0;

  while ( ((c = input()) != closing)  && (c != 0) && (c != EOF)) {
    commbuf[i] = c;
    if (i <LARGE_BUF) i++ ;
    
    if (c == '\\') {
      c = input() ;
      if (c == closing)
	commbuf[i-1] = c;
      else
	unput(c);
    }
  }
  commbuf[i] = '\0' ;
  return(commbuf);
}

#ifdef __STDC__
static void count(void)
#else
static void count()
#endif
{
  register int i;
  register int k;

  for (i = 0; yytext[i] != '\0'; i++) {
    if (yytext[i] == '\n') {
      column = 0;
      for (k = 0 ; k< NCURLINE; k++) /*PANDORE*/
        curline[k] = ' '; /*PANDORE*/
      lineno++;
    }
    else if (yytext[i] == '\t') {
      column += 8 - (column % 8);
      curline[column] = yytext[i];
    } else {
      column++;
      curline[column] = yytext[i];
    }
    curline[column+1]= '\0' ;
  }
  /*ECHO;*/
}
