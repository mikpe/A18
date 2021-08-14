/*
    HEADER:         CUG149;
    TITLE:          1805A Cross-Assembler (Portable);
    FILENAME:       A18EVAL.C;
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

This file contains the main program and line assembly routines for the
assembler.  The main program parses the command line, feeds the source lines to
the line assembly routine, and sends the results to the listing and object file
output routines.  It also coordinates the activities of everything.  The line
assembly routine uses the expression analyzer and the lexical analyzer to
parse the source line convert it into the object bytes that it represents.
*/

/*  Get global goodies:                                                 */

#include "a18.h"

/*  Define global mailboxes for all modules:                            */

char errcode, line[MAXLINE + 1], title[MAXLINE];
int pass = 0;
int eject, filesp, forwd, listhex;
unsigned address, bytes, errors, listleft, obj[MAXOBJ], pagelen, pc;
unsigned char bin[MAXOBJ];

int binary = 0;
int octal = 0;
int byteline = 0;

FILE *filestk[FILES], *source;
TOKEN token;

/*  Mainline routine.  This routine parses the command line, sets up    */
/*  the assembler at the beginning of each pass, feeds the source text  */
/*  to the line assembler, feeds the result to the listing and hex file */
/*  drivers, and cleans everything up at the end of the run.            */

int done, extend, ifsp, off;

PREDEFINED predef[] = {
    { "R0", 0 },
    { "R1", 1 },
    { "R2", 2 },
    { "R3", 3 },
    { "R4", 4 },
    { "R5", 5 },
    { "R6", 6 },
    { "R7", 7 },
    { "R8", 8 },
    { "R9", 9 },
    { "RA", 0xa },
    { "RB", 0xb },
    { "RC", 0xc },
    { "RD", 0xd },
    { "RE", 0xe },
    { "RF", 0xf },
    { "r0", 0 },
    { "r1", 1 },
    { "r2", 2 },
    { "r3", 3 },
    { "r4", 4 },
    { "r5", 5 },
    { "r6", 6 },
    { "r7", 7 },
    { "r8", 8 },
    { "r9", 9 },
    { "rA", 0xa },
    { "rB", 0xb },
    { "rC", 0xc },
    { "rD", 0xd },
    { "rE", 0xe },
    { "rF", 0xf }
};

int npredef = sizeof(predef) / sizeof(PREDEFINED);

void main(argc,argv)
int argc;
char **argv;
{
    SCRATCH unsigned *o;
    SCRATCH int i;
    SCRATCH SYMBOL* l;
    SYMBOL* new_symbol();
    int newline();
    void asm_line();
    void lclose(), lopen(), lputs();
    void hclose(), hopen(), hputc();
    void rclose(), ropen(), rputc();
    void sclose(), sopen();
    void fatal_error(), warning();

    printf("1802/1805A Cross-Assembler (Portable) Ver 3.0\n");
    printf("Copyright (c) 1985 William C. Colley, III\n");
    printf("Copyright (c) 2017 Mark W. Sherman\n\n");

    while (--argc > 0) {
        if (**++argv == '-') {
            switch (toupper(*++*argv)) {
                case 'L':
                    while (* ++*argv) {

                        switch(toupper(**argv)) {
                            case 'O':
                                octal = 1;
                                break;

                            case 'B':
                                binary = 1;
                                break;

                            case '1':
                                byteline = 1;
                                break;

                            default:
                                warning(BADOPT);
                        }
                    }

                    if (!--argc) {
                        warning(NOLST);
                        break;
                    }
                    else
                        ++argv;

                    lopen(*argv);
                    break;

                /*Specify output file (HEX)*/

                case 'O':
                    if (!*++*argv) {
                        if (!--argc) {
                            warning(NOHEX);
                            break;
                        }
                        else
                            ++argv;
                    }
                    hopen(*argv);
                    break;

                /*Specify binary output file */
                case 'B':
                    if (!*++*argv) {
                        if (!--argc) {
                            warning(BADOPT);
                            break;
                        }
                        else
                            ++argv;
                    }
                    ropen(*argv);
                    break;

                /*Specify shared symbol output file */
                case 'S':
                    if (!*++ * argv) {
                        if (!--argc) {
                            warning(BADOPT);
                            break;
                        }
                        else
                            ++argv;
                    }
                    sopen(*argv);
                    break;

                default:
                    warning(BADOPT);
            }
        }
        else if (filestk[0]) {
            warning(TWOASM);
            printf(" file is %s\n", *argv);
        }
        else if (!(filestk[0] = fopen(*argv,"r")))
            fatal_error(ASMOPEN);
    }

    if (!filestk[0])
        fatal_error(NOASM);

    for (i = 0; i < npredef; i++) {
        l = new_symbol(predef[i].name);
        l->attr = FORWD + VAL;
        l->valu = predef[i].valu;
    }
    
    while (++pass < 3) {
        source = filestk[0];
        if (source != NULL)
            fseek(source,0L,0);
        done = extend = off = FALSE;
        errors = filesp = ifsp = pagelen = pc = 0;  title[0] = '\0';
        while (!done) {
            errcode = ' ';
            if (newline()) {
                error('*');
                strcpy(line,"\tEND\n");
                done = eject = TRUE;  listhex = FALSE;
                bytes = 0;
            }
            else
                asm_line();
            pc = word(pc + bytes);
            if (pass == 2) {
                lputs();
                for (o = obj; bytes--;  hputc(*o++))
                    rputc(*o);
            }
        }
    }

    if (filestk[0] != NULL)
        fclose(filestk[0]);
    lclose();
    hclose();
    rclose();
    sclose();

    if (errors)
        printf("%d Error(s)\n",errors);
    else
        printf("No Errors\n");

    exit(errors);
}

/*  Line assembly routine.  This routine gets expressions and tokens    */
/*  from the source file using the expression evaluator and lexical     */
/*  analyzer, respectively.  It fills a buffer with the machine code    */
/*  bytes and returns nothing.                                          */

char label[MAXLINE];
int ifstack[IFDEPTH] = { ON };

OPCODE *opcod;

void asm_line()
{
    SCRATCH int i;
    SCRATCH size_t len;
    int isalph(), popc();
    OPCODE *find_code(), *find_operator();
    void do_label(), flush(), normal_op(), pseudo_op();
    void pops(), pushc(), trash();

    address = pc;
    bytes = 0;
    eject = forwd = listhex = FALSE;

    for (i = 0; i < BIGINST; obj[i++] = NOP);

    label[0] = '\0';
    if ((i = popc()) != ' ' && i != '\n') {
        if (isalph(i)) {
            pushc(i);
            pops(label);
            // Strip off optional ':'
            len = strlen(label);
            if (label[len - 1] == ':') {
                label[len - 1] = '\0';
            }
            if (find_operator(label)) {
                label[0] = '\0';
                error('L');
            }
        }
        else {
            error('L');
            while ((i = popc()) != ' ' && i != '\n');
        }
    }

    trash();
    opcod = NULL;
    if ((i = popc()) != '\n') {
        if (!isalph(i))
            error('S');
        else {
            pushc(i);
            pops(token.sval);
            if (!(opcod = find_code(token.sval)))
                error('O');
        }
        if (!opcod) {
            listhex = TRUE;
            bytes = BIGINST;
        }
    }

    if (opcod && opcod -> attr & ISIF) {
        if (label[0])
            error('L');
    }
    else if (off) {
        listhex = FALSE;
        flush();
        return;
    }

    if (!opcod) {
        do_label();
        flush();
    }
    else {
        listhex = TRUE;
        if (opcod -> attr & PSEUDO)
            pseudo_op();
        else
            normal_op();
        while ((i = popc()) != '\n')
            if (i != ' ')
                error('T');
    }
    source = filestk[filesp];
    return;
}

void flush()
{
    while (popc() != '\n');
}

void do_label()
{
    SCRATCH SYMBOL *l;
    SCRATCH size_t len;
    SYMBOL *find_symbol(), *new_symbol();
//    void error();

    if (label[0]) {
        listhex = TRUE;
        // Strip off optional ':'
        len = strlen(label);
        if (label[len - 1] == ':') {
            label[len - 1] = '\0';
        }
        if (pass == 1) {
            if (!((l = new_symbol(label)) -> attr)) {
                l -> attr = FORWD + VAL;
                l -> valu = pc;
            }
        }
        else {
            if (l = find_symbol(label, TRUE)) {
                l -> attr = VAL;
                if (l -> valu != pc)
                    error('M');
            }
            else
                error('P');
        }
    }
}

void normal_op()
{
    SCRATCH unsigned attrib, *objp, operand;
    unsigned expr();
    TOKEN *lex();
    void do_label(), unlex();

    do_label();
    bytes = (attrib = opcod -> attr) & BYTES;
    if (pass == 1)
        return;

    objp = obj;
    if (attrib & IS1805) {
        *objp++ = PREBYTE;
        if (!extend)
            error('O');
    }
    *objp++ = opcod -> valu;
    objp[0] = objp[1] = 0;

    while (attrib & (REGTYP + NUMTYP)) {
        operand = expr();
        switch (attrib & REGTYP) {
            case IOPORT:
                if (operand > 7) {
                    error('R');
                    return;
                }

            case NOT_R0:
                if (!operand) {
                    error('R');
                    return;
                }

            case ANY:
                if (operand > 15) {
                    error('R');
                    return;
                }
                *(objp - 1) += operand;
                attrib &= ~(REGTYP);
                break;

            case 0:
                switch (attrib & NUMTYP) {
                    case SIXTN:
                        *objp++ = high(operand);
                        *objp = low(operand);
                        break;

                    case BRANCH:
                        if (high(operand) != high(pc + bytes - 1)) {
                            error('B');
                            return;
                        }
                        *objp = low(operand);
                        break;

                    case IMMED:
                        if (operand > 0xff &&
                            operand < 0xff80) {
                            error('V');  return;
                        }
                        *objp = low(operand);
                }
                attrib &= ~NUMTYP;
                break;
        }
    }
}

unsigned *do_string(s, o)
char* s;
unsigned *o;
{
    SCRATCH int esc;
    SCRATCH char t;
    SCRATCH char *n, *e;
    SCRATCH unsigned i;
    int isoct(char c);

    esc = FALSE;
    while (*s) {
        if (*s == '\\' && !esc) {
            esc = TRUE;
        }
        else if (esc) {
            if (isoct(*s)) {
                // Octal escape sequence.
                // Remember the start.
                n = s;
                // Use up to 3 octal characters.
                i = 1;
                do {
                    ++s;
                    ++i;
                } while (*s && isoct(*s) && i <= 3);
                // Remember the current content and
                // terminate the digits.
                t = *s;
                *s = '\0';
                // Get the octal value.
                *o++ = strtoul(n, &e, 8);
                // Restore the following character.
                *s = t;
                s = e - 1;
            }
            else {
                switch (*s) {
                case 'a':  *o++ = 0x07; break;
                case 'b':  *o++ = 0x08; break;
                case 'e':  *o++ = 0x1b; break;
                case 'f':  *o++ = 0x0c; break;
                case 'n':  *o++ = 0x0a; break;
                case 'r':  *o++ = 0x0d; break;
                case 't':  *o++ = 0x09; break;
                case 'v':  *o++ = 0x0b; break;
                case '\\': *o++ = 0x5c; break;
                case '\'': *o++ = 0x27; break;
                case '"':  *o++ = 0x22; break;
                case '?':  *o++ = 0x3f; break;
                case 'x':
                    *o++ = strtoul(++s, &e, 16);
                    s = e - 1;
                    break;
                default:   *o++ = *s;   break;
                }
            }
            bytes++;
            esc = FALSE;
        }
        else {
            *o++ = *s;
            bytes++;
        }
        s++;
    }

    return o;
}

void pseudo_op()
{
    SCRATCH unsigned *o, u, v;
    SCRATCH SYMBOL *l;
    SCRATCH unsigned i;
    SCRATCH FILE* f;
    unsigned expr();
    SYMBOL *find_symbol(), *new_symbol();
    TOKEN *lex();
    void do_label(), fatal_error(), hseek(), rseek(), unlex();

    o = obj;
    switch (opcod -> valu) {
        case OP_ALIGN:
            u = expr();
            address = pc = (pc + (u-1)) & ~(u-1);
            if (pass == 2) {
                hseek(pc);
                rseek(pc);
            }
            do_label();
            break;

        case OP_BINCL:
            do_label();
            if ((lex()->attr & TYPE) == STR) {
                if (!(f = fopen(token.sval, "rb"))) {
                    error('V');
                }
                else {
                    fseek(f, 0, SEEK_END);
                    u = ftell(f);
                    if (u > MAXOBJ) {
                        error('V');
                    }
                    else {
                        if (pass == 2) {
                            fseek(f, 0, SEEK_SET);
                            fread(bin, 1, u, f);
                            for (i = 0; i < u; i++) {
                                *o++ = bin[i];
                            }
                        }
                        bytes = u;
                    }
                    fclose(f);
                }
            }
            else
                error('S');
            break;

        case OP_BLK:
        case OP_DS:
            do_label();
            u = word(pc + expr());
            if (forwd)
                error('P');
            else {
                pc = u;
                if (pass == 2) {
                    hseek(pc);
                    rseek(pc);
                }
            }
            break;

        case OP_BYTE:
        case OP_DB:
            do_label();
            do {
                if ((lex()->attr & TYPE) == STR) {
                    o = do_string(token.sval, o);
                    lex();
                }
                else {
                    if ((token.attr & TYPE) == SEP)
                        u = 0;
                    else {
                        unlex();
                        if ((u = expr()) > 0xff && u < 0xff80) {
                            u = 0;  error('V');
                        }
                    }
                    *o++ = low(u);
                    ++bytes;
                }
            } while ((token.attr & TYPE) == SEP);
            break;

        case OP_CALL:
            do_label();
            bytes = 3;
            obj[0] = 0xd4;
            u = expr();
            obj[1] = high(u);
            obj[2] = low(u);
            break;

        case OP_RETN:
            do_label();
            bytes = 1;
            obj[0] = 0xd5;
            break;

        case OP_CPU:
            listhex = FALSE;
            do_label();
            u = expr();
            if (forwd)
                error('P');
            else if (u != 1802 && u != 1805)
                error('V');
            else
                extend = u == 1805;
            break;

        case OP_EJCT:
            listhex = FALSE;
            do_label();
            if ((lex() -> attr & TYPE) != EOL) {
                unlex();
                pagelen = expr();
                if (pagelen > 0 && pagelen < 3) {
                    pagelen = 0;  error('V');
                }
            }
            eject = TRUE;
            break;

        case OP_ELSE:
            listhex = FALSE;
            if (ifsp)
                off = (ifstack[ifsp] = -ifstack[ifsp]) != ON;
            else
                error('I');
            break;

        case OP_END:
            do_label();
            if (filesp) {
                listhex = FALSE;
                error('*');
            }
            else {
                done = eject = TRUE;
                if (pass == 2 && (lex() -> attr & TYPE) != EOL) {
                    error('T');
                }
                if (ifsp)
                    error('I');
            }
            break;

        case OP_ENDI:
            listhex = FALSE;
            if (ifsp)
                off = ifstack[--ifsp] != ON;
            else
                error('I');
            break;

        case OP_EQU:
            if (label[0]) {
                if (pass == 1) {
                    if (!((l = new_symbol(label)) -> attr)) {
                        l -> attr = FORWD + VAL;
                        address = expr();
                        if (!forwd)
                            l -> valu = address;
                    }
                }
                else {
                    if (l = find_symbol(label, FALSE)) {
                        l -> attr = VAL;
                        address = expr();
                        if (forwd)
                            error('P');
                        if (l -> valu != address)
                            error('M');
                    }
                    else
                        error('P');
                }
            }
            else
                error('L');
            break;

        case OP_FILL:
            do_label();
            u = expr();
            if (u > 0xFF) {
                fatal_error(RANGE);
                break;
            }
            bytes = expr();
            if (bytes > MAXOBJ) {
                fatal_error(RANGE);
                break;
            }
            for (i = 0; i < bytes; i++)
                *o++ = u;
            break;

        case OP_IF:
            if (++ifsp >= IFDEPTH) {
                fatal_error(IFOFLOW);
                break;
            }
            address = expr();
            if (forwd) {
                error('P');
                address = TRUE;
            }
            if (off) {
                listhex = FALSE;
                ifstack[ifsp] = 0;
            }
            else {
                ifstack[ifsp] = address ? ON : OFF;
                if (!address)
                    off = TRUE;
            }
            break;

        case OP_LOAD:
        case OP_MOV:
            do_label();
            bytes = 6;
            obj[0] = obj[3] = LDI;
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            obj[2] = PHI + u;
            obj[5] = PLO + u;
            u = expr();
            obj[1] = high(u);
            obj[4] = low(u);
            break;

        case OP_INCL:
            listhex = FALSE;
            do_label();
            if ((lex() -> attr & TYPE) == STR) {
                if (++filesp == FILES) {
                    fatal_error(FLOFLOW);
                    break;
                }
                if (!(filestk[filesp] = fopen(token.sval,"r"))) {
                    --filesp;
                    error('V');
                }
            }
            else
                error('S');
            break;

        case OP_ORG:
            u = expr();
            if (forwd)
                error('P');
            else {
                pc = address = u;
                if (pass == 2) {
                    hseek(pc);
                    rseek(pc);
                }
            }
            do_label();
            break;

        case OP_PAGE:
            address = pc = (pc + 255) & 0xff00;
            if (pass == 2) {
                hseek(pc);
                rseek(pc);
            }
            do_label();
            break;

        case OP_SET:
            if (label[0]) {
                if (pass == 1) {
                    if (!((l = new_symbol(label)) -> attr) ||
                        (l -> attr & SOFT)) {
                        l -> attr = FORWD + SOFT + VAL;
                        address = expr();
                        if (!forwd)
                            l -> valu = address;
                    }
                }
                else {
                    if (l = find_symbol(label, FALSE)) {
                        address = expr();
                        if (forwd)
                            error('P');
                        else if (l -> attr & SOFT) {
                            l -> attr = SOFT + VAL;
                            l -> valu = address;
                        }
                        else
                            error('M');
                    }
                    else
                        error('P');
                }
            }
            else
                error('L');
            break;

        case OP_SHARED:
            if (pass == 2) {
                while ((lex()->attr & TYPE) != EOL) {
                    if ((token.attr & TYPE) == VAL) {
                        if (l = find_symbol(token.sval, FALSE)) {
                            l->shared = TRUE;
                        }
                        else
                            error('U');
                    }
                    else if ((token.attr & TYPE) != SEP)
                        error('V');
                }
            }
            break;

        case OP_TEXT:
        case OP_TEXTZ:
            do_label();
            while ((lex() -> attr & TYPE) != EOL) {
                if ((token.attr & TYPE) == STR) {
                    o = do_string(token.sval, o);
                    if ((lex() -> attr & TYPE) != SEP)
                        unlex();
                    if (opcod->valu == OP_TEXTZ) {
                        *o++ = '\0';
                        bytes++;
                    }
                }
                else
                    error('S');
            }
            break;

        case OP_TITL:
            listhex = FALSE;
            do_label();
            if ((lex() -> attr & TYPE) == EOL)
                title[0] = '\0';
            else if ((token.attr & TYPE) != STR)
                error('S');
            else
                strcpy(title,token.sval);
            break;

        case OP_DW:
        case OP_WORD:
            do_label();
            do {
                if ((lex() -> attr & TYPE) == SEP)
                    u = 0;
                else {
                    unlex();
                    u = expr();
                }
                *o++ = high(u);
                *o++ = low(u);
                bytes += 2;
            } while ((token.attr & TYPE) == SEP);
            break;

        case OP_RADC:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            if ((v = expr()) > 15) {
                v = 0;
                error('R');
            }
            bytes = 12;
            obj[0] = GLO + v;
            obj[1] = STXD;
            obj[2] = IRX;
            obj[3] = GLO + u;
            obj[4] = 0x74;
            obj[5] = PLO + u;
            obj[6] = GHI + v;
            obj[7] = STXD;
            obj[8] = IRX;
            obj[9] = GHI + u;
            obj[10] = 0x74;
            obj[11] = PHI + u;
            break;

        case OP_RADCI:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            v = expr();
            bytes = 8;
            obj[0] = GLO + u;
            obj[1] = 0x7c;
            obj[2] = low(v);
            obj[3] = PLO + u;
            obj[4] = GHI + u;
            obj[5] = 0x7c;
            obj[6] = high(v);
            obj[7] = PHI + u;
            break;

        case OP_RADD:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            if ((v = expr()) > 15) {
                v = 0;
                error('R');
            }
            bytes = 12;
            obj[0] = GLO + v;
            obj[1] = STXD;
            obj[2] = IRX;
            obj[3] = GLO + u;
            obj[4] = 0xf4;
            obj[5] = PLO + u;
            obj[6] = GHI + v;
            obj[7] = STXD;
            obj[8] = IRX;
            obj[9] = GHI + u;
            obj[10] = 0x74;
            obj[11] = PHI + u;
            break;

        case OP_RADI:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            v = expr();
            bytes = 8;
            obj[0] = GLO + u;
            obj[1] = 0xfc;
            obj[2] = low(v);
            obj[3] = PLO + u;
            obj[4] = GHI + u;
            obj[5] = 0x7c;
            obj[6] = high(v);
            obj[7] = PHI + u;
            break;

        case OP_RAND:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            if ((v = expr()) > 15) {
                v = 0;
                error('R');
            }
            bytes = 12;
            obj[0] = GLO + v;
            obj[1] = STXD;
            obj[2] = IRX;
            obj[3] = GLO + u;
            obj[4] = 0xf2;
            obj[5] = PLO + u;
            obj[6] = GHI + v;
            obj[7] = STXD;
            obj[8] = IRX;
            obj[9] = GHI + u;
            obj[10] = 0xf2;
            obj[11] = PHI + u;
            break;

        case OP_RANI:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            v = expr();
            bytes = 8;
            obj[0] = GLO + u;
            obj[1] = 0xfa;
            obj[2] = low(v);
            obj[3] = PLO + u;
            obj[4] = GHI + u;
            obj[5] = 0xfa;
            obj[6] = high(v);
            obj[7] = PHI + u;
            break;

        case OP_BRNZ:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            v = expr();
            bytes = 6;
            obj[0] = GLO + u;
            obj[1] = 0x3a;
            obj[2] = low(v);
            obj[3] = GHI + u;
            obj[4] = 0x3a;
            obj[5] = low(v);
            break;

        case OP_BRZ:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            v = expr();
            bytes = 7;
            obj[0] = GLO + u;
            obj[1] = STXD;
            obj[2] = IRX;
            obj[3] = GHI + u;
            obj[4] = 0xf1;
            obj[5] = 0x32;
            obj[6] = low(v);
            break;

        case OP_DBNZ:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            v = expr();
            if (extend) {
                *o++ = PREBYTE;
                *o++ = 0x20 + u;
                *o++ = high(v);
                *o++ = low(v);
                bytes = 4;
            }
            else {
                bytes = 7;
                obj[0] = DEC + u;
                obj[1] = GLO + u;
                obj[2] = 0x3a;
                obj[3] = low(v);
                obj[4] = GHI + u;
                obj[5] = 0x3a;
                obj[6] = low(v);
            }
            break;

        case OP_DLBNZ:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            v = expr();
            bytes = 9;
            obj[0] = DEC + u;
            obj[1] = GLO + u;
            obj[2] = 0xca;
            obj[3] = high(v);
            obj[4] = low(v);
            obj[5] = GHI + u;
            obj[6] = 0xca;
            obj[7] = high(v);
            obj[8] = low(v);
            break;

        case OP_LBRNZ:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            v = expr();
            bytes = 8;
            obj[0] = GLO + u;
            obj[1] = 0xca;
            obj[2] = high(v);
            obj[3] = low(v);
            obj[4] = GHI + u;
            obj[5] = 0xca;
            obj[6] = high(v);
            obj[7] = low(v);
            break;

        case OP_LBRZ:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            v = expr();
            bytes = 8;
            obj[0] = GLO + u;
            obj[1] = STXD;
            obj[2] = IRX;
            obj[3] = GHI + u;
            obj[4] = 0xf1;
            obj[5] = 0xc2;
            obj[6] = high(v);
            obj[7] = low(v);
            break;

        case OP_RLD:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            if ((v = expr()) > 15) {
                v = 0;
                error('R');
            }
            bytes = 4;
            obj[0] = GLO + v;
            obj[1] = PLO + u;
            obj[2] = GHI + v;
            obj[3] = PHI + u;
            break;

        case OP_RLDI:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            v = expr();
            if (extend) {
                *o++ = PREBYTE;
                *o++ = 0xc0 + u;
                *o++ = high(v);
                *o++ = low(v);
                bytes = 4;
            }
            else {
                bytes = 6;
                obj[0] = obj[3] = LDI;
                obj[1] = high(v);
                obj[2] = PHI + u;
                obj[3] = LDI;
                obj[4] = low(v);
                obj[5] = PLO + u;
            }
            break;

        case OP_ROR:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            if ((v = expr()) > 15) {
                v = 0;
                error('R');
            }
            bytes = 12;
            obj[0] = GLO + v;
            obj[1] = STXD;
            obj[2] = IRX;
            obj[3] = GLO + u;
            obj[4] = 0xf1;
            obj[5] = PLO + u;
            obj[6] = GHI + v;
            obj[7] = STXD;
            obj[8] = IRX;
            obj[9] = GHI + u;
            obj[10] = 0xf1;
            obj[11] = PHI + u;
            break;

        case OP_RORI:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            v = expr();
            bytes = 8;
            obj[0] = GLO + u;
            obj[1] = 0xf9;
            obj[2] = low(v);
            obj[3] = PLO + u;
            obj[4] = GHI + u;
            obj[5] = 0xf9;
            obj[6] = high(v);
            obj[7] = PHI + u;
            break;

        case OP_POP:
            do_label();
            bytes = 5;
            obj[0] = IRX;
            obj[1] = LDXA;
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            obj[2] = PHI + u;
            obj[3] = LDX;
            obj[4] = PLO + u;
            break;

        case OP_PUSH:
            do_label();
            bytes = 4;
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            obj[0] = GLO + u;
            obj[1] = STXD;
            obj[2] = GHI + u;
            obj[3] = STXD;
            break;

        case OP_RSBB:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            if ((v = expr()) > 15) {
                v = 0;
                error('R');
            }
            bytes = 12;
            obj[0] = GLO + v;
            obj[1] = STXD;
            obj[2] = IRX;
            obj[3] = GLO + u;
            obj[4] = 0x77;
            obj[5] = PLO + u;
            obj[6] = GHI + v;
            obj[7] = STXD;
            obj[8] = IRX;
            obj[9] = GHI + u;
            obj[10] = 0x77;
            obj[11] = PHI + u;
            break;

        case OP_RSBBI:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            v = expr();
            bytes = 8;
            obj[0] = GLO + u;
            obj[1] = 0x7f;
            obj[2] = low(v);
            obj[3] = PLO + u;
            obj[4] = GHI + u;
            obj[5] = 0x7f;
            obj[6] = high(v);
            obj[7] = PHI + u;
            break;

        case OP_RSHL:
            do_label();
            if ((lex()->attr & TYPE) == EOL) {
                *o = 0x7e;
                bytes = 1;
            }
            else {
                unlex();
                if ((u = expr()) > 15) {
                    u = 0;
                    error('R');
                }
                bytes = 6;
                obj[0] = GLO + u;
                obj[1] = 0xfe;
                obj[2] = PLO + u;
                obj[3] = GHI + u;
                obj[4] = 0x7e;
                obj[5] = PHI + u;
            }
            break;

        case OP_RSHLC:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            bytes = 6;
            obj[0] = GLO + u;
            obj[1] = 0x7e;
            obj[2] = PLO + u;
            obj[3] = GHI + u;
            obj[4] = 0x7e;
            obj[5] = PHI + u;
            break;

        case OP_RSHR:
            do_label();
            if ((lex()->attr & TYPE) == EOL) {
                *o = 0x76;
                bytes = 1;
            }
            else {
                unlex();
                if ((u = expr()) > 15) {
                    u = 0;
                    error('R');
                }
                bytes = 6;
                obj[0] = GHI + u;
                obj[1] = 0xf6;
                obj[2] = PHI + u;
                obj[3] = GLO + u;
                obj[4] = 0x76;
                obj[5] = PLO + u;
            }
            break;

        case OP_RSHRC:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            bytes = 6;
            obj[0] = GHI + u;
            obj[1] = 0x76;
            obj[2] = PHI + u;
            obj[3] = GLO + u;
            obj[4] = 0x76;
            obj[5] = PLO + u;
            break;

        case OP_RSUB:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            if ((v = expr()) > 15) {
                v = 0;
                error('R');
            }
            bytes = 12;
            obj[0] = GLO + v;
            obj[1] = STXD;
            obj[2] = IRX;
            obj[3] = GLO + u;
            obj[4] = 0xf7;
            obj[5] = PLO + u;
            obj[6] = GHI + v;
            obj[7] = STXD;
            obj[8] = IRX;
            obj[9] = GHI + u;
            obj[10] = 0x77;
            obj[11] = PHI + u;
            break;

        case OP_RSUBI:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            v = expr();
            bytes = 8;
            obj[0] = GLO + u;
            obj[1] = 0xff;
            obj[2] = low(v);
            obj[3] = PLO + u;
            obj[4] = GHI + u;
            obj[5] = 0x7f;
            obj[6] = high(v);
            obj[7] = PHI + u;
            break;

        case OP_RXOR:
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            if ((v = expr()) > 15) {
                v = 0;
                error('R');
            }
            bytes = 12;
            obj[0] = GLO + v;
            obj[1] = STXD;
            obj[2] = IRX;
            obj[3] = GLO + u;
            obj[4] = 0xf3;
            obj[5] = PLO + u;
            obj[6] = GHI + v;
            obj[7] = STXD;
            obj[8] = IRX;
            obj[9] = GHI + u;
            obj[10] = 0xf3;
            obj[11] = PHI + u;
            break;

        case OP_RXRI:
            do_label();
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            v = expr();
            bytes = 8;
            obj[0] = GLO + u;
            obj[1] = 0xfb;
            obj[2] = low(v);
            obj[3] = PLO + u;
            obj[4] = GHI + u;
            obj[5] = 0xfb;
            obj[6] = high(v);
            obj[7] = PHI + u;
            break;

        case OP_RCLR:
            do_label();
            bytes = 4;
            obj[0] = LDI;
            obj[1] = 0x00;
            if ((u = expr()) > 15) {
                u = 0;
                error('R');
            }
            obj[2] = PHI + u;
            obj[3] = PLO + u;
            break;
    }
    return;
}

int isoct(char c)
{
    return c >= '0' && c <= '7';
}

