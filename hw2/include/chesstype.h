/*
  Notation program
  @(#)chesstype.h	3.9 (C) Henry Thomas   Release 3     Dated 12/10/91
 */

/* types definition */
#ifndef _HEADERS_TYPES
#define _HEADERS_TYPES

#define FALSE 0
#define TRUE  1

#ifndef NULL
#define NULL    ((char *) 0)
#endif

#define MAX(a,b)  ((a)>(b)?(a):(b))
#define MIN(a,b)  ((a)<(b)?(a):(b))
#define ABS(a)	  ((a)>=0?(a):(-(a)))
#define SIGN(a)   ((a)>=0?(1):-1)

#define DUMMYCHAR '$'
#define MAXTOKLEN 1024


/* max number of move for displaying board */
#define NB_MOVE_TO_DISP 128

/* debugging help */
#ifdef DEBUG
#define MESSAGE(A) (void) fprintf A
#define ACTION(A)  A
#else
#define MESSAGE(A)
#define ACTION(A)
#endif

/* output move format */

#define ALGEBRAIC 0
#define SHORTENED 1

/* text type */
#define T_TEXT		1
#define T_COMMENT	2
#define T_TITLE		3
#define T_SUBTITLE	4
#define T_SCORE		5

/* -------------------------------------------- */
/*              chess data structures           */
/* -------------------------------------------- */

#define NUMPIECES 7

#define KING   1
#define QUEEN  2
#define ROOK   3
#define BISHOP 4
#define KNIGHT 5
#define PAWN   6

#define VOID  0
#define WHITE 1
#define BLACK -1

/* board and move representation */
/* board size */
#define SIZE 10 

/* ---- structure to represent the game ---- */
typedef struct {

  /* board definition */
  int board[SIZE][SIZE];
  int color[SIZE][SIZE];

} game ;
/* ---- end of structure ---- */

#define GULL (game *) 0

/* french(roque) == english(castling) */

#define MOVE		1  	/* un mouvement        */
#define PRISE		2      	/* une prise           */
#define GRANDROQUE	3 	/* grand roque         */
#define PETITROQUE	4 	/* petit roque         */
#define EN_PASSANT	5	/* prise en passant    */
#define PROMOTION  	6	/* promotion           */
#define PROM_ET_PRISE	7	/* promotion + capture */

#define SHORT_COMMENT_LEN 4

/* structure used to describe the current move */
struct deplace {
  /* system zone */
  int uid ;             /* node id */

  /* links used to chain moves   */
  struct deplace * next ; /* next move     */
  struct deplace * prev ; /* previous move */
  struct deplace * sub  ; /* variation     */

  /* user zone */
  int move ;		/* number of the move */
  int whiteturn ; 	/* boolean to tell if white's turn */

  int type;		/* type of move: MOVE, PRISE, etc..   */
  int piece;		/* type of the piece */

  int fromcol, fromlig;	/* from position */
  int tocol, tolig ;	/* destination   */

  int prise;		/* captured piece */
  int promotion;        /* name of the pice the pawn is promoted to */

  int is_check ;	/* if does the move provides check ?*/

  char comment[SHORT_COMMENT_LEN];	/* short text comment */
  char * text ;         /* long text comment */


} ;
typedef struct deplace depl ;

#define MULL (depl *) 0

#define CURCOLOR(M) (((M)->whiteturn)?(WHITE):(BLACK))
#define OPPCOLOR(M) (((M)->whiteturn)?(BLACK):(WHITE))


/* structure used to hold a complete play 
   this structure is
   - an initial board 
   - a chain of moves
   moves are applied to the board, and coan also be undone
   */
typedef struct {
  game * initial;
  depl * chain ;
} play ;


/* end headers */
#endif
