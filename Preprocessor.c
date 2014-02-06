/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>
#line 1 "Preprocessor.y"


	#include <cstdio>
	#include <cstdlib>
	#include <cassert>
	#include <vector>
	#include <map>
	#include "HLSLParser.h"
	#include "ParserUtils.h"
	#include "ParserState.h"
#line 19 "Preprocessor.c"
/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/* 
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands. 
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash 
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    ParseTOKENTYPE     is the data type used for minor tokens given 
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is ParseTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    ParseARG_SDECL     A static variable declaration for the %extra_argument
**    ParseARG_PDECL     A parameter declaration for the %extra_argument
**    ParseARG_STORE     Code to store %extra_argument into yypParser
**    ParseARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
#define YYCODETYPE unsigned char
#define YYNOCODE 48
#define YYACTIONTYPE unsigned char
#define ParseTOKENTYPE ScannerToken
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  signed long yy9;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL ParserState* parserState;
#define ParseARG_PDECL ,ParserState* parserState
#define ParseARG_FETCH ParserState* parserState = yypParser->parserState
#define ParseARG_STORE yypParser->parserState = parserState
#define YYNSTATE 75
#define YYNRULE 45
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* The yyzerominor constant is used to initialize instances of
** YYMINORTYPE objects to zero. */
static const YYMINORTYPE yyzerominor = { 0 };

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
#define YY_ACTTAB_COUNT (279)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   121,   56,   70,   55,   68,   67,   62,   31,   42,   41,
 /*    10 */    28,   34,   51,   50,   48,   46,   32,   57,   52,   70,
 /*    20 */    55,   68,   67,   62,   31,   42,   41,   28,   34,   51,
 /*    30 */    50,   48,   46,   32,   57,   43,   70,   55,   68,   67,
 /*    40 */    62,   31,   42,   41,   28,   34,   51,   50,   48,   46,
 /*    50 */    32,   57,   58,   70,   55,   68,   67,   62,   31,   42,
 /*    60 */    41,   28,   34,   51,   50,   48,   46,   32,   57,   70,
 /*    70 */    55,   68,   67,   62,   31,   42,   41,   28,   34,   51,
 /*    80 */    50,   48,   44,   70,   55,   68,   67,   62,   31,   42,
 /*    90 */    41,   28,   34,   51,   50,   45,   70,   55,   68,   67,
 /*   100 */    62,   31,   42,   41,   28,   34,   51,   47,   70,   55,
 /*   110 */    68,   67,   62,   31,   42,   41,   28,   34,   49,   70,
 /*   120 */    55,   68,   67,   62,   31,   42,   41,   28,   33,   70,
 /*   130 */    55,   68,   67,   62,   31,   42,   41,   27,   70,   55,
 /*   140 */    68,   67,   62,   31,   42,   41,   26,   74,   73,   72,
 /*   150 */     3,   75,   25,   24,   23,   22,   70,   55,   68,   67,
 /*   160 */    62,   31,   42,   38,   70,   55,   68,   67,   62,   31,
 /*   170 */    42,   37,   70,   55,   68,   67,   62,   31,   42,   36,
 /*   180 */    70,   55,   68,   67,   62,   31,   42,   35,   70,   55,
 /*   190 */    68,   67,   62,   31,   40,   70,   55,   68,   67,   62,
 /*   200 */    31,   39,   70,   55,   68,   67,   62,   30,   70,   55,
 /*   210 */    68,   67,   62,   29,   70,   55,   68,   67,   66,   70,
 /*   220 */    55,   68,   67,   65,   70,   55,   68,   67,   64,   70,
 /*   230 */    55,   68,   67,   63,   70,   55,   68,   67,   61,   70,
 /*   240 */    55,   68,   67,   60,   70,   55,   68,   67,   59,   14,
 /*   250 */    13,   12,   11,   21,   20,   19,   18,   17,   16,   15,
 /*   260 */    10,    9,    4,    2,   54,   53,   71,    8,   69,    7,
 /*   270 */   122,    6,  122,    5,  122,  122,  122,  122,    1,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    29,   30,   31,   32,   33,   34,   35,   36,   37,   38,
 /*    10 */    39,   40,   41,   42,   43,   44,   45,   46,   30,   31,
 /*    20 */    32,   33,   34,   35,   36,   37,   38,   39,   40,   41,
 /*    30 */    42,   43,   44,   45,   46,   30,   31,   32,   33,   34,
 /*    40 */    35,   36,   37,   38,   39,   40,   41,   42,   43,   44,
 /*    50 */    45,   46,   30,   31,   32,   33,   34,   35,   36,   37,
 /*    60 */    38,   39,   40,   41,   42,   43,   44,   45,   46,   31,
 /*    70 */    32,   33,   34,   35,   36,   37,   38,   39,   40,   41,
 /*    80 */    42,   43,   44,   31,   32,   33,   34,   35,   36,   37,
 /*    90 */    38,   39,   40,   41,   42,   43,   31,   32,   33,   34,
 /*   100 */    35,   36,   37,   38,   39,   40,   41,   42,   31,   32,
 /*   110 */    33,   34,   35,   36,   37,   38,   39,   40,   41,   31,
 /*   120 */    32,   33,   34,   35,   36,   37,   38,   39,   40,   31,
 /*   130 */    32,   33,   34,   35,   36,   37,   38,   39,   31,   32,
 /*   140 */    33,   34,   35,   36,   37,   38,   39,    1,    2,    3,
 /*   150 */     4,    0,    6,    7,    8,    9,   31,   32,   33,   34,
 /*   160 */    35,   36,   37,   38,   31,   32,   33,   34,   35,   36,
 /*   170 */    37,   38,   31,   32,   33,   34,   35,   36,   37,   38,
 /*   180 */    31,   32,   33,   34,   35,   36,   37,   38,   31,   32,
 /*   190 */    33,   34,   35,   36,   37,   31,   32,   33,   34,   35,
 /*   200 */    36,   37,   31,   32,   33,   34,   35,   36,   31,   32,
 /*   210 */    33,   34,   35,   36,   31,   32,   33,   34,   35,   31,
 /*   220 */    32,   33,   34,   35,   31,   32,   33,   34,   35,   31,
 /*   230 */    32,   33,   34,   35,   31,   32,   33,   34,   35,   31,
 /*   240 */    32,   33,   34,   35,   31,   32,   33,   34,   35,   15,
 /*   250 */    16,   17,   18,   10,   11,   12,    6,    7,   13,   14,
 /*   260 */    19,   20,   25,   26,    4,    3,    5,   21,    5,   22,
 /*   270 */    47,   23,   47,   24,   47,   47,   47,   47,   27,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_COUNT (56)
#define YY_SHIFT_MIN   (0)
#define YY_SHIFT_MAX   (263)
static const short yy_shift_ofst[] = {
 /*     0 */   146,  146,  146,  146,  146,  146,  146,  146,  146,  146,
 /*    10 */   146,  146,  146,  146,  146,  146,  146,  146,  146,  146,
 /*    20 */   146,  146,  146,  146,  146,  146,  234,  234,  234,  243,
 /*    30 */   243,  243,  237,  241,  241,  245,  245,  245,  245,  250,
 /*    40 */   250,  245,  250,  251,  249,  248,  249,  247,  248,  246,
 /*    50 */   247,  246,  263,  261,  262,  260,  151,
};
#define YY_REDUCE_USE_DFLT (-30)
#define YY_REDUCE_COUNT (25)
#define YY_REDUCE_MIN   (-29)
#define YY_REDUCE_MAX   (213)
static const short yy_reduce_ofst[] = {
 /*     0 */   -29,   22,    5,  -12,   38,   52,   65,   77,   88,  107,
 /*    10 */    98,  149,  141,  133,  125,  164,  157,  177,  171,  213,
 /*    20 */   208,  203,  198,  193,  188,  183,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   120,  120,  120,  120,  120,  120,  120,  120,  120,  120,
 /*    10 */   120,  120,  120,  120,  120,  120,  120,  120,  120,  120,
 /*    20 */   120,  120,  120,  120,  120,  120,  106,  105,  104,   95,
 /*    30 */    94,   93,  117,  108,  107,  103,  102,  101,  100,   98,
 /*    40 */    97,   99,   96,  120,  116,  114,  115,  112,  113,  110,
 /*    50 */   111,  109,  120,  120,  120,   79,  120,  119,  118,   92,
 /*    60 */    91,   90,   89,   88,   87,   86,   85,   84,   82,   81,
 /*    70 */    80,   83,   78,   77,   76,
};

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyidxMax;                 /* Maximum value of yyidx */
#endif
  int yyerrcnt;                 /* Shifts left before out of the error */
  ParseARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void PreprocessorParseTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "PP_INT_CONST",  "PP_CHAR_CONST",  "PP_ID",       
  "PP_LEFT_PAREN",  "PP_RIGHT_PAREN",  "PP_PLUS",       "PP_DASH",     
  "PP_BANG",       "PP_TILDE",      "PP_STAR",       "PP_SLASH",    
  "PP_PERCENT",    "PP_LEFT_OP",    "PP_RIGHT_OP",   "PP_LESS",     
  "PP_MORE",       "PP_LE_OP",      "PP_GE_OP",      "PP_EQ_OP",    
  "PP_NE_OP",      "PP_AMPERSAND",  "PP_CARET",      "PP_VERTICAL_BAR",
  "PP_AND_OP",     "PP_OR_OP",      "PP_QUESTION",   "PP_COLON",    
  "error",         "in",            "expression",    "literal_constant",
  "id",            "primary_expression",  "postfix_expression",  "unary_expression",
  "multiplicative_expression",  "additive_expression",  "shift_expression",  "relational_expression",
  "equality_expression",  "and_expression",  "exclusive_or_expression",  "inclusive_or_expression",
  "logical_and_expression",  "logical_or_expression",  "conditional_expression",
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "in ::= expression",
 /*   1 */ "literal_constant ::= PP_INT_CONST",
 /*   2 */ "literal_constant ::= PP_CHAR_CONST",
 /*   3 */ "id ::= PP_ID",
 /*   4 */ "primary_expression ::= id",
 /*   5 */ "primary_expression ::= literal_constant",
 /*   6 */ "primary_expression ::= PP_LEFT_PAREN expression PP_RIGHT_PAREN",
 /*   7 */ "postfix_expression ::= primary_expression",
 /*   8 */ "postfix_expression ::= id PP_LEFT_PAREN PP_ID PP_RIGHT_PAREN",
 /*   9 */ "unary_expression ::= postfix_expression",
 /*  10 */ "unary_expression ::= PP_PLUS unary_expression",
 /*  11 */ "unary_expression ::= PP_DASH unary_expression",
 /*  12 */ "unary_expression ::= PP_BANG unary_expression",
 /*  13 */ "unary_expression ::= PP_TILDE unary_expression",
 /*  14 */ "multiplicative_expression ::= unary_expression",
 /*  15 */ "multiplicative_expression ::= multiplicative_expression PP_STAR unary_expression",
 /*  16 */ "multiplicative_expression ::= multiplicative_expression PP_SLASH unary_expression",
 /*  17 */ "multiplicative_expression ::= multiplicative_expression PP_PERCENT unary_expression",
 /*  18 */ "additive_expression ::= multiplicative_expression",
 /*  19 */ "additive_expression ::= additive_expression PP_PLUS multiplicative_expression",
 /*  20 */ "additive_expression ::= additive_expression PP_DASH multiplicative_expression",
 /*  21 */ "shift_expression ::= additive_expression",
 /*  22 */ "shift_expression ::= shift_expression PP_LEFT_OP additive_expression",
 /*  23 */ "shift_expression ::= shift_expression PP_RIGHT_OP additive_expression",
 /*  24 */ "relational_expression ::= shift_expression",
 /*  25 */ "relational_expression ::= relational_expression PP_LESS shift_expression",
 /*  26 */ "relational_expression ::= relational_expression PP_MORE shift_expression",
 /*  27 */ "relational_expression ::= relational_expression PP_LE_OP shift_expression",
 /*  28 */ "relational_expression ::= relational_expression PP_GE_OP shift_expression",
 /*  29 */ "equality_expression ::= relational_expression",
 /*  30 */ "equality_expression ::= equality_expression PP_EQ_OP relational_expression",
 /*  31 */ "equality_expression ::= equality_expression PP_NE_OP relational_expression",
 /*  32 */ "and_expression ::= equality_expression",
 /*  33 */ "and_expression ::= and_expression PP_AMPERSAND equality_expression",
 /*  34 */ "exclusive_or_expression ::= and_expression",
 /*  35 */ "exclusive_or_expression ::= exclusive_or_expression PP_CARET and_expression",
 /*  36 */ "inclusive_or_expression ::= exclusive_or_expression",
 /*  37 */ "inclusive_or_expression ::= inclusive_or_expression PP_VERTICAL_BAR exclusive_or_expression",
 /*  38 */ "logical_and_expression ::= inclusive_or_expression",
 /*  39 */ "logical_and_expression ::= logical_and_expression PP_AND_OP inclusive_or_expression",
 /*  40 */ "logical_or_expression ::= logical_and_expression",
 /*  41 */ "logical_or_expression ::= logical_or_expression PP_OR_OP logical_and_expression",
 /*  42 */ "conditional_expression ::= logical_or_expression",
 /*  43 */ "conditional_expression ::= logical_or_expression PP_QUESTION expression PP_COLON expression",
 /*  44 */ "expression ::= conditional_expression",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to Parse and ParseFree.
*/
void *PreprocessorParseAlloc(void *(*mallocProc)(size_t)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (size_t)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    yyGrowStack(pParser);
#endif
  }
  return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  ParseARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(yyParser *pParser){
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
#ifndef NDEBUG
  if( yyTraceFILE && pParser->yyidx>=0 ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor(pParser, yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/* 
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from ParseAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void PreprocessorParseFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int PreprocessorParseStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyidxMax;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>YY_SHIFT_COUNT
   || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        return yy_find_shift_action(pParser, iFallback);
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( 
#if YY_SHIFT_MIN+YYWILDCARD<0
          j>=0 &&
#endif
#if YY_SHIFT_MAX+YYWILDCARD>=YY_ACTTAB_COUNT
          j<YY_ACTTAB_COUNT &&
#endif
          yy_lookahead[j]==YYWILDCARD
        ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser, YYMINORTYPE *yypMinor){
   ParseARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
   ParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yypParser->yyidx>yypParser->yyidxMax ){
    yypParser->yyidxMax = yypParser->yyidx;
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser, yypMinor);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser, yypMinor);
      return;
    }
  }
#endif
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor = *yypMinor;
#ifndef NDEBUG
  if( yyTraceFILE && yypParser->yyidx>0 ){
    int i;
    fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
    fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE," %s",yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"\n");
  }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 29, 1 },
  { 31, 1 },
  { 31, 1 },
  { 32, 1 },
  { 33, 1 },
  { 33, 1 },
  { 33, 3 },
  { 34, 1 },
  { 34, 4 },
  { 35, 1 },
  { 35, 2 },
  { 35, 2 },
  { 35, 2 },
  { 35, 2 },
  { 36, 1 },
  { 36, 3 },
  { 36, 3 },
  { 36, 3 },
  { 37, 1 },
  { 37, 3 },
  { 37, 3 },
  { 38, 1 },
  { 38, 3 },
  { 38, 3 },
  { 39, 1 },
  { 39, 3 },
  { 39, 3 },
  { 39, 3 },
  { 39, 3 },
  { 40, 1 },
  { 40, 3 },
  { 40, 3 },
  { 41, 1 },
  { 41, 3 },
  { 42, 1 },
  { 42, 3 },
  { 43, 1 },
  { 43, 3 },
  { 44, 1 },
  { 44, 3 },
  { 45, 1 },
  { 45, 3 },
  { 46, 1 },
  { 46, 5 },
  { 30, 1 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  ParseARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0 
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.  
  **
  ** 2007-01-16:  The wireshark project (www.wireshark.org) reports that
  ** without this code, their parser segfaults.  I'm not sure what there
  ** parser is doing to make this happen.  This is the second bug report
  ** from wireshark this week.  Clearly they are stressing Lemon in ways
  ** that it has not been previously stressed...  (SQLite ticket #2172)
  */
  /*memset(&yygotominor, 0, sizeof(yygotominor));*/
  yygotominor = yyzerominor;


  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 0: /* in ::= expression */
#line 21 "Preprocessor.y"
{
	parserState->m_preprocessorConditionResult = yymsp[0].minor.yy9;
}
#line 819 "Preprocessor.c"
        break;
      case 1: /* literal_constant ::= PP_INT_CONST */
#line 27 "Preprocessor.y"
{
	yygotominor.yy9 = ParseNumber( yymsp[0].minor.yy0.stringValue.start, yymsp[0].minor.yy0.stringValue.end );
}
#line 826 "Preprocessor.c"
        break;
      case 2: /* literal_constant ::= PP_CHAR_CONST */
#line 32 "Preprocessor.y"
{
	yygotominor.yy9 = 1;
}
#line 833 "Preprocessor.c"
        break;
      case 3: /* id ::= PP_ID */
#line 39 "Preprocessor.y"
{
	if( yymsp[0].minor.yy0.stringValue == MakeInlineString( "defined" ) )
	{
		parserState->m_expandMacros = false;
	}
	yygotominor.yy0 = yymsp[0].minor.yy0;
}
#line 844 "Preprocessor.c"
        break;
      case 4: /* primary_expression ::= id */
#line 48 "Preprocessor.y"
{
	if( yymsp[0].minor.yy0.stringValue == MakeInlineString( "defined" ) )
	{
		parserState->m_expandMacros = false;
	}
	else
	{
		yygotominor.yy9 = 0;
	}
}
#line 858 "Preprocessor.c"
        break;
      case 5: /* primary_expression ::= literal_constant */
      case 7: /* postfix_expression ::= primary_expression */ yytestcase(yyruleno==7);
      case 9: /* unary_expression ::= postfix_expression */ yytestcase(yyruleno==9);
      case 10: /* unary_expression ::= PP_PLUS unary_expression */ yytestcase(yyruleno==10);
      case 14: /* multiplicative_expression ::= unary_expression */ yytestcase(yyruleno==14);
      case 18: /* additive_expression ::= multiplicative_expression */ yytestcase(yyruleno==18);
      case 21: /* shift_expression ::= additive_expression */ yytestcase(yyruleno==21);
      case 24: /* relational_expression ::= shift_expression */ yytestcase(yyruleno==24);
      case 29: /* equality_expression ::= relational_expression */ yytestcase(yyruleno==29);
      case 32: /* and_expression ::= equality_expression */ yytestcase(yyruleno==32);
      case 34: /* exclusive_or_expression ::= and_expression */ yytestcase(yyruleno==34);
      case 36: /* inclusive_or_expression ::= exclusive_or_expression */ yytestcase(yyruleno==36);
      case 38: /* logical_and_expression ::= inclusive_or_expression */ yytestcase(yyruleno==38);
      case 40: /* logical_or_expression ::= logical_and_expression */ yytestcase(yyruleno==40);
      case 42: /* conditional_expression ::= logical_or_expression */ yytestcase(yyruleno==42);
      case 44: /* expression ::= conditional_expression */ yytestcase(yyruleno==44);
#line 60 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[0].minor.yy9;
}
#line 880 "Preprocessor.c"
        break;
      case 6: /* primary_expression ::= PP_LEFT_PAREN expression PP_RIGHT_PAREN */
#line 65 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-1].minor.yy9;
}
#line 887 "Preprocessor.c"
        break;
      case 8: /* postfix_expression ::= id PP_LEFT_PAREN PP_ID PP_RIGHT_PAREN */
#line 76 "Preprocessor.y"
{
	if( yymsp[-3].minor.yy0.stringValue != MakeInlineString( "defined" ) )
	{
		parserState->ShowMessage( EC_SYNTAX_ERROR, ToString( yymsp[-3].minor.yy0.stringValue ).c_str() );
	}
	else
	{
		if( parserState->m_defines.find(yymsp[-1].minor.yy0.stringValue)  != parserState->m_defines.end() )
		{
			yygotominor.yy9 = 1;
		}
		else
		{
			yygotominor.yy9 = 0;
		}
	}
	parserState->m_expandMacros = true;
}
#line 909 "Preprocessor.c"
        break;
      case 11: /* unary_expression ::= PP_DASH unary_expression */
#line 107 "Preprocessor.y"
{
	yygotominor.yy9 = -yymsp[0].minor.yy9;
}
#line 916 "Preprocessor.c"
        break;
      case 12: /* unary_expression ::= PP_BANG unary_expression */
#line 112 "Preprocessor.y"
{
	yygotominor.yy9 = !yymsp[0].minor.yy9;
}
#line 923 "Preprocessor.c"
        break;
      case 13: /* unary_expression ::= PP_TILDE unary_expression */
#line 117 "Preprocessor.y"
{
	yygotominor.yy9 = ~yymsp[0].minor.yy9;
}
#line 930 "Preprocessor.c"
        break;
      case 15: /* multiplicative_expression ::= multiplicative_expression PP_STAR unary_expression */
#line 128 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 * yymsp[0].minor.yy9;
}
#line 937 "Preprocessor.c"
        break;
      case 16: /* multiplicative_expression ::= multiplicative_expression PP_SLASH unary_expression */
#line 133 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 / yymsp[0].minor.yy9;
}
#line 944 "Preprocessor.c"
        break;
      case 17: /* multiplicative_expression ::= multiplicative_expression PP_PERCENT unary_expression */
#line 138 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 % yymsp[0].minor.yy9;
}
#line 951 "Preprocessor.c"
        break;
      case 19: /* additive_expression ::= additive_expression PP_PLUS multiplicative_expression */
#line 149 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 + yymsp[0].minor.yy9;
}
#line 958 "Preprocessor.c"
        break;
      case 20: /* additive_expression ::= additive_expression PP_DASH multiplicative_expression */
#line 154 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 - yymsp[0].minor.yy9;
}
#line 965 "Preprocessor.c"
        break;
      case 22: /* shift_expression ::= shift_expression PP_LEFT_OP additive_expression */
      case 23: /* shift_expression ::= shift_expression PP_RIGHT_OP additive_expression */ yytestcase(yyruleno==23);
#line 165 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 << yymsp[0].minor.yy9;
}
#line 973 "Preprocessor.c"
        break;
      case 25: /* relational_expression ::= relational_expression PP_LESS shift_expression */
#line 181 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 < yymsp[0].minor.yy9;
}
#line 980 "Preprocessor.c"
        break;
      case 26: /* relational_expression ::= relational_expression PP_MORE shift_expression */
#line 186 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 > yymsp[0].minor.yy9;
}
#line 987 "Preprocessor.c"
        break;
      case 27: /* relational_expression ::= relational_expression PP_LE_OP shift_expression */
#line 191 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 <= yymsp[0].minor.yy9;
}
#line 994 "Preprocessor.c"
        break;
      case 28: /* relational_expression ::= relational_expression PP_GE_OP shift_expression */
#line 196 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 >= yymsp[0].minor.yy9;
}
#line 1001 "Preprocessor.c"
        break;
      case 30: /* equality_expression ::= equality_expression PP_EQ_OP relational_expression */
#line 207 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 == yymsp[0].minor.yy9;
}
#line 1008 "Preprocessor.c"
        break;
      case 31: /* equality_expression ::= equality_expression PP_NE_OP relational_expression */
#line 212 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 != yymsp[0].minor.yy9;
}
#line 1015 "Preprocessor.c"
        break;
      case 33: /* and_expression ::= and_expression PP_AMPERSAND equality_expression */
#line 223 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 & yymsp[0].minor.yy9;
}
#line 1022 "Preprocessor.c"
        break;
      case 35: /* exclusive_or_expression ::= exclusive_or_expression PP_CARET and_expression */
#line 234 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 ^ yymsp[0].minor.yy9;
}
#line 1029 "Preprocessor.c"
        break;
      case 37: /* inclusive_or_expression ::= inclusive_or_expression PP_VERTICAL_BAR exclusive_or_expression */
#line 245 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 | yymsp[0].minor.yy9;
}
#line 1036 "Preprocessor.c"
        break;
      case 39: /* logical_and_expression ::= logical_and_expression PP_AND_OP inclusive_or_expression */
#line 256 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 && yymsp[0].minor.yy9;
}
#line 1043 "Preprocessor.c"
        break;
      case 41: /* logical_or_expression ::= logical_or_expression PP_OR_OP logical_and_expression */
#line 267 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-2].minor.yy9 || yymsp[0].minor.yy9;
}
#line 1050 "Preprocessor.c"
        break;
      case 43: /* conditional_expression ::= logical_or_expression PP_QUESTION expression PP_COLON expression */
#line 278 "Preprocessor.y"
{
	yygotominor.yy9 = yymsp[-4].minor.yy9 ? yymsp[-2].minor.yy9 : yymsp[0].minor.yy9;
}
#line 1057 "Preprocessor.c"
        break;
      default:
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = (YYACTIONTYPE)yyact;
      yymsp->major = (YYCODETYPE)yygoto;
      yymsp->minor = yygotominor;
    }else
#endif
    {
      yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else{
    assert( yyact == YYNSTATE + YYNRULE + 1 );
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  ParseARG_FETCH;
#define TOKEN (yyminor.yy0)
#line 18 "Preprocessor.y"
 parserState->ShowMessage( TOKEN, EC_SYNTAX_ERROR, ToString( TOKEN.stringValue ).c_str() ); 
#line 1121 "Preprocessor.c"
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "ParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void PreprocessorParse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  ParseTOKENTYPE yyminor       /* The value for the token */
  ParseARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
      yyminorunion = yyzerominor;
      yyStackOverflow(yypParser, &yyminorunion);
      return;
    }
#endif
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  ParseARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact<YYNSTATE ){
      assert( !yyendofinput );  /* Impossible to shift the $ token */
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(yypParser,yyact-YYNSTATE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor,yyminorunion);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}