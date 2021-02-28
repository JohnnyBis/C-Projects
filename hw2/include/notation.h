/*
  Notation program
  @(#)notation.h	3.9 (C) Henry Thomas   Release 3     Dated 12/10/91
 */
#ifndef _NOTATION_HEADERS
#define _NOTATION_HEADERS


/* ---- KEYWORDS ----  */

/* prefix for keywords */
#define PREFIX '@'


#define START     0
#define CLEAR     1
#define SHOWBOARD 2
#define TOWHITE   3
#define TOBLACK   4
#define CONFIGWH  5
#define CONFIGBL  6
#define DEFAULTP  7
#define TITLE	  8
#define SUBTITLE  9
#define SCORE	  10
#define LANGUE    11
#define SPECIAL   12
#define KNULL     13

#define NBKEYWORD (KNULL+1)


/* ---- LANGUAGES ---- */

#define FRENCH 		0
#define ENGLISH 	1
#define ITALIAN 	2
#define SPANISH 	3
#define GERMAN  	4
#define DUTCH   	5
#define CZECH		6
#define HUNGARIAN      	7
#define POLISH		8
#define ROMANIAN       	9
#define FIDE		10

#define USERDEF		11

#define NBLANGUAGES 	(USERDEF+0)

#ifndef DEFAULT_INPUT_LANGUAGE
#define DEFAULT_INPUT_LANGUAGE  FRENCH
#endif

#ifndef DEFAULT_OUTPUT_LANGUAGE
#define DEFAULT_OUTPUT_LANGUAGE FRENCH
#endif


/* a convenient way  to get a value for NUM_COMMENT 
   look at GNU-CC source, it's full of *neat* tricks 
   */

#define CHESSSYMB(LET,LASC,SASC,TEX,PS,ENG,FRA) LET,

enum com_code {
#include "chesssymb.def"
  LAST_UNUSED_NUM_COM_CODE
  };
#undef CHESSSYMB

#define NUM_COM_CODE ((int) LAST_UNUSED_NUM_COM_CODE) 

/* ---- MISC TABLES ---- */

/* roque ascii table */
#define NBROQUE     6
#define SPETITROQUE 0
#define SGRANDROQUE 1


/* ---- DRIVERS ---- */

/* output drivers name table */
#define DEFAULT_DRIVER D_ASCII

/* ---- IO ---- */

#ifndef LIB_DIR
#define LIB_DIR    "./lib/"
#endif

#define HELP_FILE  "notation.hlp"

extern FILE * infile;

/* ------------- service routines -------------------*/

/* Output an error message and exit */
#define fatal(A) (void) fprintf A , close_files() , exit(1)

/* Output an error message and set the error flag */
#define error(A) (void) fprintf A , error_flag = TRUE , (void)fflush(stderr)

/* Output an error message and set the error flag */
#define message(A) (void) fprintf A 

#define MALLOC(T)  (T *)malloc((unsigned)sizeof(T))
#define ALLOCP(P)  if ( (P) == NULL ) { fatal((stderr,"malloc failed")) ; };

/* ------- conversion of input character --------------------- */

#define lettertocol(c)  ((c)-'a'+1)
#define lettertolig(c)  ((c)-'1'+1)
#define coltoletter(n)  ((n)-1+'a')
#define ligtoletter(n)  ((n)-1+'1')


/* ------- variables and functions --------------------------- */
extern char *version_string;
extern int configuring;
extern int configside;
extern char *in_table;
extern char *out_table;
extern char *c_roque[];
extern char *c_en_passant[];
extern char c_prise;
extern char *c_comments[];
extern int error_flag;

extern char *com_short[];
extern char *com_long[];

/* ---------- service routines ---------- */

#ifdef __STDC__
extern void clear_board(game *g);
extern game *new_board(void);
extern game *copy_board(game *from, game *to);
extern void init_board(game *tgm);
extern depl *new_move(void);
extern void init_move(depl *m);
extern depl *copy_move(depl *from, depl *to);
extern void do_move(game *g,depl *m);
extern void undo_move(game *g,depl *m);
extern void enter_variation(void);
extern void exit_variation(void);
#else
extern void clear_board(/*game *g*/);
extern game *new_board(/*void*/);
extern game *copy_board(/*game *from, game *to*/);
extern void init_board(/*game *tgm*/);
extern depl *new_move(/*void*/);
extern void init_move(/*depl *m*/);
extern depl *copy_move(/*depl *from, depl *to*/);
extern void do_move(/*game *g,depl *m*/);
extern void undo_move(/*game *g,depl *m*/);
extern void enter_variation(/*void*/);
extern void exit_variation(/*void*/);
#endif

/* ------ parse and analysis routines ------ */

#ifdef __STDC__
extern int in_board(int l, int c);
extern int path_free(int l1, int c1, int l2, int c2);
extern int check_roque(void);
extern int guess_piece(void);
extern int guess_depl(int nb, int tab[][2], int *pl1, int *pc1, int l2, int c2, int path);
extern int ambiguity(depl *d, int * amline, int *amcols);
extern int check_move(depl *m);
extern int guess_move(void);
extern int clear_pos(int lig, int col);
extern int configure(void);
extern int execute_move(void);
extern int typechar(char c);
extern int execute(int num, char c);
extern int parse_number(char *token);
extern int parse_text(char *text);
extern int parse_comment(char *com);
extern int parse_keyword(char *token, char *text);
extern int parse_roque(char *token);
extern int parse_move(char *token);
extern void init_parse(depl *m);
#else
extern int in_board(/*int l, int c*/);
extern int path_free(/*int l1, int c1, int l2, int c2*/);
extern int check_roque(/*void*/);
extern int guess_piece(/*void*/);
extern int guess_depl(/*int nb, int tab[][2], int *pl1, int *pc1, int l2, int c2, int path*/);
extern int ambiguity(/*depl *d ...*/);
extern int check_move(/*depl *m*/);
extern int guess_move(/*void*/);
extern int clear_pos(/*int lig, int col*/);
extern int configure(/*void*/);
extern int execute_move(/*void*/);
extern int typechar(/*char c*/);
extern int execute(/*int num, char c*/);
extern int parse_number(/*char *token*/);
extern int parse_text(/*char *text*/);
extern int parse_comment(/*char *com*/);
extern int parse_keyword(/*char *token, char *text*/);
extern int parse_roque(/*char *token*/);
extern int parse_move(/*char *token*/);
extern void init_parse(/*depl *m*/);
#endif

/* ------ top-level interface functions -------- */

#ifdef __STDC__
extern int parse_options(int argc, char *argv[]);
extern void close_files(void);
extern int associe_traduction(char **table, int langage);

#else
extern int parse_options(/*int argc, char *argv[]*/);
extern void close_files(/*void*/);
extern int associe_traduction(/*char **table, int langage*/);

#endif


#endif
