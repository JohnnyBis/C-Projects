/*
  Notation program
  @(#)drivers.h	3.9 (C) Henry Thomas   Release 3     Dated 12/10/91
 */
/* headers for output drivers */
#ifndef _HEADERS_DRIVERS
#define HEADERS_DRIVERS


#define D_ASCII  0
#define D_POST   1
#define D_TEX    2
#define D_ROFF   3
#define D_XCHESS 4
#define D_GNU    5

#define NB_DRIVER 6

/* variation convention */
#define VARIATION_IN 0
#define VARIATION_OUT 1


#define PS_HEADER  "lib/Header.ps"
#define PS_FOOTER  "lib/Footer.ps"
#define TEX_HEADER "lib/Header.tex"


/* output buffers */
#define TAMPON 256

typedef struct {

  /* type of driver */
  int type ;

  /* output_file */
  FILE *outfile ;
  /* these  booleans control the output format */
  int print_move    ;  /* move numbering */
  int print_piece   ;  /* print piece name */
  int print_pawn    ; /* print the PAWN name */
  int roque_alg     ; /*  roque in algebraic form Ke1g1 or O-O */
  int print_liaison ; /* print the - or x in move output */
  int only_board ;
  int variation     ; /* variation level */
  int print_headers ; /* include the header/footer file */

  /* boolean to print the coordinates in ascii output of board */
  int coordinates   ;
  int output_move_format ;

  char *out_table;	/* translation table */

  /* procedures */
  void (*out_init)() ;
  void (*out_move)() ;
  void (*out_variation)() ;
  void (*out_text)() ;
  void (*out_board)() ;
  void (*out_end)() ;

  /* temp vars  used by move buffering */

  int iswhiteturn ; /*= FALSE */
  int interrupt ; /*= FALSE */

  char move_buffer[TAMPON]  /*= ""*/ ;
  char white_buffer[TAMPON] /*= ""*/ ;
  char black_buffer[TAMPON] /*= ""*/ ;


} format ;


/* fonctions ----------------- */

#ifdef __STDC__

extern void output_init(format *dr);
extern void output_move(format *dr, depl *d);
extern void output_variation(format *dr, int inout);
extern void output_text(format *dr, int type, char *string, int code);
extern void output_board(format *dr, game *g);
extern void output_end(format *dr);

extern format *new_driver(void);
extern void init_driver(format *dr, int driver);

#else

extern void output_init(/*format *dr*/);
extern void output_move(/*format *dr, depl *d*/);
extern void output_variation(/*format *dr, int inout*/);
extern void output_text(/*format *dr, int type, char *string, int code*/);
extern void output_board(/*format *dr, game *g*/);
extern void output_end(/*format *dr*/);

extern format *new_driver(/*void*/);
extern void init_driver(/*format *dr, int driver*/);
#endif

#endif
