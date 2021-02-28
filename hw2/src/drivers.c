/*
  Notation program
  @(#)drivers.c	3.9 (C) Henry Thomas	Release 3     Dated 12/10/91
 */

#ifdef __STDC__
#include <stdlib.h>
#endif
#include<stdio.h>

#include <ctype.h>
#include "chesstype.h"
#include "notation.h"
#include "drivers.h"

/* postscript characters translation table
   one entry per piece
   each entry has four fields:
   - white piece on white case
   - white piece on black case
   - black piece on ...
   ..
 */
#define PSINDEX(a,b) ((((a)==WHITE)?0:2)+(((b)%2)?0:1))

static char * postscript_table[][4] = {
  { " ", "x",     " ", "x"     }, /* void */
  { "k", "\\373", "K", "\\360" }, /* king */
  { "q", "\\317", "Q", "\\316" }, /* queen */
  { "r", "\\250", "R", "\\345" }, /* rook */
  { "b", "\\272", "B", "\\365" }, /* bishop */
  { "n", "\\265", "N", "\\366" }, /* knight */
  { "p", "\\271", "P", "\\270" }  /* pawn */
};

/* TeX table for using the chess figure in board design */
static char * texboard_table[][4] = {
  /*   W/W      W/B      B/W      B/B */
  { " ", "*", " ", "*" }, /* void */
  { "K", "K", "k", "k" }, /* king */
  { "Q", "Q", "q", "q" }, /* queen */
  { "R", "R", "r", "r" }, /* rook */
  { "B", "B", "b", "b" }, /* bishop */
  { "N", "N", "n", "n" }, /* knight */
  { "P", "P", "p", "p" }  /* pawn */
};

/* TeX table for using the chess figures in move description */
/* P.T. macros render this table useless as english symbols 
   are active char in analysis mode
   */
static char * latex_table[] = {
  "", /* null */
/*  "{\\Fig K}", "{\\Fig Q}", "{\\Fig R}", "{\\Fig B}", "{\\Fig N}",  "" */
  "K", "Q", "R", "B", "N",  "" 
    /* last entry = pawn; not represented, otherwise "{\\Fig P}" */
};

/* various tex symbols */
static char FigDash[] = "\\FigDash";
static char FigCapt[] = "*"; /* "*" is an active char with P.T. macros */
static char FigDots[] = ":"; /* ":" is an active char for "..." */
static char FigDot[] = "\\FigDot";

#define G_ROQUE  "O-O-O" 
#define P_ROQUE  "O-O" 

/* variation symbols */
static char varsymb[][2] = { { '[', ']' }, { '(', ')' } };

static char * com_tex[] = {
#define CHESSSYMB(LET,LASC,SASC,TEX,PS,ENG,FRA) TEX,
#include "chesssymb.def"
 ""
 };
#undef CHESSSYMB

static char * com_ps[] = {
#define CHESSSYMB(LET,LASC,SASC,TEX,PS,ENG,FRA) PS,
#include "chesssymb.def"
 ""
 };
#undef CHESSSYMB

static FILE * ftmp ;


/* ---------------- output functions ---------------- */

/* convert a roque in term of king's move */
#ifdef __STDC__
static int roque_to_move(depl *m)
#else
static int roque_to_move(m)
     depl * m;
#endif
{

  m->piece = KING;
  m->fromcol   = 5;
  if (m->type == GRANDROQUE)  
    m->tocol = 3;
  else
    m->tocol = 7;

  if (m->whiteturn)
    m->fromlig = m->tolig = 1;
  else
    m->fromlig = m->tolig = 8;

  return(TRUE);
}

/* (kind of) buffering of output */
#ifdef __STDC__
static void init_buffer(format *d, int side)
#else
static void init_buffer(d,side)
     format * d;
     int side ;
#endif
{
  switch (d->type) {
  case D_ASCII:
    if (side != BLACK) (void) sprintf(d->white_buffer,"...");
    if (side != WHITE) (void) sprintf(d->black_buffer,"   ");
    break;
  case D_TEX:
    if (side != BLACK) (void) sprintf(d->white_buffer,"%s",FigDots);
    if (side != WHITE) (void) sprintf(d->black_buffer,"~");
    break;
  default:
    if (side != BLACK) d->white_buffer[0] = '\0' ;
    if (side != WHITE) d->black_buffer[0] = '\0' ;
    break;
  }
}

/* this procedure is responsible for PRINTING the move */
#ifdef __STDC__
static void flush_buffer(format *d)
#else
static void flush_buffer(d)
     format * d;
#endif
{
  
  /* if we have been interupted (by a comment, a board display etc...
     if the move is black
     we display <movenumber> ... <blackmove>
     */
  if ((d->interrupt == TRUE) && (d->iswhiteturn == FALSE)) {
    switch (d->type) {
    case D_TEX:
      (void) fprintf(d->outfile,
		     "%s %s %s\n", 
		     d->move_buffer,FigDots,d->black_buffer);
      break;
    case D_GNU:
    case D_XCHESS:
      /* no special case for GNU */
      (void) fprintf(d->outfile,"\t%s\n",d->black_buffer);
      break;    
    default:
      (void) fprintf(d->outfile,"\n%3s.%9s%9s", 
		     d->move_buffer,"...",d->black_buffer);
      break;
    }
    d->interrupt = FALSE ;
  } else {
  /* else (no interrupt)
     we display either white or black move 
     */
    switch (d->type) {
    case D_TEX:
      if (d->iswhiteturn)
	(void) fprintf(d->outfile,"%s %s", d->move_buffer,d->white_buffer);
      else
	(void) fprintf(d->outfile," %s\n",d->black_buffer);
      break;
    case D_XCHESS:
      if (d->iswhiteturn)
	(void) fprintf(d->outfile,"%3s.", d->move_buffer);
    case D_GNU:
      if (d->iswhiteturn)
	(void) fprintf(d->outfile,"\t%s",d-> white_buffer);
      else
	(void) fprintf(d->outfile,"\t%s\n",d->black_buffer);
      break;    
    default:
      if (d->iswhiteturn)
	(void) fprintf(d->outfile,"\n%3s.%9s", d->move_buffer,d->white_buffer);
      else
	(void) fprintf(d->outfile,"%9s", d->black_buffer);
      break;
    }
  } /* end printing */

  /* reset buffer */
  if (! d->iswhiteturn)
    init_buffer(d,VOID);
  d->interrupt = FALSE;
}

/* a generic parametrised driver for move output
   */
#ifdef __STDC__
static void output_move_generic(format *dr, depl *d)
#else
static void output_move_generic(dr,d)
     format * dr;
     depl *d;
#endif
{
  char ligne[128] ;
  char themove[128] ;
  char thepiece[16]  ;
  char debcol[16];
  char frommove[16]  ;
  char tomove[16] ;
  char captsymb[16] ;
  char lie[16] ;
  char prom[16];

  int ambigue = FALSE ;
  int ambigueline, ambiguecols;

  ligne[0] = themove[0] = thepiece[0] = '\0';
  frommove[0] = tomove[0] = lie[0] = prom[0] = '\0' ;

  if (dr->type == D_TEX) 
    (void) sprintf(captsymb,"%s", FigCapt);
  else
    (void) sprintf(captsymb,"%s", "x" );

  if (dr->type == D_TEX) {
    (void) sprintf (dr->move_buffer,"\\mn{%d}",d->move);
  } else 
    (void) sprintf (dr->move_buffer,"%d",d->move);

  if ((d->type == PETITROQUE) && !dr->roque_alg)
    (void) sprintf (themove,"%s",P_ROQUE);
  if ((d->type == GRANDROQUE) && !dr->roque_alg)
    (void) sprintf (themove,"%s",G_ROQUE);
  if (dr->roque_alg && 
      ((d->type == GRANDROQUE) || (d->type == PETITROQUE)))
    (void) roque_to_move(d);

  if (dr-> roque_alg || 
      ((d->type != GRANDROQUE) && (d->type != PETITROQUE))) {

    /* we check here for ambiguous move */
    if ((d->type != GRANDROQUE) && (d->type != PETITROQUE)) {
      ambigue = ambiguity (d, &ambigueline, &ambiguecols );
      /* if ( (ambigue ) && (d->piece != PAWN ))
       * (void) fprintf (stderr,"output ambiguity at move %d %d",
       * d->move,d->whiteturn); 
       */
    }

    themove[0] = '\0' ;
    if ((dr->output_move_format == SHORTENED) 
	&& (d->type == PRISE) && (d->piece == PAWN))
      (void) sprintf (debcol, "%c",coltoletter(d->fromcol));

    if (dr->print_piece)
      if (d->piece != PAWN || dr->print_pawn) {
        if (dr->type == D_TEX )
          (void) sprintf(thepiece,"%s",latex_table[d->piece]);
        else
          (void) sprintf(thepiece,"%c",dr->out_table[d->piece]);
      }

    if ((dr->output_move_format == ALGEBRAIC))
      (void)sprintf(frommove,"%c%c",
		    coltoletter(d->fromcol),ligtoletter(d->fromlig));
    if ( ambigue && dr->print_liaison ) {
      /* is the ambiguity on lines ? -> print col */
      if (ambigueline && !ambiguecols)
	(void)sprintf(frommove,"%c", coltoletter(d->fromcol));
      /* is the ambiguity on lines ? -> print lig */
      if (ambiguecols && !ambigueline)
	(void)sprintf(frommove,"%c", ligtoletter(d->fromlig));     
      /* unable to find where is ambiguity ? print all */
      /* ( I doubt this case ever occurs ... ) */
      if ( ambigueline && ambiguecols) 
	(void)sprintf(frommove,"%c%c",
		      coltoletter(d->fromcol),ligtoletter(d->fromlig));
      debcol[0] = '\0' ;
    }

    if (d->promotion) {
      if (dr->print_liaison) {
        if (dr->type == D_TEX )
          (void) sprintf(prom,"=%s ",latex_table[d->promotion]);
          else
            (void) sprintf(prom,"=%c",dr->out_table[d->promotion]);
      } else /* xchess - gnu output */
	(void) sprintf(prom,"%c",dr->out_table[d->promotion]);
    }
          
    if (dr->print_liaison) {
      if ((d->type == PRISE) || (d->type == PROM_ET_PRISE) 
	  || (d->type == EN_PASSANT) )
	(void) sprintf(lie,"%s",captsymb);
      else
	if ((dr->output_move_format == ALGEBRAIC))
	  (void) sprintf(lie,"%c",'-');
    }
    
    (void) sprintf(tomove,"%c%c",coltoletter(d->tocol),ligtoletter(d->tolig));

    (void) sprintf (themove,"%s%s%s%s%s%s",
		    thepiece,debcol,frommove,lie, tomove,prom);
  }

  if (d->whiteturn)
    (void) sprintf (dr->white_buffer, "%s",themove);
  else
    (void) sprintf (dr->black_buffer, "%s",themove);

  dr->iswhiteturn = d->whiteturn; 

  /*fprintf(dr->outfile, "=%d=%d= ",d->move,d->whiteturn);*/
  flush_buffer(dr); 
}

/* variation handler */
#ifdef __STDC__
static void output_variation_generic (format *dr, int inout)
#else
static void output_variation_generic (dr,inout)
     format * dr;
     int inout;
#endif
{
  char symbol;

  if (dr->variation > 1)
    symbol = varsymb[1][inout];
  else
    symbol = varsymb[0][inout];
    
  switch (dr->type) {
  case D_TEX:
    /* we must boldface the brackets for level 1 */
    if (dr->variation == 1 ) {
      /*(void) fprintf(dr->outfile, " {\\bf %c} ",symbol);*/
      if (inout == 0 )
        (void) fprintf(dr->outfile, "%%\n\\begin{Variation}%%\n ");
      else
        (void) fprintf(dr->outfile, "\\end{Variation} %%\n");
    } else
      (void) fprintf(dr->outfile, " %c ",symbol); 
    break;
  default:
    (void) fprintf(dr->outfile, "  %c",symbol);
    break;
  };
}

#ifdef __STDC__
static void output_text_generic(format *dr, int type, char *string, int code)
#else
static void output_text_generic(dr, type, string, code)
     format *dr ;
     int type;
     char * string;
     int code;
#endif
{
  switch (type) {
  case T_COMMENT:
    if (com_short[code] != '\0' )
      (void) fprintf(dr->outfile," %s ",com_short[code]);
    else
      (void) fprintf(dr->outfile," %s ",com_long[code]);
      break;
  case T_TEXT:
    (void) fprintf(dr->outfile," %s ",string);
    break;
  case T_TITLE:
    (void) fprintf(dr->outfile,"\n  %s\n",string);
    break;
  case T_SUBTITLE:
    (void) fprintf(dr->outfile,"    %s\n",string);
    break;
  case T_SCORE:
    (void) fprintf(dr->outfile,"  %s\n",string);
  default:
    break;
  }
}


/* ---------------- ascii driver ----------  */
#ifdef __STDC__
static void output_init_ascii(format *dr) 
#else
static void output_init_ascii(dr) 
format *dr;
#endif
{}
    
#ifdef __STDC__
static void output_board_ascii(format *dr,game *g)
#else
static void output_board_ascii(dr,g)
     format * dr;
     game * g;
#endif
{
  register int i,j;

  dr->interrupt = TRUE;

  (void) fprintf(dr->outfile,"\n\n");
  for (i=8 ; i >=1 ; i--) {
    if (dr->coordinates)
      (void) fprintf(dr->outfile,"%d ",i);
    (void) fputc('|',dr->outfile);
    for (j=1 ; j<9 ; j++) {
      if (g->board[i][j] != VOID) {
	if (g->color[i][j] == WHITE) 
	 (void) fputc(dr->out_table[g->board[i][j]], dr->outfile);
	else 
	  (void) fputc(tolower(dr->out_table[g->board[i][j]]),dr->outfile);
      } else
	(void) fputc ( ((i+j)% 2)?' ':'/', dr->outfile);
      (void) fputc('|', dr->outfile);
    }
    (void) fputc('\n', dr->outfile);
  }
  if (dr->coordinates)
    (void) fprintf(dr->outfile,"   a b c d e f g h\n");
  (void) fprintf(dr->outfile,"\n");
}

/* ---------------- postscript --------- */

#ifdef __STDC__
static void output_board_ps(format *dr,game *g)
#else
static void output_board_ps(dr,g)
     format *dr;
     game * g;
#endif
{
  register int i,j;
  register int c;
  char chaine[MAXTOKLEN];

  /* header file */
  (void) strcpy(chaine,LIB_DIR);
  if ((ftmp = fopen(strcat(chaine,PS_HEADER),"r")) == NULL)
    message((stderr,"Can't open ps header file.\n"));
  else {
    while ((c = getc(ftmp)) != EOF)
      (void) fputc(c,dr->outfile);
    (void) fclose(ftmp);
  }

  (void) fprintf(dr->outfile,"( ________) 72 714 T\n");
  for (i=8 ; i >=1 ; i--) {
    (void) fprintf(dr->outfile,"(/");
    for (j=1 ; j<9 ; j++) {
    (void) fprintf(dr->outfile,"%s",
		   postscript_table[g->board[i][j]][PSINDEX(g->color[i][j],(i+j))]);
    }
    (void) fprintf(dr->outfile,"\\\\) 72 %d T\n",474 + (i-1)*30);
  }
  (void) fprintf(dr->outfile,"( --------) 72 444 T\n");

  /* footer file */
  (void) strcpy(chaine,LIB_DIR);
  if ((ftmp = fopen(strcat(chaine,PS_FOOTER),"r")) == NULL)
    message((stderr,"Can't open ps footer file.\n"));
  else {
    while ((c = getc(ftmp)) != EOF)
      (void) fputc(c,dr->outfile);
    (void) fclose(ftmp);
  }
}

/* ---------------- nroff --------------- */
#ifdef __STDC__
static void output_init_roff(format *dr)
#else
static void output_init_roff(dr)
     format *dr;
#endif
{
}

#ifdef __STDC__
static void output_board_roff(format *dr,game *g)
#else
static void output_board_roff(dr, g)
     format *dr;
     game * g;
#endif
{
  register int i,j;

  dr->interrupt = TRUE;

  (void) fprintf(dr->outfile,".br\n");
  for (i=8 ; i >=1 ; i--) {
    (void) fprintf(dr->outfile,".ce\n  ");
    for (j=1 ; j<9 ; j++) {
      if (g->board[i][j] != VOID) {
	if (g->color[i][j] == WHITE) 
	 (void) fputc(dr->out_table[g->board[i][j]], dr->outfile);
	else 
	  (void) fputc(tolower(dr->out_table[g->board[i][j]]),dr->outfile);
      } else
	/*(void) fputc ( ((i+j)% 2)?' ':'/', dr->outfile);*/
	(void) fprintf(dr->outfile,".");
    }
    (void) fprintf(dr->outfile,"\n.br\n");
  }
  (void) fprintf(dr->outfile,"\n");
}

/* ---------------- tex -------------------- */
#ifdef __STDC__
static void output_init_tex(format *dr)
#else
static void output_init_tex(dr)
     format *dr;
#endif
{
  register int c;

  /* header text */
  fprintf(dr->outfile, "%% This file generated by the Notation program\n");
  fprintf(dr->outfile, "%% @ Henry Thomas 1991\n");
  fprintf(dr->outfile, "\\documentstyle[twocolumn,chess]{article}\n");
  fprintf(dr->outfile, "\\input{lib/notation.tex}\n");
  fprintf(dr->outfile, "\n");
  fprintf(dr->outfile, "\\begin{document}\n");
  fprintf(dr->outfile, "\n");
  fprintf(dr->outfile, "\\begin{Mainline}{}{}\n");
}

#ifdef __STDC__
static void output_text_tex(format *dr, int type, char * string, int code)
#else
static void output_text_tex(dr, type, string, code)
     format *dr ;
     int type;
     char * string;
     int code;
#endif
{
  if (type != T_COMMENT)
    (void) fprintf(dr->outfile, " \\nochess");

  switch (type) {
  case T_COMMENT:
    if (com_tex[code] != '\0' )
      (void) fprintf(dr->outfile,"%s\\ ",com_tex[code]);
    else
      (void) fprintf(dr->outfile,"%s\\ ",com_short[code]);
      break;
  case T_TEXT:
    (void) fprintf(dr->outfile," %s ",string);
    break;
  case T_TITLE:
    (void) fprintf(dr->outfile,"\n\\ChessTitle{%s}\n",string);
    break;
  case T_SUBTITLE:
    (void) fprintf(dr->outfile,"\n\\ChessSubTitle{%s}\n",string);
    break;
  case T_SCORE:
    (void) fprintf(dr->outfile,"\n\\ChessScore{%s}\n",string);
  default:
    break;
  };
  if (type != T_COMMENT)
    (void) fprintf(dr->outfile, "\\endnochess ");
}

#ifdef __STDC__
static void output_board_tex(format *dr,game *g)
#else
static void output_board_tex(dr,g)
     format *dr;
     game * g;
#endif
{
  register int i,j;

  dr->interrupt = TRUE;

  (void) fprintf(dr->outfile,"\n\n\\begin{diagram}\n");
  (void)fprintf(dr->outfile,"\\board");
  for (i=8 ; i >=1 ; i--) {
    (void) fprintf(dr->outfile,"\t{");
    for (j=1 ; j < 9 ; j++) {
    (void) fprintf(dr->outfile,"%s",
		   texboard_table[g->board[i][j]][PSINDEX(g->color[i][j],(i+j))]);
    }
    (void) fprintf(dr->outfile,"}\n");
  }
  (void) fprintf(dr->outfile,"\\end{diagram}\n\n");
}

#ifdef __STDC__
static void output_end_tex(format *dr)
#else
static void output_end_tex(dr)
     format *dr;
#endif
{ 
  (void) fprintf(dr->outfile, "\n\\end{Mainline}\n");
  (void) fprintf(dr->outfile, " \n\n\n\\end{document}\n");
}

/* ------------------ gnu - xchess ---------- */

#ifdef __STDC__
static void output_init_gnu(format *dr)
#else
static void output_init_gnu(dr)
     format *dr;
#endif
{
  (void) fprintf(dr->outfile, "X Chess -- Mon Dec 10 11:47:18 MET 1990\n");
  (void) fprintf(dr->outfile,"\tGame played on dummkopft.irisa.fr:0.0\n");
  (void) fprintf(dr->outfile,"\talgebraic\n");
}

/* ---------------- driver handler ---------- */
/* dummy driver */
#ifdef __STDC__
static void null_driver(void) {}
#else
static void null_driver() {}
#endif

/* the drivers */
#ifdef __STDC__
void output_init(format *dr)
#else
void output_init(dr)
     format *dr ;
#endif
{
  if (dr->print_headers)
    dr->out_init(dr);
}

#ifdef __STDC__
void output_move(format *dr,depl *d)
#else
void output_move(dr,d)
     format *dr ;
     depl *d;
#endif
{
  if (! (((dr->type == D_GNU) || (dr->type == D_XCHESS))
	&& (dr->variation > 0)))
    dr->out_move(dr,d);
}

#ifdef __STDC__
void output_variation(format *dr, int inout)
#else
void output_variation(dr, inout)
     format *dr ;
     int inout;
#endif
{
  dr->out_variation(dr,inout);
}

#ifdef __STDC__
void output_text(format *dr, int type, char *string, int code)
#else
void output_text(dr, type, string, code)
     format *dr ;
     int type;
     char * string;
     int code;
#endif
{
  if ((dr->type != D_GNU) && (dr->type != D_XCHESS))
    dr->out_text(dr, type, string, code);
}

#ifdef __STDC__
void output_board(format *dr, game *g)
#else
void output_board(dr,g)
     format *dr ;
     game *g ;
#endif
{
  dr->out_board(dr,g);
}

#ifdef __STDC__
void output_end(format *dr)
#else
void output_end(dr)
     format *dr ;
#endif
{
  if (dr->print_headers)
    dr->out_end(dr);
  (void) fprintf(dr->outfile,"\n");
}


#ifdef __STDC__
format * new_driver(void)
#else
format * new_driver()
#endif
{
  format * tmp;
  int i; 

  tmp = (format *) malloc (sizeof(format));
  ALLOCP(tmp);
  for (i=0; i < ((sizeof (format))/ sizeof (int)) ; i++)
    ((int *) tmp)[i] = 0;
  tmp->output_move_format = SHORTENED;
  tmp->print_headers = TRUE;
  return(tmp);
}

#ifdef __STDC__
void init_driver(format *dr,int driver)
#else
void init_driver(dr,driver)
     format * dr;
     int driver;
#endif
{
  dr->type = driver ;

  init_buffer(dr, VOID);
  switch (dr->type) {
  case D_ASCII:
    dr->print_move = TRUE;
    dr->print_piece = TRUE;
    dr->print_pawn = FALSE;
    dr->roque_alg = FALSE;
    dr->print_liaison = TRUE;
    dr->out_init = output_init_ascii;
    dr->out_move = output_move_generic;
    dr->out_variation = output_variation_generic;
    dr->out_text = output_text_generic;
    dr->out_board = output_board_ascii;
    dr->out_end = null_driver;
    break;
  case D_POST:
    dr->out_init = null_driver;
    dr->out_move = null_driver;
    dr->out_variation = null_driver;
    dr->out_text = null_driver;
    dr->out_board = output_board_ps;
    dr->out_end = null_driver;
    break;
  case D_TEX:
    dr->print_move = TRUE;
    dr->print_piece = TRUE;
    dr->print_pawn = FALSE;
    dr->roque_alg = FALSE;
    dr->print_liaison = TRUE;
    dr->out_init = output_init_tex;
    dr->out_move = output_move_generic;
    dr->out_variation = output_variation_generic;
    dr->out_text = output_text_tex;
    dr->out_board = output_board_tex;
    dr->out_end = output_end_tex;
    break;
  case D_ROFF:
    dr->print_move = TRUE;
    dr->print_piece = TRUE;
    dr->print_pawn = FALSE;
    dr->roque_alg = FALSE;
    dr->print_liaison = TRUE;
    dr->out_init = output_init_roff;
    dr->out_move = output_move_generic;
    dr->out_variation = output_variation_generic;
    dr->out_text = output_text_generic;
    dr->out_board = output_board_roff;
    dr->out_end = null_driver;
    break;
  case D_XCHESS:
    dr->output_move_format = ALGEBRAIC;
    dr->print_move = TRUE;
    dr->print_piece = FALSE;
    dr->print_pawn = FALSE;
    dr->roque_alg = TRUE;
    dr->print_liaison = FALSE;
    dr->out_init = output_init_gnu;
    dr->out_move = output_move_generic;
    dr->out_variation = null_driver;
    dr->out_text = null_driver;
    dr->out_board = null_driver;
    dr->out_end = null_driver;
    break;
  case D_GNU:
    dr->output_move_format = ALGEBRAIC;
    dr->print_move = FALSE;
    dr->print_piece = FALSE;
    dr->print_pawn = FALSE;
    dr->roque_alg = TRUE;
    dr->print_liaison = FALSE;
    dr->out_init = null_driver;
    dr->out_move = output_move_generic;
    dr->out_variation = null_driver;
    dr->out_text = null_driver;
    dr->out_board = null_driver;
    dr->out_end = null_driver;
    break;
  default:
    error((stderr,"unknown driver"));
    break;
  }
  if (dr->only_board)
    dr->out_move = null_driver ;

  dr->variation = 0;
  dr->iswhiteturn = FALSE ;
  dr->interrupt = FALSE;
}
