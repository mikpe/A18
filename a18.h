/*
    HEADER:         CUG149;
    TITLE:          1805A Cross-Assembler (Portable);
    FILENAME:       A18.H;
    VERSION:        2.5;
    DATE:           08/27/1988;

    DESCRIPTION:    "This program lets you use your computer to assemble
                    code for the RCA 1802, 1804, 1805, 1805A, 1806, and
                    1806A microprocessors.  The program is written in
                    portable C rather than BDS C.  All assembler features
                    are supported except relocation, linkage, and macros.";

    KEYWORDS:       Software Development, Assemblers, Cross-Assemblers,
                    RCA, CDP1802, CDP1805A;

    SEE-ALSO:       CUG113, 1802 Cross-Assembler;

    SYSTEM:         CP/M-80, CP/M-86, HP-UX, MSDOS, PCDOS, QNIX;

    COMPILERS:      Aztec C86, Aztec CII, CI-C86, Eco-C, Eco-C88, HP-UX,
                    Lattice C, Microsoft C, QNIX C;

    WARNINGS:       "This program has compiled successfully on 2 UNIX
                    compilers, 5 MSDOS compilers, and 2 CP/M compilers.
                    A port to BDS C would be extremely difficult, but see
                    volume CUG113.  A port to Toolworks C is untried."

    AUTHORS:        William C. Colley III;
*/

/*
            1805A Cross-Assembler in Portable C

            Copyright (c) 1985 William C. Colley, III

Revision History:

Ver     Date        Description

2.0     MAY 1985    Recoded from BDS C version 1.1.  WCC3.

2.1     AUG 1985    Greatly shortened the routines find_symbol() and
                    new_symbol().  Fixed bugs in expression evaluator.
                    Added compilation instructions for Aztec C86,
                    Microsoft C, and QNIX C.  Added optional optimizations
                    for 16-bit machines.  Adjusted structure members for
                    fussy compilers.  WCC3.

2.2     SEP 1985    Added the INCL pseudo-op and associated stuff.  WCC3.

2.3     JUL 1986    Added compilation instructions and tweaks for CI-C86,
                    Eco-C88, and Lattice C. WCC3.

2.4     JAN 1987    Fixed bug that made "BYTE 0," legal syntax.  WCC3.

2.5     AUG 1988    Fixed a bug in the command line parser that puts it
                    into a VERY long loop if the user types a command line
                    like "A18 FILE.ASM -L".  WCC3 per Alex Cameron.

This header file contains the global constants and data type definitions for
all modules of the cross-assembler.  This also seems a good place to put the
compilation and linkage instructions for the animal.  This list currently
includes the following compilers:

        Compiler Name               Op. Sys.        Processor

    1)  Aztec C86                   CP/M-86         8086, 8088
                                    MSDOS/PCDOS

    2)  AZTEC C II                  CP/M-80         8080, Z-80

    3)  Computer Innovations C86    MSDOS/PCDOS     8086, 8088

    4)  Eco-C                       CP/M-80         Z-80

    5)  Eco-C88                     MSDOS/PCDOS     8086, 8088

    6)  HP C                        HP-UX           68000

    7)  Lattice C                   MSDOS/PCDOS     8086, 8088

    8)  Microsoft C                 MSDOS/PCDOS     8086, 8088

    9)  QNIX C                      QNIX            8086, 8088

Further additions will be made to the list as users feed the information to
me.  This particularly applies to UNIX and IBM-PC compilers.

Compile-assemble-link instructions for this program under various compilers
and operating systems:

    1)  Aztec C86:

    A)  Uncomment out the "#define AZTEC_C 1" line and comment out all
        other compiler names in A18.H.

    B)  Assuming that all files are on drive A:, run the following sequence
        of command lines:

        A>cc a18
        A>cc a18eval
        A>cc a18util
        A>ln a18.o a18eval.o a18util.o -lc
        A>era a18*.o

    2)  Aztec CII (version 1.06B):

    A)  Uncomment out the "#define AZTEC_C 1" line and comment out all
        other compiler names in A18.H.

    B)  Assuming the C compiler is called "CC.COM" and all files are
        on drive A:, run the following sequence of command lines:

        A>cc a18
        A>as -zap a18
        A>cc a18eval
        A>as -zap a18eval
        A>cc a18util
        A>as -zap a18util
        A>ln a18.o a18eval.o a18util.o -lc
        A>era a18*.o

    3)  Computer Innovations C86:

    A)  Uncomment out the "#define CI_C86 1" line and comment out all
        other compiler names in A18.H.

    B)  Compile the files A18.C, A18EVAL.C, and A18UTIL.C.  Link
        according to instructions that come with the compiler.

    4)  Eco-C (CP/M-80 version 3.10):

    A)  Uncomment out the "#define ECO_C 1" line and comment out all
        other compiler names in A18.H.

    B)  Assuming all files are on drive A:, run the following sequence of
        command lines:

        A>cp a18 -i -m
        A>cp a18eval -i -m
        A>cp a18util -i -m
        A>l80 a18,a18eval,a18util,a18/n/e
        A>era a18*.mac
        A>era a18*.rel

    5)  Eco-C88:

    A)  Uncomment out the "#define ECO_C 1" line and comment out all
        other compiler names in A18.H.

    B)  Compile the files A18.C, A18EVAL.C, and A18UTIL.C.  Link
        according to instructions that come with the compiler.

    6)  HP-UX (a UNIX look-alike running on an HP-9000 Series 200/500,
        68000-based machine):

    A)  Uncomment out the "#define HP_UX 1" line and comment out all
        other compiler names in A18.H.

    B)  Run the following command line:

        . cc a18.c a18eval.c a18util.c

    7)  Lattice C:

    A)  Uncomment out the "#define LATTICE_C 1" line and comment out all
        other compiler names in A18.H.

    B)  Compile the files A18.C, A18EVAL.C, and A18UTIL.C.  Link
        according to instructions that come with the compiler.

    8)  Microsoft C (version 3.00):

    A)  Uncomment out the "#define MICROSOFT_C 1" line and comment out
        all other compiler names in A68.H.

    B)  Run the following command line:

        C>cl a18.c a18eval.c a18util.c

    9)  QNIX C:

    A)  Uncomment out the "#define QNIX 1" line and comment out all other
        compiler names in A18.H.

    B)  Run the following command line:

        . cc a18.c a18eval.c a18util.c

Note that, under CP/M-80, you can't re-execute a core image from a previous
assembly run with the "@.COM" trick. This technique is incompatible with the
Aztec CII compiler, so I didn't bother to support it at all.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef MODERN
#include <ctype.h>
int popc();
#endif

/*  Comment out all but the line containing the name of your compiler:    */

/* #define    AZTEC_C        1                    */
/* #define    CI_C86         1                    */
/* #define    ECO_C          1                    */
/* #define    HP_UX          1                    */
/* #define    LATTICE_C      1                    */
/* #define    MICROSOFT_C    1                    */
/* #define    QNIX           1                    */

/*  Compiler dependencies:                        */

#ifdef AZTEC_C
#define     getc(f)     agetc(f)
#define     putc(c,f)   aputc(c,f)
#endif

#ifndef ECO_C
#define     FALSE       0
#define     TRUE        (!0)
#endif

#ifdef  LATTICE_C
#define     void        int
#endif

#ifdef  QNIX
#define     fprintf     tfprintf
#define     printf      tprintf
#endif

/*  On 8-bit machines, the static type is as efficient as the register  */
/*  type and far more efficient than the auto type.  On larger machines */
/*  such as the 8086 family, this is not necessarily the case.  To      */
/*  let you experiment to see what generates the fastest, smallest code */
/*  for your machine, I have declared internal scratch variables in     */
/*  functions "SCRATCH int", "SCRATCH unsigned", etc.  A SCRATCH        */
/*  varible is made static below, but you might want to try register    */
/*  instead.                                */

#ifdef MODERN
#define SCRATCH
#else
#define     SCRATCH     static
#endif

/*  A slow, but portable way of cracking an unsigned into its various   */
/*  component parts:                                                    */

#define     clamp(u)    ((u) &= 0xffff)
#define     high(u)     (((u) >> 8) & 0xff)
#define     low(u)      ((u) & 0xff)
#define     word(u)     ((u) & 0xffff)

/*  The longest source line the assembler can hold without exploding:   */

#define     MAXLINE     255

/*  The largest amount of code that can be generated for one line       */

#define     MAXOBJ      1024

/*  The maximum number of source files that can be open simultaneously: */

#define     FILES       4

/*  The fatal error messages generated by the assembler:                */

#define     ASMOPEN     "Source File Did Not Open"
#define     ASMREAD     "Error Reading Source File"
#define     DSKFULL     "Disk or Directory Full"
#define     FLOFLOW     "File Stack Overflow"
#define     HEXOPEN     "Object File Did Not Open"
#define     IFOFLOW     "If Stack Overflow"
#define     LSTOPEN     "Listing File Did Not Open"
#define     NOASM       "No Source File Specified"
#define     SYMBOLS     "Too Many Symbols"
#define     RAWOPEN     "Binary File Did Not Open"
#define     SHROPEN     "Shared Symbol File Did Not Open"
#define     RANGE       "Value Out of Range"

/*  The warning messages generated by the assembler:                    */

#define     BADOPT      "Illegal Option Ignored"
#define     NOHEX       "-o Option Ignored -- No File Name"
#define     NOLST       "-l Option Ignored -- No File Name"
#define     NOSHR       "-s Option Ignored -- No File Name"
#define     TWOASM      "Extra Source File Ignored"
#define     TWOHEX      "Extra Object File Ignored"
#define     TWOLST      "Extra Listing File Ignored"
#define     TWOSHR      "Extra Shared Symbol File Ignored"

/*  Line assembler (A18.C) constants:                                   */

#define     BIGINST     4       /* longest instruction length           */
#define     IFDEPTH     16      /* maximum IF nesting level             */
#define     PREBYTE     0x68    /* processor's opcode prebyte           */
#define     GLO         0x80    /* processor's GLO R0 opcode            */
#define     GHI         0x90    /* processor's GHI R0 opcode            */
#define     IRX         0x60    /* processor's IRX instruction          */
#define     LDX         0xf0    /* processor's LDX instruction          */
#define     LDXA        0x72    /* processor's LDXA instruction         */
#define     STXD        0x73    /* processor's STXD instruction         */
#define     PLO         0xa0    /* processor's PLO R0 opcode            */
#define     PHI         0xb0    /* processor's PHI R0 opcode            */
#define     NOP         0xc4    /* processor's NOP opcode               */
#define     LDI         0xf8    /* processor's LDI opcode               */
#define     DEC         0x20    /* processor's DEC opcode               */
#define     ON          1       /* assembly turned on                   */
#define     OFF         -1      /* assembly turned off                  */

/*  Line assembler (A18.C) opcode attribute word flag masks:            */

#define     PSEUDO      0x200   /* is pseudo-op                         */
#define     ISIF        0x100   /* is IF, ELSE, or ENDI                 */
#define     IS1805      0x080   /* is 1805A-only opcode                 */
#define     REGTYP      0x060   /* register argument type:              */
#define     IOPORT      0x060   /*    i/o port                          */
#define     NOT_R0      0x040   /*    any register but R0               */
#define     ANY         0x020   /*    any register                      */
#define     NUMTYP      0x018   /* numerical argument type:             */
#define     SIXTN       0x018   /*    16-bit word                       */
#define     BRANCH      0x010   /*    8-bit branch target               */
#define     IMMED       0x008   /*    8-bit immediate    byte           */
#define     BYTES       0x007   /* number of instruction bytes          */

/*  Line assembler (A18.C) pseudo-op opcode token values:               */

#define     OP_ALIGN    1
#define     OP_BINCL    2
#define     OP_BLK      3
#define     OP_BRNZ     4
#define     OP_BYTE     5
#define     OP_BRZ      6
#define     OP_CALL     7
#define     OP_CPU      8
#define     OP_DB       9
#define     OP_DBNZ     10
#define     OP_DLBNZ    11
#define     OP_DS       12
#define     OP_DW       13
#define     OP_EJCT     14
#define     OP_ELSE     15
#define     OP_END      16
#define     OP_ENDI     17
#define     OP_EQU      18
#define     OP_FILL     19
#define     OP_IF       20
#define     OP_INCL     21
#define     OP_LBRNZ    22
#define     OP_LBRZ     23
#define     OP_LOAD     24
#define     OP_MOV      25
#define     OP_RADC     26
#define     OP_RADCI    27
#define     OP_RADD     28
#define     OP_RADI     29
#define     OP_RAND     30
#define     OP_RANI     31
#define     OP_RETN     32
#define     OP_RLD      33
#define     OP_RLDI     34
#define     OP_ROR      35
#define     OP_RORI     36
#define     OP_RSBB     37
#define     OP_RSBBI    38
#define     OP_RSHL     39
#define     OP_RSHLC    40
#define     OP_RSHR     41
#define     OP_RSHRC    42
#define     OP_RSUB     43
#define     OP_RSUBI    44
#define     OP_RXOR     45
#define     OP_RXRI     46
#define     OP_RCLR     47
#define     OP_ORG      48
#define     OP_PAGE     49
#define     OP_POP      50
#define     OP_PUSH     51
#define     OP_SET      52
#define     OP_SHARED   53
#define     OP_TEXT     54
#define     OP_TEXTZ    55
#define     OP_TITL     56
#define     OP_WORD     57

/*  Lexical analyzer (A18EVAL.C) token buffer and stream pointer:       */

typedef struct {
    unsigned attr;
    unsigned valu;
    char sval[MAXLINE + 1];
} TOKEN;

/*  Lexical analyzer (A18EVAL.C) token attribute values:                */

#define     EOL         0       /*  end of line                         */
#define     SEP         1       /*  field separator                     */
#define     OPR         2       /*  operator                            */
#define     STR         3       /*  character string                    */
#define     VAL         4       /*  value                               */

/*  Lexical analyzer (A18EVAL.C) token attribute word flag masks:       */

#define     BINARY      0x8000  /*  Operator: is binary operator        */
#define     UNARY       0x4000  /*            is unary operator         */
#define     PREC        0x0f00  /*            precedence                */

#define     FORWD       0x8000  /*  Value:    is forward referenced     */
#define     SOFT        0x4000  /*            is redefinable            */

#define     TYPE        0x000f  /*  All:      token type                */

/*  Lexical analyzer (A18EVAL.C) operator token values (unlisted ones   */
/*  use ASCII characters):                                              */

#define     AND         0
#define     GE          1
#define     HIGH        2
#define     LE          3
#define     LOW         4
#define     MOD         5
#define     NE          6
#define     NOT         7
#define     OR          8
#define     SHR         9
#define     SHL         10
#define     XOR         11

/*  Lexical analyzer (A18EVAL.C) operator precedence values:            */

#define     UOP1        0x0000  /*  unary +, unary -                    */
#define     MULT        0x0100  /*  *, /, MOD, SHL, SHR                 */
#define     ADDIT       0x0200  /*  binary +, binary -                  */
#define     RELAT       0x0300  /*  >, >=, =, <=, <, <>                 */
#define     UOP2        0x0400  /*  NOT                                 */
#define     LOG1        0x0500  /*  AND                                 */
#define     LOG2        0x0600  /*  OR, XOR                             */
#define     UOP3        0x0700  /*  HIGH, LOW                           */
#define     RPREN       0x0800  /*  )                                   */
#define     LPREN       0x0900  /*  (                                   */
#define     ENDEX       0x0a00  /*  end of expression                   */
#define     START       0x0b00  /*  beginning of expression             */

/*  Utility package (A18UTIL.C) symbol table routines:                  */

struct _symbol {
    unsigned attr;
    unsigned valu;
    int shared;
    struct _symbol *left, *right;
    char sname[1];
};

typedef struct _symbol SYMBOL;

#define    SYMCOLS        4

/*  Utility package (A18UTIL.C) opcode/operator table routines:         */

typedef struct {
    unsigned attr;
    unsigned valu;
    char oname[7];
} OPCODE;

/*  Utility package (A18UTIL.C) hex file output routines:               */

typedef struct {
    char* name;
    unsigned valu;
} PREDEFINED;

#define    HEXSIZE        32

/* from a18eval.c */
unsigned expr(void);
int isalph(char c);
TOKEN *lex(void);
int newline(void);
int popc(void);
void pops(char *s);
void pushc(char c);
void trash(void);

/* from a18util.c */
void error(char code);
void fatal_error(char *msg);
OPCODE *find_code(char *nam);
OPCODE *find_operator(char *nam);
SYMBOL *find_symbol(char *nam, int label);
void hclose(void);
void hopen(char *nam);
void hputc(unsigned c);
void hseek(unsigned a);
void lclose(void);
void lopen(char *nam);
void lputs(void);
SYMBOL *new_symbol(char *nam);
void rclose(void);
void ropen(char *nam);
void rputc(unsigned c);
void rseek(unsigned a);
void sclose(void);
void sopen(char *nam);
void unlex(void);
void warning(char *msg);
