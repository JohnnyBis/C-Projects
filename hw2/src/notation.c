/*
  Notation program
  @(#)notation.c	3.9 (C) Henry Thomas   Release 3     Dated 12/10/91
 */
/* Programme d'analyse de notation echiquienne
   Copyright (C) 1990 Henry Thomas
   Nom: notation
   Auteur: Henry Thomas
   Date: 27/11/90
/*
This file is part of NOTATION program.

NOTATION is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

NOTATION is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with NOTATION; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* --------------------- data part ---------------------- */

/* les tableaux suivants sont les tables de transcription de notation
   selon les langages
   */
#ifdef __STDC__
#include <stdlib.h>
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "chesstype.h"
#include "notation.h"
#include "drivers.h"
#include "lexer.h"

extern void close_files();

char * version_string =
  "@(#)notation.c	3.9 (C) Henry Thomas.   Release 3     Dated 12/10/91";

static char * keywords[]= {
  "@startplay" , "@clearboard" , "@showboard" ,
  "@whitesmove", "@blacksmove", "@configwhite", "@configblack" ,
  "@default" , 
  /* these are keywords with arguments */
  "@title", "@subtitle", "@score", "@language",
  /* "special" keyword */
  "@special", 
  /* null and final keyword */
  "@null"
  };

int configuring = FALSE ;
int configside = 0 ;


static char * t_language[] = {
  "french", "english", "italian", "spanish", "german", "dutch",
  "czech",  "hungarian","polish", "romanian", "FIDE"
};

static int in_language = DEFAULT_INPUT_LANGUAGE ;
static int out_language = DEFAULT_OUTPUT_LANGUAGE ;

static char c_language[NBLANGUAGES][7] = {
/* french    */  { '@' ,'R' , 'D' , 'T' , 'F' , 'C' , 'P' },
/* english   */  { '@' ,'K' , 'Q' , 'R' , 'B' , 'N' , 'P' },
/* italian   */  { '@' ,'R' , 'D' , 'T' , 'A' , 'C' , 'P' },
/* spanish   */  { '@' ,'R' , 'D' , 'T' , 'A' , 'C' , 'P' },
/* german    */  { '@' ,'K' , 'D' , 'T' , 'L' , 'S' , 'B' },
/* dutch     */  { '@' ,'K' , 'D' , 'T' , 'L' , 'P' , 'O' },
/* czech     */  { '@' ,'K' , 'D' , 'V' , 'S' , 'J' , 'P' },
/* hungarian */  { '@' ,'K' , 'V' , 'B' , 'F' , 'H' , 'G' },
/* polish    */  { '@' ,'K' , 'H' , 'W' , 'G' , 'S' , 'P' },
/* romanian  */  { '@' ,'R' , 'D' , 'T' , 'N' , 'C' , 'P' },
/* FIDE      */  { '@' ,'K' , 'D' , 'T' , 'S' , 'N' , 'P' }
/* UNIMPLEMENTED ... */
/* user_def  *//*{ '@' ,'X' , 'X' , 'X' , 'X' , 'X' , 'X' }*/
/* russian not implemented : ASCII russian is an oxymoron */
/* russian   *//*{ '@' ,'K' , 'F' , 'D' , 'C' , 'K' , 'P' }*/
	       };

/* input translation table */
char *in_table;
 

char *  c_roque[] = { "O-O" , "O-O-O" , "o-o" , "o-o-o" , "0-0" , "0-0-0" };

/* various notations for en passant */
#define N_EP 2
char * c_en_passant[] = { "ep" , "e.p." } ;


/* notation for catch */
char c_prise ='x';

/* various comments */
char * c_comments[] = { "+" , "++" , 
			  "?" , "??", "!", "!!", "!?", "?!",
			  "mate", "draw" };

/* movement tables */
/* move only */
/* white pawn, move */
#define NB_M_PAWN_MOVE_WD 2
static int m_pawn_move_wd [][2] = {
  { 1, 0}, {2, 0}
};

/* black pawn, move */
#define NB_M_PAWN_MOVE_BD 2
static int m_pawn_move_bd [][2] = {
  {-1, 0}, {-2, 0}
};

/* TRICK = we have added the catching move at the end of
   the non catching ones; so in check_depl, we try first 
   the non catching one and then the catching one.
   So, even if catching (x) is non indicated in the input, 
   we succeed in guessing the move
   */
/* white pawn, move */
/*#define NB_M_PAWN_WD 2*/
#define NB_M_PAWN_WD 4
static int m_pawn_wd [][2] = {
  { 1, 0}, {2, 0},
/* catch... */
  { 1, 1}, { 1,-1}
};

/* white pawn, catch */
#define NB_M_PAWN_WX 2
static int m_pawn_wx [][2] = {
  { 1, 1}, { 1,-1}
};

/* black pawn, move */
/*#define NB_M_PAWN_BD 2*/
#define NB_M_PAWN_BD 4
static int m_pawn_bd [][2] = {
  {-1, 0}, {-2, 0},
/* catch... */
  {-1, 1}, {-1,-1} 
};

/* black pawn, catch */
#define NB_M_PAWN_BX 2
static int m_pawn_bx [][2] = {
  {-1, 1}, {-1,-1} 
};


#define NB_M_KNIGHT  8
static int m_knight[][2] = { 
  { 2, 1}, { 2,-1}, {-2, 1}, {-2,-1},
  { 1, 2}, { 1,-2}, {-1, 2}, {-1,-2}
};

#define NB_M_BISHOP 28
static int m_bishop[][2] = {
  { 7, 7},  {6, 6}, { 5, 5}, { 4, 4}, { 3, 3}, { 2, 2}, { 1, 1},
  { 7,-7}, { 6,-6}, { 5,-5}, { 4,-4}, { 3,-3}, { 2,-2}, { 1,-1},
  {-7,-7}, {-6,-6}, {-5,-5}, {-4,-4}, {-3,-3}, {-2,-2}, {-1,-1},
  {-7, 7}, {-6, 6}, {-5, 5}, {-4, 4}, {-3, 3}, {-2, 2}, {-1, 1}
};

#define NB_M_ROOK 28
static int m_rook[][2] = {
  { 7, 0}, { 6, 0}, { 5, 0}, { 4, 0}, { 3, 0}, { 2, 0}, { 1, 0},
  {-7, 0}, {-6, 0}, {-5, 0}, {-4, 0}, {-3, 0}, {-2, 0}, {-1, 0},
  { 0, 7}, { 0, 6}, { 0, 5}, { 0, 4}, { 0, 3}, { 0, 2}, { 0, 1},
  { 0,-7}, { 0,-6}, { 0,-5}, { 0,-4}, { 0,-3}, { 0,-2}, { 0,-1}
};

#define NB_M_QUEEN 56
static int m_queen[][2] = {
  { 7, 7},  {6, 6}, { 5, 5}, { 4, 4}, { 3, 3}, { 2, 2}, { 1, 1},
  { 7,-7}, { 6,-6}, { 5,-5}, { 4,-4}, { 3,-3}, { 2,-2}, { 1,-1},
  {-7,-7}, {-6,-6}, {-5,-5}, {-4,-4}, {-3,-3}, {-2,-2}, {-1,-1},
  {-7, 7}, {-6, 6}, {-5, 5}, {-4, 4}, {-3, 3}, {-2, 2}, {-1, 1},
  { 7, 0}, { 6, 0}, { 5, 0}, { 4, 0}, { 3, 0}, { 2, 0}, { 1, 0},
  {-7, 0}, {-6, 0}, {-5, 0}, {-4, 0}, {-3, 0}, {-2, 0}, {-1, 0},
  { 0, 7}, { 0, 6}, { 0, 5}, { 0, 4}, { 0, 3}, { 0, 2}, { 0, 1},
  { 0,-7}, { 0,-6}, { 0,-5}, { 0,-4}, { 0,-3}, { 0,-2}, { 0,-1}
};

#define NB_M_KING 8
static int m_king[][2] = {
  { 1, 1}, { 1, 0}, { 1,-1},
  {-1, 1}, {-1, 0}, {-1,-1},
  { 0, 1}, { 0, -1}
};


/* I/O */
FILE * infile ;
FILE * fhelp;

static char * t_output[] = 
{ "ascii", "postscript", "tex", "roff", "xchess", "gnu" };

/* stack -- used for variation */

/* stack element */
typedef struct {
  depl * d;
  game * b;
  
  /* we don't stack drivers, but only to variables */
  int d1,d2; /* iswhiteturn and interrupt */
} stack_elt ;

/* size of the stack
   0 = ordinary play
   1 = level 1 variation
   2 = level 2 variation
   3 = level 3 variation
*/
#define VARIATION_MAX 3

/* the stack itself */
static stack_elt stack[VARIATION_MAX];

/* top of the stack */
/* --> explicit in dr->variation */


/* ---------- automata definitions --------- */
/* table for syntaxic analysis of move */

#define FINAL	10
#define TML 	FINAL   /* terminal state */
#define NBETAT 	11
#define NBCLAS 	8

/* successor of state */
static int transit[NBETAT][NBCLAS] = { 
/*   P a-h 1-8   -   x   =  \0   ? */ 
/*(  0   1   2   3   4   5   6   7)*/
  {  1,  2, -1, -1, -1, -1, -1, -1 }, /* etat  0 */
  { -1,  2, -1, -1,  4, -1, -1, -1 }, /* etat  1 */
  { -1,  6,  3,  4,  4,  8,TML,TML }, /* etat  2 */
  { -1,  6, -1,  4,  4,  8,TML,TML }, /* etat  3 */
  {  5,  6, -1, -1, -1, -1, -1, -1 }, /* etat  4 */
  { -1,  6, -1, -1, -1, -1, -1, -1 }, /* etat  5 */
  { -1, -1,  7, -1, -1, -1, -1, -1 }, /* etat  6 */
  { -1, -1, -1, -1, -1,  8,TML,TML }, /* etat  7 */
  {  9, -1, -1, -1, -1, -1,TML, -1 }, /* etat  8 */
  { -1, -1, -1, -1, -1, -1,TML,TML }, /* etat  9 */
  { -1, -1, -1, -1, -1, -1, -1, -1 }  /* etat 10 == terminal */
};

/* actions to do */
static int action[NBETAT][NBCLAS] = {
/*   P a-h 1-8   -   x   =  \0   ? */ 
  {  1,  2, -1, -1, -1, -1, -1, -1 }, /* etat  0 */
  { -1,  2, -1, -1, 10, -1, -1, -1 }, /* etat  1 */
  { -1, 13,  3,  4,  5, 14,  6,  7 }, /* etat  2 */
  { -1, 13, -1,  4,  5, 14,  6,  7 }, /* etat  3 */
  {  1,  2, -1, -1, -1, -1, -1, -1 }, /* etat  4 */
  { -1,  2, -1, -1, -1, -1, -1, -1 }, /* etat  5 */
  { -1, -1,  3, -1, -1, -1, -1, -1 }, /* etat  6 */
  { -1, -1, -1, -1, -1, 14,  8,  9 }, /* etat  7 */
  { 15, -1, -1, -1, -1, -1, 17, -1 }, /* etat  8 */
  { -1, -1, -1, -1, -1, -1, 17, 17 }, /* etat  9 */
  { -1, -1, -1, -1, -1, -1, -1, -1 }  /* etat 10 */
};


/* the complete play */
play * theplay ;

/* current game
   the name "tos" means "top of stack"
   */
static game * tos = GULL ;

/* variable holding current move */
static depl * m = MULL ;


int alternate_moves[10][2]; /* table of alternate moves, guessed by
			       the "move generator": guess depl
			       */


/* the output driver */
static format * dr;

static int driver; /* driver type, ie gnu, ascii ... */

static int movecount;

/* current move, used by the parser */
static int curpiece,  curcol,  curlig ;
static int curdigit, curmove;

/* booleen d'erreur */
int error_flag = FALSE;

/* move to display board */
static int count = 0 ;

static int move_to_display[NB_MOVE_TO_DISP] ;
static int nb_move_to_dsp = 0;
static int stop_at_display = FALSE;

/* short and long form comment table */
char * com_short[] = {
#define CHESSSYMB(LET,LASC,SASC,TEX,PS,ENG,FRA) SASC,
#include "chesssymb.def"
 ""
 };
#undef CHESSSYMB

char * com_long[] = {
#define CHESSSYMB(LET,LASC,SASC,TEX,PS,ENG,FRA) LASC,
#include "chesssymb.def"
 ""
 };
#undef CHESSSYMB



#define setboard(A,I,J,P,C)  { (A)->board[(I)][(J)] = (P) ; \
				 (A)->color[(I)][(J)] = (C); }
#define clsboard(A,I,J)   { (A)->board[(I)][(J)] = VOID ; \
				(A)->color[(I)][(J)] = VOID ;}

/* --------------------------- code part --------------------- */


#ifdef __STDC__
static int ispiece(char c)
#else
static int ispiece(c)
     char c;
#endif
{
  register int i;
  
  for ( i = 0 ; (i < NUMPIECES) && (c != in_table[i]) ; i++ ) ;
  /*(void) fprintf(stdout, "piece %d %c\n" , i , c);*/
  return(i<NUMPIECES);
}


#ifdef __STDC__
static int piece(char c)
#else
static int piece(c)
     char c ;
#endif
{
  register int i;
  
  for ( i = 0 ; (i < NUMPIECES) && (c != in_table[i]) ; i++ ) ;
  if ( i== NUMPIECES)
    i = PAWN ;
  return(i);
}

/* this function returns the rank of a keyword in a given table.
   if key is not present, it returns the default value
   */
#ifdef __STDC__
static int find_keyword(char *tab[], int nbentry,int defaut,
			char *key,int warning)
#else
static int find_keyword(tab, nbentry,defaut,key,warning)
     char * tab[]; /* the table to look in */
     int nbentry;  /* number of entries */
     int defaut;  /* the default value to return if search failed */
     char *key;    /* the key to find */
     int warning;  /* do we display a warning ? */
#endif
{
  int i ;

  for(i=0; (i< nbentry) ;i++)
    if (strcmp(tab[i],key))
      return(i);

  /* we failed to find the keyword */
  if (warning)
    (void) fprintf (stderr, "unknow keyword %s in this context\n",key);
  return(defaut);
}

/* ---------- board management function ------------- */

#ifdef __STDC__
void clear_board(game *g)
#else
void clear_board(g)
     game *g;
#endif
{
  register int i,j;

  for (i=0; i < 10; i++ )
    for (j=0 ; j< 10 ; j++) {
      g->board[i][j] = VOID;
      g->color[i][j] = VOID;
    }
}

#ifdef __STDC__
game * new_board(void)
#else
game * new_board()
#endif
{
  game * tmp;
  int i; 

  tmp = (game *) malloc (sizeof(game));
  ALLOCP(tmp);
  for (i=0; i < ((sizeof (game))/ sizeof (int)) ; i++)
    ((int *) tmp)[i] = 0;
  return(tmp);
}

#ifdef __STDC__
game * copy_board(game *from, game *to)
#else
game * copy_board(from, to)
     game * from;
     game * to;
#endif
{
  int i; 

  for (i=0; i < ((sizeof (game))/ sizeof (int)) ; i++)
    ((int *) to)[i] =  ((int *) from)[i] ;
  return(to);
}

#ifdef __STDC__
void init_board(game *tgm)
#else
void init_board(tgm)
  game * tgm;
#endif
{
  register int i,j;

  clear_board(tgm);

  for (i=1; i< 9 ; i=i+7) {
    tgm->board[i][1]= tgm->board[i][8] = ROOK ;
    tgm->board[i][2]= tgm->board[i][7] = KNIGHT ;
    tgm->board[i][3]= tgm->board[i][6] = BISHOP ;
    tgm->board[i][4]= QUEEN;
    tgm->board[i][5]= KING;
  }
  for (i=2; i< 8 ; i=i+5) 
    for (j=1; j <=8 ; j++)
      tgm->board[i][j] = PAWN;

  for (i=1; i <=2; i++)
    for (j=1; j <=8 ; j++) {
      tgm->color[i][j] = WHITE;
      tgm->color[i+6][j] = BLACK ;
    }
}

#ifdef __STDC__
depl * new_move(void)
#else
depl * new_move()
#endif
{
  depl * tmp;
  int i; 
  static int counter = 0;

  tmp = (depl *) malloc (sizeof(depl *));
  ALLOCP(tmp);
  for (i=0; i < ((sizeof (depl))/ sizeof (int)) ; i++)
    ((int *) tmp)[i] = 0;
  tmp->uid = ++counter;
  tmp->whiteturn = FALSE;
  tmp->move = 0;
  return(tmp);
}


#ifdef __STDC__
void init_move(depl *m)
#else
void init_move(m)
     depl *m;
#endif
{
  m->move= 1 ;
  m->whiteturn = TRUE ;
}

#ifdef __STDC__
depl * copy_move(depl *from,depl *to)
#else
depl * copy_move(from,to)
     depl * from;
     depl * to ;
#endif
{
  int i; 

  for (i=0; i < ((sizeof (depl))/ sizeof (int)) ; i++)
    ((int *) to)[i] = ((int *) from)[i];

  return(to);
}

/* add a new move as successor to the move m */
#ifdef __STDC__
depl * add_trailing_move(depl *mo)
#else
depl * add_trailing_move(mo)
     depl * mo;
#endif
{
  mo->next = new_move();

  mo->next->prev = mo;
  mo->next->next = (depl *) NULL;
  mo->next->sub  = (depl *) NULL;

  mo->next->whiteturn = !( m->whiteturn ) ;
  mo->next->move = mo->move;
  if ( mo->next->whiteturn) {
    mo->next->move++;
  }

  return(mo->next);
}

#ifdef __STDC__
static depl * add_variation(depl *mo)
#else
static depl * add_variation(mo)
     depl * mo;
#endif
{
  depl *ip ; /* insertion point */

  ip = mo ;
  while (ip->sub != (depl *) NULL )
    ip = ip->sub ;
  
  ip->sub = new_move();

  ip->sub->prev = mo;
  ip->sub->next = (depl *) NULL;
  ip->sub->sub  = (depl *) NULL;
  
  /* as we have a fictif element heading our list, 
     ( generated by add_trailing_move() )
     we have to go back in the numbering */
  ip->sub->whiteturn =  mo->prev->whiteturn  ;
  ip->sub->move = mo->prev->move ;

  return(ip->sub);
}


#ifdef __STDC__
static void free_move_list(depl *d)
#else
static void free_move_list(d)
     depl * d;
#endif
{  

  if (d->next != (depl *) NULL) {
    free_move_list(d->next);
    free(d->next);
    d->next = (depl *) NULL;
  }
  if (d->sub != (depl *) NULL) {
    free_move_list(d->sub);
    free(d->sub);
    d->sub = (depl *) NULL;
  }
}
  
/* procedure upadate borad g with move m */
#ifdef __STDC__
void do_move(game *g,depl *m)
#else
void do_move(g,m)
     game *g;
     depl *m;
#endif
{
  switch (m->type) {
  case MOVE:
    setboard(g,m->tolig,m->tocol,m->piece,CURCOLOR(m)) ;
    clsboard(g,m->fromlig,m->fromcol) ;
    break;
  case PRISE:
    setboard(g,m->tolig,m->tocol,m->piece,CURCOLOR(m)) ;
    clsboard(g,m->fromlig,m->fromcol);
    break;
  case GRANDROQUE:
    if (m->whiteturn)
      m->fromlig = 1;
    else 
      m->fromlig = 8;
    setboard(g,m->fromlig,3,KING,CURCOLOR(m)) ;
    setboard(g,m->fromlig,4,ROOK,CURCOLOR(m)) ;
    clsboard(g,m->fromlig,1) ;
    clsboard(g,m->fromlig,5) ;
    break;
  case PETITROQUE:
    if (m->whiteturn)
      m->fromlig = 1;
    else 
      m->fromlig = 8;
    setboard(g,m->fromlig,7,KING,CURCOLOR(m)) ;
    setboard(g,m->fromlig,6,ROOK,CURCOLOR(m)) ;
    clsboard(g,m->fromlig,5) ;
    clsboard(g,m->fromlig,8) ;
    break;
  case EN_PASSANT:
    setboard(g,m->tolig,m->tocol,m->piece,CURCOLOR(m)) ;
    clsboard(g,m->tolig,m->fromcol) ;
    clsboard(g,m->fromlig,m->fromcol) ;
    break;
  case PROMOTION:
    setboard(g,m->tolig,m->tocol,m->piece,CURCOLOR(m)) ;
    clsboard(g,m->fromlig,m->fromcol);
    break;
  case PROM_ET_PRISE:
    setboard(g,m->tolig,m->tocol,m->piece,CURCOLOR(m)) ;
    clsboard(g,m->fromlig,m->fromcol);
    break;
  default:
    fprintf(stderr,"\nUnable to do move: unknown move type\n");
    break;
  }
}



/* this procedure undo the effect of move m on the board g */
#ifdef __STDC__
void undo_move(game *g,depl *m)
#else
void undo_move(g,m)
     game *g;
     depl *m;
#endif
{
  switch (m->type) {
  case MOVE:
    clsboard(g,m->tolig,m->tocol) ;
    setboard(g,m->fromlig,m->fromcol,m->piece,CURCOLOR(m)) ;
    break;
  case PRISE:
    setboard(g,m->tolig,m->tocol,m->prise,OPPCOLOR(m)) ;
    setboard(g,m->fromlig,m->fromcol,m->piece,CURCOLOR(m)) ;
    break;
  case GRANDROQUE:
    if (m->whiteturn)
      m->fromlig = 1;
    else 
      m->fromlig = 8;
    clsboard(g,m->fromlig,3) ;
    clsboard(g,m->fromlig,4) ;
    setboard(g,m->fromlig,5,KING,CURCOLOR(m)) ;
    setboard(g,m->fromlig,1,ROOK,CURCOLOR(m)) ;
    break;
  case PETITROQUE:
    if (m->whiteturn)
      m->fromlig = 1;
    else 
      m->fromlig = 8;
    clsboard(g,m->fromlig,6) ;
    clsboard(g,m->fromlig,7) ;
    setboard(g,m->fromlig,5,KING,CURCOLOR(m)) ;
    setboard(g,m->fromlig,8,ROOK,CURCOLOR(m)) ;
    break;
  case EN_PASSANT:
    clsboard(g,m->tolig,m->tocol) ;
    setboard(g,m->tolig,m->fromcol,PAWN,OPPCOLOR(m)) ;
    setboard(g,m->fromlig,m->fromcol,m->piece,CURCOLOR(m)) ;
    break;
  case PROMOTION:
    clsboard(g,m->tolig,m->tocol);
    setboard(g,m->fromlig,m->fromcol,PAWN,CURCOLOR(m)) ;
    break;
  case PROM_ET_PRISE:
    setboard(g,m->tolig,m->tocol,m->prise,OPPCOLOR(m)) ;
    setboard(g,m->fromlig,m->fromcol,PAWN,CURCOLOR(m)) ;
    break;
  default:
    fprintf(stderr,"\nUnable to undo move: unknown move type\n");
    break;
  }
}

/* variation procedures == stack manipulation */

#ifdef __STDC__
void enter_variation(void)
#else
void enter_variation()
#endif
{
  int l;

  l = dr->variation ;
  
  if (l >= VARIATION_MAX) {
    error((stderr,"\nMaximum imbricated variation is %d\n",VARIATION_MAX));
  } else {
    /* save current line/variation */
    stack[l].d = m;
    stack[l].b = tos;
    stack[l].d1 = dr->iswhiteturn;
    stack[l].d2 = dr->interrupt = TRUE ;    
    /* create new */
    tos = new_board();
    (void) copy_board(stack[l].b, tos);

    /* A variation FOLLOWS the main line 
       so we need to backtrack one move
       */
    m = add_variation(stack[l].d);
    undo_move(tos,stack[l].d);

    /* set variables */
    l++;
    dr->variation = l;

    output_variation(dr,VARIATION_IN);
  }
}

#ifdef __STDC__
void exit_variation(void)
#else
void exit_variation()
#endif
{
  int l ;

  l = dr->variation ;
  
  if (l == 0) {
    error((stderr,"\nYou cannot exit from the main line (a variation level error?)\n"));
  } else {
    output_variation(dr,VARIATION_OUT);

    l--;
    free(m);
    m = stack[l].d ;
    tos = stack[l].b ;

    dr->iswhiteturn = stack[l].d1 ;
    dr->interrupt = stack[l].d2 ;
    dr->variation = l;
  }
}

/* ----------- semantic evaluation of move ----------- */
/* check if  position lies within the board
   */
#ifdef __STDC__
int in_board(int l,int c)
#else
int in_board(l,c)
     int l,c;
#endif
{
  return ((c >= 1) && (c <= 8) && (l >= 1) && (l <= 8));
}

/* check that the path from pos1 to pos2 is free
   */
#ifdef __STDC__
int path_free(int l1,int c1,int l2,int c2)
#else
int path_free(l1, c1, l2, c2)
int l1,c1, l2, c2;
#endif
{
  int li = 1 ;
  int ci = 1 ;
  int lig, col;


  li = SIGN(l2-l1);
  ci = SIGN(c2-c1);


  if ( c1 == c2 ) {    
    col = c1;
    for (lig = l1 +li; lig != l2 ; lig +=li)
      if (tos->board[lig][col] != VOID)
	return (FALSE);
    return(TRUE);
  }

  if ( l1 == l2) {
    lig = l1 ;
    for (col = c1 + ci; col != c2 ; col +=ci)
      if (tos->board[lig][col] != VOID)
	return (FALSE);
    return(TRUE);
  }

  for (lig = l1+li,col =c1+ci; (lig!=l2) && (col!=c2); lig+=li, col+= ci)
    if (tos->board[lig][col] != VOID) {
      return (FALSE);
    }
  return(TRUE);
}

/* check roque is possible */
#ifdef __STDC__
int check_roque(void)
#else
int check_roque()
#endif
{
  int lig, col ;

  if (m->whiteturn)
    lig = 1 ;
  else
    lig =8;
  if (m->type == GRANDROQUE)
    for (col = 2; col < 5 ; col++)
      if (tos->board[lig][col] != VOID)
	return(FALSE);
  if (m->type == PETITROQUE)
    for (col = 6; col < 7 ; col++)
      if (tos->board[lig][col] != VOID)
	return(FALSE);
  return(TRUE);
}
  
/* check -- or guess -- where a given piece come */
#ifdef __STDC__
int guess_piece(void) 
#else
int guess_piece() 
#endif
{
  return(tos->board[m->fromlig][m->fromcol]); 
}

/* try to guess the move -- low-level function 
   it returns -- in the parms -- the coordinates of a possible move
   it returns -- as value -- the number of possible move
 */
#ifdef __STDC__
int guess_depl(int nb, int tab[][2],
	       int * pl1, int * pc1, int l2, int c2, int path)
#else
int guess_depl(nb, tab, pl1, pc1, l2,c2,path)
     int nb;
     int tab[10][2];
     int *pl1, *pc1;
     int l2,c2;
     int path; /* tell if we have to check for a free path 
		  used for en passant */
#endif
{
  int i, c, l;
  int count = 0;

  for (i=0; i< nb; i++ ) {
    l = l2 - tab[i][0];
    c = c2 - tab[i][1];
    if (in_board(l,c))
      if ((tos->board[l][c] == m->piece) &&
	  (tos->color[l][c] == CURCOLOR(m)) &&
	  ( !path || (path && path_free(l,c, l2, c2))) &&
	  ( ((*pl1) == 0) || ((*pl1) == l) ) &&
	  ( ((*pc1) == 0) || ((*pc1) == c) ) )
	{
	  alternate_moves[count][0] = l;
	  alternate_moves[count][1] = c;
	  count++;
	}
  }
  alternate_moves[count][0] = alternate_moves[count][1] = 0;
  if (count > 0) {
    /* we return the first entry because the last entry, in the case of pawn,
       movement, might be a catching move, even it a non-catching, more 
       probable, is present
       */
    *pl1 = alternate_moves[0][0];
    *pc1 = alternate_moves[0][1];
  }
  return(count);
}

/* check for ambiguity in a move
   used in output function: the piece had been already moved and
   if we guess another move, there is an ambiguity
   */
#ifdef __STDC__
int ambiguity(depl *d, int *amline, int *amcols)
#else
int ambiguity(d, amline, amcols )
     depl *d ;
     int * amline;
     int *amcols;
#endif
{
  int l1 = 0 ;
  int c1 = 0 ;
  int r = 0;
  int frompiece = d->piece;
  int l2 = d->tolig;
  int c2 = d->tocol ;
  int i;

  undo_move(tos,m);


  switch(frompiece) {
  case PAWN:
    if (m->type == PRISE) {
      if (m->whiteturn)
	r = guess_depl(NB_M_PAWN_WX, m_pawn_wx, &l1,&c1, l2,c2, FALSE);
      else
	r = guess_depl(NB_M_PAWN_BX, m_pawn_bx, &l1,&c1, l2,c2, FALSE);
    } else {
      if (m->whiteturn)
	r = guess_depl(NB_M_PAWN_MOVE_WD,m_pawn_move_wd,&l1,&c1, l2,c2, FALSE);
      else
	r = guess_depl(NB_M_PAWN_MOVE_BD,m_pawn_move_bd,&l1,&c1,l2,c2, FALSE);
    }
    break;
  case KNIGHT:
    r = guess_depl(NB_M_KNIGHT, m_knight, &l1,&c1, l2,c2, FALSE);
    break;
  case BISHOP:
    r = guess_depl(NB_M_BISHOP, m_bishop, &l1,&c1, l2,c2, TRUE);
    break;
  case ROOK:
    r = guess_depl(NB_M_ROOK,   m_rook,   &l1,&c1, l2,c2, TRUE);
    break;
  case QUEEN:
    r = guess_depl(NB_M_QUEEN,  m_queen,  &l1,&c1, l2,c2, TRUE);
    break;
  case KING:
    r = guess_depl(NB_M_KING,   m_king,   &l1,&c1, l2,c2, TRUE);
    break;
  default:
    break;
  }
  do_move(tos,m);

  if (r > 1) {
    /* we have an ambiguity, we use alternate_moves to resolve it:
       we look through that table to find identical lines: if so, 
       we signal that the column determines move; 
       we then do the same with columns
       */
    *amline = TRUE ;
    *amcols = TRUE ;
    for (i= 1; i < r ; i++) {
      if (alternate_moves[i][0] != alternate_moves[0][0])
	*amline = FALSE;
      if (alternate_moves[i][1] != alternate_moves[0][1])
	*amcols = FALSE;
    }
  }
  return( (r > 1) );
}

#ifdef __STDC__
int check_move(depl *m)
#else
int check_move(m)
     depl * m;
#endif
{
  int l1,c1,l2,c2;
  int tmp; /* tmp boolean */
 
  l1 = m->fromlig;
  c1 = m->fromcol;
  l2 = m->tolig;
  c2 = m->tocol;

  if ((m->type == GRANDROQUE) || (m->type == PETITROQUE))
    return(check_roque());

  if ((tos->board[l1][c1] != m->piece)||
      (tos->color[l1][c1] != CURCOLOR(m))){
    fprintf(stderr,"Problem: piece should be %c\n",in_table[tos->board[l1][c1]]);
    if (m->whiteturn)
      error ((stderr,"\nOriginating position and piece not coherent for White move %d\n",m->move));
    else
      error ((stderr,"\nOriginating position and piece not coherent for Black move %d\n",m->move));
    return(FALSE);
  }

  /* if prise === FALSE, we must not take a piece */
  if (tos->board[l2][c2] != VOID 
      && (m->type != PRISE) && (m->type != PROM_ET_PRISE)) {
    (void) fprintf(stderr,"catching not indicated at move %d.\n",m->move);
    return(FALSE);
  }

  /* prendre une de ses propres pieces */
  if (tos->color[l2][c2] == tos->color[l1][c1] && m->prise) {
    (void) fprintf(stderr,"attempt to catch same color piece at move %d.\n",
		   m->move);
    return(FALSE);
  }

  /* we check if the move is a possible one for the piece
     */

  switch(m->piece) {
  case PAWN:
    if (m->prise) {
      if (m->whiteturn)
	tmp = guess_depl(NB_M_PAWN_WX, m_pawn_wx, &l1,&c1, l2,c2, FALSE);
      else
	tmp = guess_depl(NB_M_PAWN_BX, m_pawn_bx, &l1,&c1, l2,c2, FALSE);
    } else {
      if (m->whiteturn)
	tmp = guess_depl(NB_M_PAWN_WD, m_pawn_wd, &l1,&c1, l2,c2, FALSE);
      else
	tmp = guess_depl(NB_M_PAWN_BD, m_pawn_bd, &l1,&c1, l2,c2, FALSE);
    }
    /* is it a "prise en passant " */
    if ((c1 != c2) && (tos->board[l2][c2] == VOID)
	&& (tos->board[l1][c2] == PAWN)) {
      m->type = EN_PASSANT ;
      /* we must perform here the "en passant" test */
      tos->board[l1][c2] = VOID ;
      tos->color[l1][c2] = VOID ;
      tmp = TRUE;
    }
    return(tmp);
    break;
  case KNIGHT:
    return(guess_depl(NB_M_KNIGHT, m_knight, &l1,&c1, l2,c2, FALSE));
    break;
  case BISHOP:
    return(guess_depl(NB_M_BISHOP, m_bishop, &l1,&c1, l2,c2, TRUE));
    break;
  case ROOK:
    return(guess_depl(NB_M_ROOK,   m_rook,   &l1,&c1, l2,c2, TRUE));
    break;
  case QUEEN:
    return(guess_depl(NB_M_QUEEN,  m_queen,  &l1,&c1, l2,c2, TRUE));
    break;
  case KING:
    return(guess_depl(NB_M_KING,   m_king,   &l1,&c1, l2,c2, TRUE));
    break;
  default:
    break;
  }

  return(TRUE);
}

/* try to guess the move -- used for shortened notation
   */
#ifdef __STDC__
int guess_move(void)
#else
int guess_move()
#endif
{
  int l1,c1,l2,c2;

  if ((m->type == GRANDROQUE) || (m->type == PETITROQUE))
    return(TRUE);

  l1 = m->fromlig ;
  c1 = m->fromcol ;
  l2 = m->tolig;
  c2 = m->tocol;

  switch(m->piece) {
  case PAWN:
    if (m->prise) {
      if (m->whiteturn)
	(void) guess_depl(NB_M_PAWN_WX, m_pawn_wx, &l1,&c1, l2,c2, FALSE);
      else
	(void) guess_depl(NB_M_PAWN_BX, m_pawn_bx, &l1,&c1, l2,c2, FALSE);
    } else {
      if (m->whiteturn)
	(void) guess_depl(NB_M_PAWN_WD, m_pawn_wd, &l1,&c1, l2,c2, FALSE); 
      else
	(void) guess_depl(NB_M_PAWN_BD, m_pawn_bd, &l1,&c1, l2,c2, FALSE); 
    }
    break;
  case KNIGHT:
    (void) guess_depl(NB_M_KNIGHT, m_knight, &l1,&c1, l2,c2, FALSE);
    break;
  case BISHOP:
    (void) guess_depl(NB_M_BISHOP, m_bishop, &l1,&c1, l2,c2, TRUE);
    break;
  case ROOK:
    (void) guess_depl(NB_M_ROOK, m_rook, &l1,&c1, l2,c2, TRUE);
    break;
  case QUEEN:
    (void) guess_depl(NB_M_QUEEN, m_queen, &l1,&c1, l2,c2, TRUE);
    break;
  case KING:
    (void) guess_depl(NB_M_KING, m_king, &l1,&c1, l2,c2, TRUE);
    break;
  default:
    break;
  }

  if ((l1 == 0) || (c1 == 0)) {
    if (m->whiteturn)
      error((stderr,"\nUnable to guess white move %d, with piece %c\n",
	     m->move,in_table[m->piece]));
    else
      error((stderr,"\nUnable to guess black move %d, with piece %c\n",
	     m->move,in_table[m->piece]));
    return(FALSE);
  } else {
    m->fromcol = c1;
    m->fromlig = l1;
    return(TRUE);
  }
}

/* --------------- execution of move ----------------- */

/* clear a position */
#ifdef __STDC__
int clear_pos(int lig, int col)
#else
int clear_pos(lig,col)
     int lig;
     int col;
#endif
{
  tos->board[lig][col] = VOID ;
  tos->color[lig][col] = VOID ;
  return(TRUE);
}

/* configure the board */
#ifdef __STDC__
int configure(void)
#else
int configure()
#endif
{
  if (configuring) {
    if (m->piece == VOID)
      m->piece = PAWN ;
    tos->board[m->tolig][m->tocol] = m->piece ;
    tos->color[m->tolig][m->tocol] = configside ;
  }
  return(TRUE);
}

/* execute a move, no checking */
#ifdef __STDC__
int execute_move(void)
#else
int execute_move()
#endif
{
  register int i;

  if (m->piece == VOID )
    m->piece = PAWN;

  if ((m->fromlig == 0) || (m->fromcol == 0))
    (void) guess_move();
  
  /* supply to the -- maybe -- deficiency of input notation
     */
  if ((m->fromlig !=0) || (m->fromcol != 0))
    m->piece = tos->board[m->fromlig][m->fromcol];

  if (tos->board[m->tolig][m->tocol] != VOID) {
    m->type = PRISE;
    m->prise = tos->board[m->tolig][m->tocol] ;
  }

  if (!check_move(m)) {
    if (m->whiteturn)
      error((stderr,"\nWhite move %d illegal\n",m->move));
    else
      error((stderr,"\nBlack move %d illegal\n",m->move));
  }

  do_move(tos, m);

  output_move(dr,m);

  if (error_flag) {
    (void) fprintf(dr->outfile, "\nLast position encountered:\n");
    output_board(dr,tos);
    close_files();
    exit(0);
  }

  /* do we need to display the move ? */
  if (nb_move_to_dsp > 0) {
    for (i=0; i < nb_move_to_dsp; i++)
      if (m->move == (move_to_display[i] ) && !m->whiteturn ) {
	output_board(dr,tos);
	if (stop_at_display) {
	  output_end(dr);
	  close_files();
	  exit(0);
	}
      }
  }

  return(TRUE);
}

/* ------------------ automata ----------------------- */

/* categorise the input for the automata */
#ifdef __STDC__
int typechar(char c)
#else
int typechar(c)
     char c;
#endif
{
  if (ispiece(c))
    return(0);
  if ((c >=  'a') && ( c <= 'h'))
    return(1);
  if ((c >=  '1') && ( c <= '8'))
    return(2);
  if ( c== '-' )
    return(3);
  if ((c == 'x') || (c == 'X' ))
    return(4);
  if (c == '=' )
    return(5);
  if (c == '\0' )
    return(6);
  return(7);
}


/* execute the actions decided by the automata */
#ifdef __STDC__
int execute(int num,char c)
#else
int execute(num,c)
     int num;
     char c;
#endif
{
  switch (num) {
  case 1: /* set cur piece */
    curpiece = piece(c);
    break;
  case 2: /* set cur col */
    curcol = lettertocol(c);
    break;
  case 3: /* set cur lig */
    curlig = lettertolig(c);
    break;
  case 4: /* from = cur ; prise = false */
    m->piece = curpiece ;
    m->fromcol = curcol ;
    m->fromlig = curlig;
    /*m->topiece = curpiece;*/
    break;
  case 5: /* from = cur ; prise = true */
    m->piece = curpiece ;
    m->fromcol = curcol ;
    m->fromlig = curlig;
    m->type = PRISE ;
    m->prise = curpiece;
    break;
  case 6: /* to = cur ; guess from */
  case 7: /* to = cur ; guess from ; parse remaining token */
    m->piece = curpiece ;
    m->tocol = curcol;
    m->tolig = curlig ;

    /*m->topiece = curpiece ; /* ? */

    if (configuring)
      (void) configure();
    else {
      (void) execute_move();
    }
    break;
  case 8: /* to = cur */
  case 9: /* to = cur */
    m->tocol = curcol;
    m->tolig = curlig ;

    if (configuring)
      (void) configure();
    else {
      (void) execute_move();
    }
    break;
  case 10: /* piece = cur piece ; prise = true */
    /* later : guess from position */
    m->piece = curpiece ;
    m->type = PRISE ;
    break;
  case 11: /* grand roque */
  case 12: /* petit roque */
    (void) execute_move();
    break;
  case 13: /* case of simpliest algebraic notation ;
	      only e2e4 : this is the transition from e2 to e4
	      also the case of move such as Nge2
	      from =cur; prise = FALSE;
	      also:
	      curcol = ...
	      */
    m->piece = curpiece ;
    m->fromcol = curcol ;
    m->fromlig = curlig;

    m->type = MOVE;
    curcol = lettertocol(c);
    break;
  case 14: /* promotion, the "=" */
    /* NB: actions need some clean up here */
    /* to = cur */

    m->tocol = curcol;
    m->tolig = curlig ;
    /*m->topiece = curpiece ;*/

    if (m->type == PRISE )
      m->type = PROM_ET_PRISE ;
    else
      m->type = PROMOTION ;
    /* by default, we promote to queen 
       this can be overwritten by explicit naming
       */
    m->promotion = curpiece = QUEEN ;
    break;
  case 15: /* promotion, the piece name */
    m->promotion = curpiece = piece(c) ;
    break;
  case 16: /* not used */ 
    break;
  case 17: /* execute move for promotion */
    (void) execute_move();
    break;
  case -1:
    break;
  default:
    break;
  }
  return(TRUE);
}

#ifdef __STDC__
int parse_number(char *token)
#else
int parse_number(token)
     char *token;
#endif
{
  int curmove = 0 ;
  int i;

  /* check coherency with internal numbering */
  i = 0;
  while (isdigit(token[i])) {
   curmove = curmove * 10 +  ((int) token[i++] - (int) '0' );
  }
  movecount = curmove ;
  return(TRUE);
}

#ifdef __STDC__
int parse_text(char *text)
#else
int parse_text(text)
     char *text;
#endif
{
  output_text(dr,T_TEXT, text, 0);
  return(TRUE);
}

#ifdef __STDC__
int parse_comment(char *com)
#else
int parse_comment(com)
     char *com;
#endif
{
  int t;

  if (com[0] == '$')
    /* we look in the long ascii table */
    t = find_keyword(com_long, NUM_COM_CODE, NUM_COM_CODE, com, TRUE);
  else {
    /* we look for the comment in the short ascii table */
    t = find_keyword(com_short, NUM_COM_CODE, NUM_COM_CODE, com,FALSE);
    if (t == NUM_COM_CODE)
      fprintf (stderr,"\nWhat is \"%s\" ?\n",com);   
  }
  if (t != NUM_COM_CODE)
    output_text(dr,T_COMMENT, com, t);
  return(TRUE);
}

#ifdef __STDC__
int parse_keyword(char *token, char *text)
#else
int parse_keyword(token,text)
     char *token;
     char *text;
#endif
{
  char c;

  switch (find_keyword(keywords, NBKEYWORD, KNULL, token, TRUE)) {
  case START:
    /* don't forget we are configuring the previous move */
    /* -> move 0, black */
    configuring = FALSE;
    m->move = 0;
    m->whiteturn = FALSE;
    break;
  case CLEAR:
    clear_board(tos);
    m= theplay->chain;
    free_move_list(m);
    break;
  case SHOWBOARD:
    output_board(dr,tos);
    break;
  case TOWHITE:
    /* don't forget we are configuring the previous move */
    /* reset to 0,black --> 1,white */
    m->move = 0;
    m->whiteturn = FALSE;
    break;
  case TOBLACK:
    /* reset to 1,white -> 1 black */
    m->move = 1;
    m->whiteturn = TRUE;
    break;
  case CONFIGWH:
    configuring = TRUE ;
    configside = WHITE;
    m= theplay->chain;
    free_move_list(m);
    break;
  case CONFIGBL:
    configuring = TRUE ;
    configside = BLACK;
    m= theplay->chain;
    free_move_list(m);
    break;
  case DEFAULTP:
    init_board(tos);
    m= theplay->chain;
    free_move_list(m);
    break;
  case TITLE:
    output_text(dr, T_TITLE, text, NULL);
    break;
  case SUBTITLE:
    output_text(dr, T_SUBTITLE, text, NULL);
    break;
  case SCORE:
    output_text(dr, T_SCORE, text, NULL);
    break;
  case LANGUE:
    in_language = find_keyword (t_language, NBLANGUAGES, in_language,
			       text,TRUE);
    associe_traduction( &in_table, in_language);	       
    break;
  case SPECIAL: /* all input, up to \n is copied to output */
    while ((( c = getc(infile)) != EOF) && (c != '\n'))
      (void) putc (c,dr->outfile);
    putc ('\n', dr->outfile);
    break;
  case KNULL:
  default:
    fprintf(stderr,"unknown keyword %s\n",token);
    break;
  }
  return(TRUE);
}

#ifdef __STDC__
int parse_roque(char *token)
#else
int parse_roque(token)
     char * token;
#endif
{ 
  int i;

  for (i=0; i < NBROQUE && (strcmp(c_roque[i],token)!=0); i++) ;
  if ( i < NBROQUE ) {
    
    m = add_trailing_move(m);
    init_parse(m);

    if (strlen(token) == 3) {
      m->type = PETITROQUE ;
      (void) execute(12,DUMMYCHAR);
    } else {
      m->type = GRANDROQUE ;
      (void) execute(11,DUMMYCHAR);
    }
    /*(void) fprintf(stderr,"ROQUE\n");*/
    return(TRUE);
  }

  return(FALSE);
}

#ifdef __STDC__
int  parse_move(char *token)
#else
int  parse_move(token)
     char *token;
#endif
{
  register int i;
  int correcte = FALSE ;
  int erreursyntaxe = FALSE ;
  int etat =0;
  int code;
  
  m = add_trailing_move(m);
  init_parse(m);
  m->type = MOVE;

  i=0;
  while ( !correcte && !erreursyntaxe ) {
    code = typechar(token[i]);
    (void) execute(action[etat][code],token[i]);
    etat = transit[etat][code] ;
    if (etat == -1) 
      erreursyntaxe = TRUE;
    if (etat == FINAL)
      correcte = TRUE ;
    i++;
  }
  if (erreursyntaxe) {
    (void) fprintf(stderr, "no comprende, senor: %s\n",token);
    return(FALSE);
  }
  if (correcte) {
    /*(void) fprintf(stderr, "ia panimaiou, davai\n");*/
  }
  /*init_parse(m);*/
  return(TRUE);
}

#ifdef __STDC__
void init_parse(depl *m)
#else
void init_parse(m)
     depl * m ;
#endif
{

  /* global position and piece variable initialised to 0
     */
  /* move and whiteturn unchanged */ 

  m->type = MOVE ;

  curpiece = m->piece = VOID ;
  curcol = m->tocol = m->fromcol = 0;
  curlig = m->tolig = m->fromlig = 0;

  m->promotion = VOID;
  m->prise = VOID;
  m->is_check = FALSE ;

  curdigit = curmove = 0;

  /*if (movecount != m->move)
    (void) fprintf(stderr,"problem in move numbering: %d vs %d\n",
		   m->move, movecount);*/

}

/* ------------------- top routines -------------------- */

/* cette fonction analyse les arguments de la ligne de commande
   */
#ifdef __STDC__
int parse_options(int argc,char *argv[])
#else
int parse_options(argc,argv)
     int argc;
     char * argv[];
#endif
{
  int narg =1 ;
  int i;
  register int c;
  char cp[132];
  char chaine[MAXTOKLEN];

  infile = stdin;
  dr->outfile = stdout;
  nb_move_to_dsp = 0;

  while (narg < argc ) {
    (void) strcpy (cp,argv[narg]);
    switch (cp[0]) {
    case '-' :
      switch (cp[1]) {
      case 'f' : /* from langage */
	if  ((narg+1) >= argc )
	  fatal((stderr,"missing argument to %s option",cp));
	narg++ ;
	in_language = find_keyword (t_language, NBLANGUAGES,
				    DEFAULT_INPUT_LANGUAGE,
				    argv[narg],TRUE);
	break;
      case 't' : /* to langage */
	if  ((narg+1) >= argc )
	  fatal((stderr,"missing argument to %s option",cp));
	narg++ ;
	out_language = find_keyword (t_language, NBLANGUAGES,
				     DEFAULT_OUTPUT_LANGUAGE,
				     argv[narg],TRUE);
	break;
      case 'o' : /* next arg is output file */
	narg++ ;
	if ((dr->outfile = fopen (argv[narg],"w+")) == NULL) {
	  (void) fprintf (stderr,"can't open %s output file\n",argv[narg]);
	  (void) fprintf (stderr,"assume stdout for output\n");
	}
      case 'e':
	if  ((narg+1) >= argc )
	  fatal((stderr,"missing argument to %s option",cp));
	narg++ ;

	i=0;
	nb_move_to_dsp = 0;
	move_to_display[nb_move_to_dsp] = 0;
	while (isdigit(argv[narg][i])) {
	  move_to_display[nb_move_to_dsp] =
	    ((int) argv[narg][i] - (int) '0')
	      + move_to_display[nb_move_to_dsp] * 10;
	  i++;
	}
	nb_move_to_dsp++;
	stop_at_display = TRUE;
	break;
      case 'c':
	if  ((narg+1) >= argc )
	  fatal((stderr,"missing argument to %s option",cp));
	narg++ ;

	i=0;
	while (isdigit(argv[narg][i])) {
	  move_to_display[nb_move_to_dsp] = 0;
	  while (isdigit(argv[narg][i])) {
	    move_to_display[nb_move_to_dsp] =
	      ((int) argv[narg][i] - (int) '0')
	      + move_to_display[nb_move_to_dsp] * 10;
	    i++;
	  }
	  nb_move_to_dsp++;

	  if (nb_move_to_dsp > NB_MOVE_TO_DISP)
	    fatal((stderr,"max. number of move to display exceeded"));

	  /* process next number */
	  if (argv[narg][i] == ',')
	    i++;
	}
	break;
      case 'a': /* algebraic output */
	dr->output_move_format = ALGEBRAIC;
	break;
      case 's':  /* shortened output */
	dr->output_move_format = SHORTENED;
	break;
      case 'b': /* display only the board, no move */
	dr->only_board = TRUE;
	break;
      case 'd': /* output driver */
	if  ((narg+1) >= argc )
	  fatal((stderr,"missing argument to %s option",cp));
	narg++ ;
	driver = find_keyword(t_output, NB_DRIVER, DEFAULT_DRIVER,
			      argv[narg],TRUE);
	break;
      case 'i': /* no headers */
	dr->print_headers = FALSE;
	break;
      case 'v': /* print version */
	/* this already done, so exit() */
	exit(0);
	break;
      case 'h': /* help file */
	(void) strcpy(chaine,LIB_DIR);
        if ((fhelp = fopen(strcat(chaine,HELP_FILE),"r")) == NULL)
          fatal((stderr,"Can't find help file.\n"));
        else {
          while ((c = getc(fhelp)) != EOF)
            (void) fputc(c,stderr);
          (void) fclose(fhelp);
	  exit(0);
        }
         break;
      default:
	error((stderr,"\nUnknown command line options %s\n",cp));
	break;
      }
      break;
    default: /* assume this is the input file */
      if ((infile = fopen (cp,"r")) == NULL)
	fatal((stderr,"can't open %s input file\n",cp));
    }
    narg++;
  } /* process next arg */
  return(argc);
}

#ifdef __STDC__
void close_files(void)
#else
void close_files()
#endif
{
  if (!((infile == stdin)||(infile == NULL)))
    (void) fclose(infile);
  if (dr->outfile != stdout )
    (void) fclose(dr->outfile);
}

#ifdef __STDC__
int associe_traduction (char **table, int language)
#else
int associe_traduction (table, language)
char ** table;
int language ;
#endif
{
  if (language < 0 || (language >= NBLANGUAGES))
    error((stderr,"\nUnknown language\n"));
  else
    *table = c_language[language];
  return(language);
}

#ifdef __STDC__
static void print_all_play(play *p)
#else
static void print_all_play(p)
play *p;
#endif
{ 
  depl *d;
  d = p->chain;
  while (d->next != NULL){
    d = d->next;
    output_move(dr,d);
  }
}

/* ------------- main --------------------- */

#ifdef __STDC__
int notation_main(int argc,char *argv[])
#else
int notation_main(argc,argv)
     int argc;
     char * argv[];
#endif
{
  (void) fprintf(stderr,"%s\n",version_string);
  
  /* allocation of driver descriptor */
  dr = new_driver();

  /* default configuration */
  init_driver(dr,DEFAULT_DRIVER);
  (void) associe_traduction(&in_table,  DEFAULT_INPUT_LANGUAGE );
  (void) associe_traduction(&(dr->out_table), DEFAULT_OUTPUT_LANGUAGE);

  (void) parse_options(argc,argv);

  (void) associe_traduction (&in_table, in_language);
  (void) associe_traduction (&(dr->out_table), out_language);

  /* assoc driver */
  init_driver(dr,driver);

  configuring = FALSE;
  configside = VOID;

  /* initialise output file */
  output_init(dr);

  if (error_flag)
    fatal((stderr,"\nToo many errors"));

  /* allocation of board descriptor */
  tos = new_board();
  init_board(tos);

  /* allocation of move descriptor */
  m->type = VOID ;
  /*init_move(m);*/
  
  /* allocation of the play descriptor */
  theplay = (play *) malloc (sizeof(play)) ;
  theplay->initial = tos ;
  theplay->chain   = m ;
  movecount = 1;

  /* main analysis routine */
  yyin = infile ;
  yyout = stderr ;

  /*init_parse(m); */
  yylex();

  if ((count == 0) && !error_flag)
    output_board(dr,tos);

  if (error_flag) {
    error((stderr,"\nLast valid position:\n"));
    output_board(dr,tos);
    fatal((stderr,"\nToo many errors"));
  }
      
  /* terminates output files */
  output_end(dr);

  /* close files */
  close_files();

  /* exit properly */
  return 0;
}
