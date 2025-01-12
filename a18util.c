/*
    HEADER:         CUG149;
    TITLE:          1805A Cross-Assembler (Portable);
    FILENAME:       A18.C;
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

This module contains the following utility packages:

    1)  symbol table building and searching

    2)  opcode and operator table searching

    3)  listing file output

    4)  hex file output

    5)  error flagging
*/

/*  Get global goodies:                                                 */

#include "a18.h"

/*  Local prototypes:                                                   */

static void check_page(void);
static void list_sym(SYMBOL *sp);
static OPCODE *mybsearch(OPCODE *lo, OPCODE *hi, char *nam);
static void putb(unsigned b);
static void record(unsigned typ);
static int ustrcmp(char *s, char *t);
static void write_shared(SYMBOL *sp);

/*  Make sure that MSDOS compilers using the large memory model know    */
/*  that calloc() returns pointer to char as an MSDOS far pointer is    */
/*  NOT compatible with the int type as is usually the case.            */

#ifndef MODERN
char *calloc();
#endif

/*  Get access to global mailboxes defined in A18.C:                    */

extern char errcode, line[], title[];
extern int eject, listhex;
extern unsigned address, bytes, errors, listleft, obj[], pagelen;

/*  The symbol table is a binary tree of variable-length blocks drawn   */
/*  from the heap with the calloc() function.  The root pointer lives   */
/*  here:                                                               */

SYMBOL *sroot = NULL;
SYMBOL* global = NULL;
char composite[64];

/*  Add new symbol to symbol table.  Returns pointer to symbol even if  */
/*  the symbol already exists.  If there's not enough memory to store   */
/*  the new symbol, a fatal error occurs.                               */

SYMBOL *new_symbol(char *nam)
{
    SCRATCH int i;
    SCRATCH char* n;
    SCRATCH SYMBOL **p, *q;

    if ((*nam == '.') && (global != NULL)) {
        n = composite;
        strcpy(n, global->sname);
        strcat(n, nam);
    }
    else {
        n = nam;
    }

    for (p = &sroot; (q = *p) && (i = strcmp(n,q -> sname)); )
        p = i < 0 ? &(q -> left) : &(q -> right);
    if (!q) {
        if ((*p = q = (SYMBOL*)calloc(1, sizeof(SYMBOL) + strlen(n))) != NULL) {
            strcpy(q->sname, n);
            q->shared = FALSE;
        }
        else
            fatal_error(SYMBOLS);
    }

    if (*nam != '.') {
        global = q;
    }

    return q;
}

/*  Look up symbol in symbol table.  Returns pointer to symbol or NULL  */
/*  if symbol not found.                                                */

SYMBOL *find_symbol(char *nam, int label)
{
    SCRATCH int i;
    SCRATCH char* n;
    SCRATCH SYMBOL *p;

    if ((*nam == '.') && (global != NULL)) {
        n = composite;
        strcpy(n, global->sname);
        strcat(n, nam);
    }
    else {
        n = nam;
    }

    for (p = sroot; p && (i = strcmp(n, p->sname));
         p = i < 0 ? p->left : p->right);

    if (label && (*nam != '.')) {
        global = p;
    }

    return p;
}

/*  Opcode table search routine.  This routine pats down the opcode     */
/*  table for a given opcode and returns either a pointer to it or      */
/*  NULL if the opcode doesn't exist.                                   */

OPCODE *find_code(char *nam)
{
    static OPCODE opctbl[] = {
    { 1,                        0x74,       "ADC"    },
    { IMMED + 2,                0x7c,       "ADCI"   },
    { 1,                        0xf4,       "ADD"    },
    { IMMED + 2,                0xfc,       "ADI"    },
    { PSEUDO,                   OP_ALIGN,   "ALIGN"  },
    { 1,                        0xf2,       "AND"    },
    { IMMED + 2,                0xfa,       "ANI"    },
    { BRANCH + 2,               0x34,       "B1"     },
    { BRANCH + 2,               0x35,       "B2"     },
    { BRANCH + 2,               0x36,       "B3"     },
    { BRANCH + 2,               0x37,       "B4"     },
    { IS1805 + BRANCH + 3,      0x3e,       "BCI"    },
    { BRANCH + 2,               0x33,       "BDF"    },
    { BRANCH + 2,               0x33,       "BGE"    },
    { PSEUDO,                   OP_BINCL,   "BINCL"  },
    { BRANCH + 2,               0x3b,       "BL"     },
    { PSEUDO,                   OP_BLK,     "BLK"    },
    { BRANCH + 2,               0x3b,       "BM"     },
    { BRANCH + 2,               0x3c,       "BN1"    },
    { BRANCH + 2,               0x3d,       "BN2"    },
    { BRANCH + 2,               0x3e,       "BN3"    },
    { BRANCH + 2,               0x3f,       "BN4"    },
    { BRANCH + 2,               0x3b,       "BNF"    },
    { BRANCH + 2,               0x39,       "BNQ"    },
    { BRANCH + 2,               0x3a,       "BNZ"    },
    { BRANCH + 2,               0x33,       "BPZ"    },
    { BRANCH + 2,               0x31,       "BQ"     },
    { BRANCH + 2,               0x30,       "BR"     },
    { PSEUDO,                   OP_BRNZ,    "BRNZ"   },
    { PSEUDO,                   OP_BRZ,     "BRZ"    },
    { IS1805 + BRANCH + 3,      0x3f,       "BXI"    },
    { PSEUDO,                   OP_BYTE,    "BYTE"   },
    { BRANCH + 2,               0x32,       "BZ"     },
    { PSEUDO,                   OP_CALL,    "CALL"   },
    { IS1805 + 2,               0x0d,       "CID"    },
    { IS1805 + 2,               0x0c,       "CIE"    },
    { PSEUDO,                   OP_CPU,     "CPU"    },
    { IS1805 + IMMED + 3,       0x7c,       "DACI"   },
    { IS1805 + 2,               0x74,       "DADC"   },
    { IS1805 + 2,               0xf4,       "DADD"   },
    { IS1805 + IMMED + 3,       0xfc,       "DADI"   },
    { PSEUDO,                   OP_DB,      "DB"     },
    { PSEUDO,                   OP_DBNZ,    "DBNZ"   },
    { ANY + 1,                  0x20,       "DEC"    },
    { 1,                        0x71,       "DIS"    },
    { PSEUDO,                   OP_DLBNZ,   "DLBNZ"  },
    { PSEUDO,                   OP_DS,      "DS"     },
    { IS1805 + 2,               0x76,       "DSAV"   },
    { IS1805 + IMMED + 3,       0x7f,       "DSBI"   },
    { IS1805 + 2,               0xf7,       "DSM"    },
    { IS1805 + 2,               0x77,       "DSMB"   },
    { IS1805 + IMMED + 3,       0xff,       "DSMI"   },
    { IS1805 + 2,               0x01,       "DTC"    },
    { PSEUDO,                   OP_DW,      "DW"     },
    { PSEUDO,                   OP_EJCT,    "EJCT"   },
    { PSEUDO + ISIF,            OP_ELSE,    "ELSE"   },
    { PSEUDO,                   OP_END,     "END"    },
    { PSEUDO + ISIF,            OP_ENDI,    "ENDI"   },
    { PSEUDO,                   OP_EQU,     "EQU"    },
    { IS1805 + 2,               0x09,       "ETQ"    },
    { PSEUDO,                   OP_FILL,    "FILL"   },
    { IS1805 + 2,               0x08,       "GEC"    },
    { ANY + 1,                  0x90,       "GHI"    },
    { ANY + 1,                  0x80,       "GLO"    },
    { 1,                        0x00,       "IDL"    },
    { PSEUDO + ISIF,            OP_IF,      "IF"     },
    { ANY + 1,                  0x10,       "INC"    },
    { PSEUDO,                   OP_INCL,    "INCL"   },
    { IOPORT + 1,               0x68,       "INP"    },
    { 1,                        0x60,       "IRX"    },
    { SIXTN + 3,                0xc3,       "LBDF"   },
    { SIXTN + 3,                0xcb,       "LBNF"   },
    { SIXTN + 3,                0xc9,       "LBNQ"   },
    { SIXTN + 3,                0xca,       "LBNZ"   },
    { SIXTN + 3,                0xc1,       "LBQ"    },
    { SIXTN + 3,                0xc0,       "LBR"    },
    { PSEUDO,                   OP_LBRNZ,   "LBRNZ"  },
    { PSEUDO,                   OP_LBRZ,    "LBRZ"   },
    { SIXTN + 3,                0xc2,       "LBZ"    },
    { ANY + 1,                  0x40,       "LDA"    },
    { IS1805 + 2,               0x06,       "LDC"    },
    { IMMED + 2,                0xf8,       "LDI"    },
    { NOT_R0 + 1,               0x00,       "LDN"    },
    { 1,                        0xf0,       "LDX"    },
    { 1,                        0x72,       "LDXA"   },
    { PSEUDO,                   OP_LOAD,    "LOAD"   },
    { 1,                        0xcf,       "LSDF"   },
    { 1,                        0xcc,       "LSIE"   },
    { 1,                        0xc8,       "LSKP"   },
    { 1,                        0xc7,       "LSNF"   },
    { 1,                        0xc5,       "LSNQ"   },
    { 1,                        0xc6,       "LSNZ"   },
    { 1,                        0xcd,       "LSQ"    },
    { 1,                        0xce,       "LSZ"    },
    { 1,                        0x79,       "MARK"   },
    { PSEUDO,                   OP_MOV,     "MOV"    },
    { BRANCH + 2,               0x38,       "NBR"    },
    { SIXTN + 3,                0xc8,       "NLBR"   },
    { 1,                        0xc4,       "NOP"    },
    { 1,                        0xf1,       "OR"     },
    { PSEUDO,                   OP_ORG,     "ORG"    },
    { IMMED + 2,                0xf9,       "ORI"    },
    { IOPORT + 1,               0x60,       "OUT"    },
    { PSEUDO,                   OP_PAGE,    "PAGE"   },
    { ANY + 1,                  0xb0,       "PHI"    },
    { ANY + 1,                  0xa0,       "PLO"    },
    { PSEUDO,                   OP_POP,     "POP"    },
    { PSEUDO,                   OP_PUSH,    "PUSH"   },
    { PSEUDO,                   OP_RADC,    "RADC"   },
    { PSEUDO,                   OP_RADCI,   "RADCI"  },
    { PSEUDO,                   OP_RADD,    "RADD"   },
    { PSEUDO,                   OP_RADI,    "RADI"   },
    { PSEUDO,                   OP_RAND,    "RAND"   },
    { PSEUDO,                   OP_RANI,    "RANI"   },
    { PSEUDO,                   OP_RCLR,    "RCLR"   },
    { 1,                        0x7a,       "REQ"    },
    { 1,                        0x70,       "RET"    },
    { PSEUDO,                   OP_RETN,    "RETN"   },
    { PSEUDO,                   OP_RLD,     "RLD"    },
    { PSEUDO,                   OP_RLDI,    "RLDI"   },
    { IS1805 + ANY + 2,         0x60,       "RLXA"   },
    { IS1805 + ANY + 2,         0xb0,       "RNX"    },
    { PSEUDO,                   OP_ROR,     "ROR"    },
    { PSEUDO,                   OP_RORI,    "RORI"   },
    { PSEUDO,                   OP_RSHLC,   "RRSHL"  },
    { PSEUDO,                   OP_RSHRC,   "RRSHR"  },
    { PSEUDO,                   OP_RSBB,    "RSBB"   },
    { PSEUDO,                   OP_RSBBI,   "RSBBI"  },
    { PSEUDO,                   OP_RSHL,    "RSHL"   },
    { PSEUDO,                   OP_RSHLC,   "RSHLC"  },
    { PSEUDO,                   OP_RSHR,    "RSHR"   },
    { PSEUDO,                   OP_RSHRC,   "RSHRC"  },
    { PSEUDO,                   OP_RSUB,    "RSUB"   },
    { PSEUDO,                   OP_RSUBI,   "RSUBI"  },
    { IS1805 + ANY + 2,         0xa0,       "RSXD"   },
    { PSEUDO,                   OP_RXOR,    "RXOR"   },
    { PSEUDO,                   OP_RXRI,    "RXRI"   },
    { 1,                        0x78,       "SAV"    },
    { IS1805 + ANY + SIXTN + 4, 0x80,       "SCAL"   },
    { IS1805 + 2,               0x05,       "SCM1"   },
    { IS1805 + 2,               0x03,       "SCM2"   },
    { 1,                        0xf5,       "SD"     },
    { 1,                        0x75,       "SDB"    },
    { IMMED + 2,                0x7d,       "SDBI"   },
    { IMMED + 2,                0xfd,       "SDI"    },
    { ANY + 1,                  0xd0,       "SEP"    },
    { 1,                        0x7b,       "SEQ"    },
    { PSEUDO,                   OP_SET,     "SET"    },
    { ANY + 1,                  0xe0,       "SEX"    },
    { PSEUDO,                   OP_SHARED,  "SHARED" },
    { 1,                        0xfe,       "SHL"    },
    { 1,                        0x7e,       "SHLC"   },
    { 1,                        0xf6,       "SHR"    },
    { 1,                        0x76,       "SHRC"   },
    { 1,                        0x38,       "SKP"    },
    { 1,                        0xf7,       "SM"     },
    { 1,                        0x77,       "SMB"    },
    { IMMED + 2,                0x7f,       "SMBI"   },
    { IMMED + 2,                0xff,       "SMI"    },
    { IS1805 + 2,               0x04,       "SPM1"   },
    { IS1805 + 2,               0x02,       "SPM2"   },
    { IS1805 + ANY + 2,         0x90,       "SRET"   },
    { IS1805 + 2,               0x07,       "STM"    },
    { IS1805 + 2,               0x00,       "STPC"   },
    { ANY + 1,                  0x50,       "STR"    },
    { 1,                        0x73,       "STXD"   },
    { PSEUDO,                   OP_TEXT,    "TEXT"   },
    { PSEUDO,                   OP_TEXTZ,   "TEXTZ"  },
    { PSEUDO,                   OP_TITL,    "TITL"   },
    { PSEUDO,                   OP_WORD,    "WORD"   },
    { IS1805 + 2,               0x0b,       "XID"    },
    { IS1805 + 2,               0x0a,       "XIE"    },
    { 1,                        0xf3,       "XOR"    },
    { IMMED + 2,                0xfb,       "XRI"    }
    };

    return mybsearch(opctbl,opctbl + (sizeof(opctbl) / sizeof(OPCODE)),nam);
}

/*  Operator table search routine.  This routine pats down the          */
/*  operator table for a given operator and returns either a pointer    */
/*  to it or NULL if the opcode doesn't exist.                          */

OPCODE *find_operator(char *nam)
{
    static OPCODE oprtbl[] = {
    { UNARY  + UOP1  + OPR,     '$',    "$"     },
    { BINARY + LOG1  + OPR,     AND,    "AND"   },
    { BINARY + RELAT + OPR,     '=',    "EQ"    },
    { BINARY + RELAT + OPR,     GE,     "GE"    },
    { BINARY + RELAT + OPR,     '>',    "GT"    },
    { UNARY  + UOP3  + OPR,     HIGH,   "HIGH"  },
    { BINARY + RELAT + OPR,     LE,     "LE"    },
    { UNARY  + UOP3  + OPR,     LOW,    "LOW"   },
    { BINARY + RELAT + OPR,     '<',    "LT"    },
    { BINARY + MULT  + OPR,     MOD,    "MOD"   },
    { BINARY + RELAT + OPR,     NE,     "NE"    },
    { UNARY  + UOP2  + OPR,     NOT,    "NOT"   },
    { BINARY + LOG2  + OPR,     OR,     "OR"    },
    { BINARY + MULT  + OPR,     SHL,    "SHL"   },
    { BINARY + MULT  + OPR,     SHR,    "SHR"   },
    { BINARY + LOG2  + OPR,     XOR,    "XOR"   }
    };

    return mybsearch(oprtbl,oprtbl + (sizeof(oprtbl) / sizeof(OPCODE)),nam);
}

static int ustrcmp(char *s, char *t)
{
    SCRATCH int i;

    while (!(i = toupper(*s++) - toupper(*t)) && *t++);
    return i;
}

static OPCODE *mybsearch(OPCODE *lo, OPCODE *hi, char *nam)
{
    SCRATCH int i;
    SCRATCH OPCODE *chk;

    for (;;) {
        chk = lo + (hi - lo) / 2;
        if (!(i = ustrcmp(chk -> oname,nam)))
            return chk;
        if (chk == lo)
            return NULL;
        if (i < 0)
            lo = chk;
        else
            hi = chk;
    };
}

/*  Buffer storage for line listing routine.  This allows the listing   */
/*  output routines to do all operations without the main routine       */
/*  having to fool with it.                                             */

FILE *list = NULL;

/*  Listing file open routine.  If a listing file is already open, a    */
/*  warning occurs.  If the listing file doesn't open correctly, a      */
/*  fatal error occurs.  If no listing file is open, all calls to       */
/*  lputs() and lclose() have no effect.                                */

void lopen(char *nam)
{
    if (list)
        warning(TWOLST);
    else if (!(list = fopen(nam,"w")))
        fatal_error(LSTOPEN);
    return;
}

/*  Listing file line output routine.  This routine processes the       */
/*  source line saved by popc() and the output of the line assembler in */
/*  buffer obj into a line of the listing.  If the disk fills up, a     */
/*  fatal error occurs.                                                 */

extern int binary;
extern int octal;
extern int byteline;

void lputs(void)
{
    SCRATCH int i, j, k;
    SCRATCH unsigned *o;
    unsigned char bb=0;
    int didline;

    if (list) {
        i = bytes;
        o = obj;
        do {

            fprintf(list,"%c  ",errcode);
            didline=0;

            if (listhex) {
                for (j = 4; j; --j) {


                    if (((j==4)) || (byteline))  {
                        if (j!=4)
                            fprintf(list, "   ");

                        if (octal)
                            fprintf(list,"%06o  ",address);
                        else
                            fprintf(list,"%04x  ",address);
                    }

                    if (i) {
                        --i;
                        ++address;
                        bb=*o;

                        if (octal)
                            fprintf(list," %03o",*o++);
                        else
                            fprintf(list," %02x",*o++);

                        if (binary){
                            fprintf(list, ":");
                            for (k=0;k<8;k++) {
                                if (bb&0x80)
                                  fprintf(list, "1");
                              else
                                  fprintf(list, "0");
                              bb=bb<<1;
                            }
                        }

                        if (byteline) {
                            if (j==4) {
                             fprintf(list," %s", line);
                             didline=1;
                            }
                            else
                                fprintf(list,"\n");
                        }
                    }
                    else  if (!byteline)
                        fprintf(list,"   ");
                }
            }
            else
                fprintf(list,"%18s","");

            if (!didline)
                fprintf(list,"   %s",line);

            strcpy(line,"\n");
            check_page();
            if (ferror(list))
                fatal_error(DSKFULL);
        } while (listhex && i);
    }
    return;
}

/*  Listing file close routine.  THe symbol table is appended to the    */
/*  listing in alphabetic order by symbol name, and the listing file is */
/*  closed.  If the disk fills up, a fatal error occurs.                */

static int col = 0;

void lclose(void)
{
    if (list) {
        if (sroot) {
            list_sym(sroot);
            if (col)
                fprintf(list,"\n");
        }
        fprintf(list,"\f");
        if (ferror(list) || fclose(list) == EOF)
            fatal_error(DSKFULL);
    }
    return;
}

static void list_sym(SYMBOL *sp)
{
    if (sp) {
        list_sym(sp -> left);
        fprintf(list,"%04x  %-10s",sp -> valu,sp -> sname);
        col = (col + 1) % SYMCOLS;
        if (col != 0)
            fprintf(list,"    ");
        else {
            fprintf(list,"\n");
            if (sp -> right)
                check_page();
        }
        list_sym(sp -> right);
    }
    return;
}

static void check_page(void)
{
    if (pagelen && !--listleft)
        eject = TRUE;
    if (eject) {
        eject = FALSE;
        listleft = pagelen;
        fprintf(list,"\f");
        if (title[0]) {
            fprintf(list,"%s\n\n",title);
             listleft -= 2;
        }
    }
    return;
}

static FILE *raw = NULL;
static unsigned rcnt = 0;
static unsigned raddr = 0;
static unsigned rgap = 0;
static unsigned char rbuf[HEXSIZE];

/* Raw file output routines*/
void ropen(char *nam)
{
    if (raw)
        warning(TWOHEX);
    else if (!(raw = fopen(nam,"wb")))
        fatal_error(RAWOPEN);
    return;
}

void rputc(unsigned c)
{
    if (raw) {
        if (rgap) {
            if (rcnt) {
                fwrite(rbuf, 1, rcnt, raw);
                rcnt = 0;
            }

            memset(rbuf, 0xff, HEXSIZE);

            while (rgap > 0) {
                rcnt = (rgap > HEXSIZE) ? HEXSIZE : rgap;
                fwrite(rbuf, 1, rcnt, raw);
                rgap -= rcnt;
            }

            rcnt = 0;
            rgap = 0;
        }

        rbuf[rcnt++] = c;
        raddr++;

        if (rcnt == HEXSIZE) {
            fwrite(rbuf, 1, HEXSIZE, raw);
            rcnt = 0;
        }
    }
}

void rseek(unsigned a)
{
    if (a < raddr) {
        fatal_error("Binary can't go backward.");
    }

    if (raddr != 0) {
        rgap = a - raddr;
    }

    raddr = a;
}

void rclose(void)
{
    if (raw) {
        if (rcnt)
            fwrite(rbuf, 1, rcnt, raw);

        if (fclose(raw) == EOF)
            fatal_error(DSKFULL);
    }
}

/*  Buffer storage for shared symbol file routine.  This allows the     */
/*  shared symbol file output routines to do all operations without the */
/*  main routine having to fool with it.                                */

FILE* shared = NULL;

/*  Shared symbol file open routine.  If a shared symbol file is        */
/*  already open, a warning occurs.  If the listing file doesn't open   */
/*  correctly, a fatal error occurs.  If no listing file is open, all   */
/*  calls to sclose() have no effect.                                   */

void sopen(char *nam)
{
    if (shared)
        warning(TWOSHR);
    else if (!(shared = fopen(nam, "w")))
        fatal_error(SHROPEN);
    return;
}

/*  Shared symbol file close routine.  An assembler EQU statement is    */
/*  generated for each shared symbol in alphabetic order by symbol name,*/
/*  and the shared symbol file is closed.  If the disk fills up, a fatal*/
/*  error occurs.                                                       */

void sclose(void)
{
    if (shared) {
        if (sroot) {
            write_shared(sroot);
            if (ferror(shared) || fclose(shared) == EOF)
                fatal_error(DSKFULL);
        }
    }
    return;
}

static void write_shared(SYMBOL *sp)
{
    if (sp) {
        write_shared(sp->left);
        if (sp->shared) {
            fprintf(shared, "%-11s EQU     $%04x\n", sp->sname, sp->valu);
        }
        write_shared(sp->right);
    }
    return;
}


/*  Buffer storage for hex output file.  This allows this module to     */
/*  do all of the required buffering and record forming without the     */
/*  main routine having to fool with it.                                */

static FILE *hex = NULL;
static unsigned cnt = 0;
static unsigned addr = 0;
static unsigned sum = 0;
static unsigned buf[HEXSIZE];


/*  Hex file open routine.  If a hex file is already open, a warning    */
/*  occurs.  If the hex file doesn't open correctly, a fatal error      */
/*  occurs.  If no hex file is open, all calls to hputc(), hseek(), and */
/*  hclose() have no effect.                                            */

void hopen(char *nam)
{
    if (hex)
        warning(TWOHEX);
    else if (!(hex = fopen(nam,"w")))
        fatal_error(HEXOPEN);
    return;
}

/*  Hex file write routine.  The data byte is appended to the current   */
/*  record.  If the record fills up, it gets written to disk.  If the   */
/*  disk fills up, a fatal error occurs.                                */

void hputc(unsigned c)
{
    if (hex) {
        buf[cnt++] = c;
        if (cnt == HEXSIZE)
            record(0);
    }
    return;
}

/*  Hex file address set routine.  The specified address becomes the    */
/*  load address of the next record.  If a record is currently open,    */
/*  it gets written to disk.  If the disk fills up, a fatal error       */
/*  occurs.                                                             */

void hseek(unsigned a)
{
    if (hex) {
        if (cnt)
            record(0);
        addr = a;
    }
    return;
}

/*  Hex file close routine.  Any open record is written to disk, the    */
/*  EOF record is added, and file is closed.  If the disk fills up, a   */
/*  fatal error occurs.                                                 */

void hclose(void)
{
    if (hex) {
        if (cnt)
            record(0);
        record(1);
        if (fclose(hex) == EOF)
            fatal_error(DSKFULL);
    }
    return;
}

static void record(unsigned typ)
{
    SCRATCH unsigned i;

    putc(':',hex);
    putb(cnt);
    if (typ == 1) {
        putb(0);
        putb(0);
    }
    else {
        putb(high(addr));
        putb(low(addr));
    }
    putb(typ);
    for (i = 0; i < cnt; ++i)
        putb(buf[i]);
    putb(low(~sum + 1));
    putc('\n',hex);

    addr += cnt;
    cnt = 0;

    if (ferror(hex))
        fatal_error(DSKFULL);
    return;
}

static void putb(unsigned b)
{
    static char digit[] = "0123456789ABCDEF";

    putc(digit[b >> 4],hex);
    putc(digit[b & 0x0f],hex);
    sum += b;
    return;
}

/*  Error handler routine.  If the current error code is non-blank,     */
/*  the error code is filled in and the    number of lines with errors  */
/*  is adjusted.                                                        */

void error(char code)
{
    if (errcode == ' ') {
        errcode = code;
        ++errors; }
    return;
}

/*  Fatal error handler routine.  A message gets printed on the stderr  */
/*  device, and the program bombs.                                      */

void fatal_error(char *msg)
{
    printf("Fatal Error -- %s\n",msg);
    exit(-1);
}

/*  Non-fatal error handler routine.  A message gets printed on the     */
/*  stderr device, and the routine returns.                             */

void warning(char *msg)
{
    printf("Warning -- %s\n",msg);
    return;
}
