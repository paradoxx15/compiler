/*
* COP3402 - Spring 2018
* System Software Assignment 3
* parser.c
* Submitted by: Gaelen Dignan and Ben Faria
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_STACK_HEIGHT 2000
#define MAX_SYMBOL_SIZE 1000
#define MAX_CODE_LENGTH 500
#define MAX_LEXI_LEVELS 3
#define MAX_LEX_NUMBER 5000
#define MAX_STRING_LENGTH 11
#define MAX_NUM_LENGTH 5

typedef struct lex 
{
    char *name;
    int value;
    int token;
}lex;

typedef struct instruction
{
    int op; // Op code
    int r; // Register
    int l;  // L
    int m;  // M
}instruction;

typedef struct counters
{
    int instCount;
    int lex;
    int bp;
    int sp;
    int pc;
}counters;

typedef struct 
{ 
    int kind; // const = 1, var = 2, proc = 3
    char *name;// name up to 11 chars
    int val; // number (ASCII value) 
    int level; // L level
    int addr; // M address
} symbol; 

enum tokens {nulsym = 1, identsym = 2, numbersym = 3, plussym = 4, minussym = 5, multsym = 6,  
             slashsym = 7, oddsym = 8, eqsym = 9, neqsym = 10, lessym = 11, leqsym = 12, gtrsym = 13, 
             geqsym = 14, lparentsym = 15, rparentsym = 16, commasym = 17, semicolonsym = 18,
             periodsym = 19, becomessym = 20, beginsym = 21, endsym = 22, ifsym = 23, thensym = 24, 
             whilesym = 25, dosym = 26, callsym = 27, constsym = 28, varsym = 29, procsym = 30, 
             writesym = 31, readsym = 32, elsesym = 33} token_type;

instruction *code;
lex *lexTable;
symbol *symbolTable;
int iCount;
int sCount;
int lCount;
int error;
int tokenCounter;
int token;
int number;
int vmPrint;
char *tokName;
char *lexNames[] = {"nulsym", "identsym", "numbersym", "plussym", "minussym", "multsym",  
                   "slashsym", "oddsym", "eqsym", "neqsym", "lessym", "leqsym", "gtrsym", 
                   "geqsym", "lparentsym", "rparentsym", "commasym", "semicolonsym",
                   "periodsym", "becomessym", "beginsym", "endsym", "ifsym", "thensym", 
                   "whilesym", "dosym", "callsym", "constsym", "varsym", "procsym", "writesym", "readsym", "elsesym"};
char *opTypes[] = {"", "lit", "rtn", "lod", "sto", "cal", "inc", "jmp", "jpc", "sio", "neg",
                  "add", "sub", "mul", "div", "odd", "mod", "eql", "neq", "lss", "leq",
                  "gtr", "geq"};

void factor(int level, int reg);
void term(int level, int reg);
void expression(int level, int reg);
void condition(int level);
void statement(int level);
void block(int level);
void program();

int base(int l, int base, int *stack)
{
    int b1 = base; //find base L levels down
    while (l > 0)
    {
        b1 = stack[b1 + 1];
        l--;
    }
    return b1;
}

void printStack(int sp, int bp, int* stack, int lex){
     int i;
     if (bp == 1) 
     {
     	if (lex > 0) 
        {
	        printf("|");
	    }
     }	   
     else 
     {
     	//Print the lesser lexical level
     	printStack(bp-1, stack[bp + 2], stack, lex-1);
	    printf("|");
     }
     //Print the stack contents - at the current level
     for (i = bp; i <= sp; i++) 
     {
     	 printf("%3d ", stack[i]);	
     }
}

void execute(int *stack, int *registers, counters *counter)
{
    instruction inst;
    int sioEnd = 0;
    int i = 0;

    if (vmPrint)
    {
        printf("\n-------------------------------------------\n");
        printf("VIRTUAL MACHINE TRACE:\n");
        printf("Initial Values:\n");
        printf("PC  BP  SP  Stack\n");
        printf("0 1 0 0\n\n");
        printf("Stack Trace:\n");
        printf("    R L M PC  BP  SP  Stack\n");
    }

    while (sioEnd == 0)
    {
        inst = code[counter->pc];
        switch(inst.op)
        {
            // lit
            case 1:
                registers[inst.r] = inst.m;
                counter->pc++;
                break;

            // rtn
            case 2:
                counter->sp = counter->bp - 1;
                counter->bp = stack[counter->sp + 3];
                counter->pc = stack[counter->sp + 4];
                counter->lex--;
                break;

            // lod
            case 3:
                registers[inst.r] = stack[base(inst.l, counter->bp, stack) + inst.m];
                counter->pc++;
                break;

            // sto
            case 4:
                stack[base(inst.l, counter->bp, stack) + inst.m] = registers[inst.r];
                counter->pc++;
                break;

            // cal
            case 5:
                stack[counter->sp + 1] = 0;
                stack[counter->sp + 2] = base(inst.l, counter->bp, stack);
                stack[counter->sp + 3] = counter->bp;
                stack[counter->sp + 4] = counter->pc + 1;
                counter->bp = counter->sp + 1;
                counter->pc = inst.m;
                counter->lex++;
                counter->sp += 4;
                break;

            // inc
            case 6:
                counter->sp += inst.m;
                counter->pc++;
                break;

            // jmp
            case 7:
                counter->pc = inst.m;
                break;

            // jpc
            case 8:
                if (registers[inst.r] == 0)
                    counter->pc = inst.m;
                else
                    counter->pc++;
                break;

            // sio
            case 9:
                counter->pc++;
                if (inst.m == 1)
                    printf("OUTPUT: %d\n", registers[inst.r]);
                else if (inst.m == 2)
                    scanf("%d", &registers[inst.r]);
                else if (inst.m == 3)
                    sioEnd = 1;
                break;

            // neg
            case 10:
                registers[inst.r] = -registers[inst.l];
                counter->pc++;
                break;

            // add
            case 11:
                registers[inst.r] = registers[inst.l] + registers[inst.m];
                counter->pc++;
                break;

            // sub
            case 12:
                registers[inst.r] = registers[inst.l] - registers[inst.m];
                counter->pc++;
                break;

            // mul
            case 13:
                registers[inst.r] = registers[inst.l] * registers[inst.m];
                counter->pc++;
                break;

            // div
            case 14:
                registers[inst.r] = registers[inst.l] / registers[inst.m];
                counter->pc++;
                break;

            // odd
            case 15:
                registers[inst.r] = registers[inst.r] % 2;
                counter->pc++;
                break;

            // mod
            case 16:
                registers[inst.r] = registers[inst.l] % registers[inst.m];
                counter->pc++;
                break;

            // eql
            case 17:
                registers[inst.r] = registers[inst.l] == registers[inst.m];
                break;

            // neq
            case 18:
                registers[inst.r] = registers[inst.l] != registers[inst.m];
                counter->pc++;
                break;

            // lss
            case 19:
                registers[inst.r] = registers[inst.l] < registers[inst.m];
                counter->pc++;
                break;

            // leq
            case 20:
                registers[inst.r] = registers[inst.l] <= registers[inst.m];
                counter->pc++;
                break;

            // gtr
            case 21:
                registers[inst.r] = registers[inst.l] > registers[inst.m];
                counter->pc++;
                break;

            // geq
            case 22:
                registers[inst.r] = registers[inst.l] >= registers[inst.m];
                counter->pc++;
                break;   
        }

        if (vmPrint)
        {
            printf("%d %-4s%3d%3d%3d%3d%3d%3d ", i++, opTypes[inst.op], inst.r, inst.l, inst.m, counter->pc, counter->bp, counter->sp);
            printStack(counter->sp, counter->bp, stack, counter->lex);
            printf("\nRF:%3d%3d%3d%3d%3d%3d%3d%3d\n",
                registers[0], registers[1], registers[2], registers[3], registers[4], registers[5], registers[6], registers[7]);
        }
    }
}

// subject to change
void vm()
{
    int i;
    int *stack;
    int *registers;
    counters *counter;

    stack = calloc(MAX_STACK_HEIGHT, sizeof(int));
    registers = calloc(8, sizeof(int));
    counter = calloc(1, sizeof(counters));
    counter->bp = 1;

    // Need to remove file pointer
    execute(stack, registers, counter);
}

int checkReserved(char *word)
{
        if (strcmp(word, "begin") == 0)
            return 21;
        else if (strcmp(word, "end") == 0)
            return 22;
        else if (strcmp(word, "if") == 0)
            return 23;
        else if (strcmp(word, "then") == 0)
            return 24;
        else if (strcmp(word, "while") == 0)
            return 25;
        else if (strcmp(word, "do") == 0)
            return 26;
        else if (strcmp(word, "call") == 0)
            return 27;
        else if (strcmp(word, "const") == 0)
            return 28;
        else if (strcmp(word, "var") == 0)
            return 29;
        else if (strcmp(word, "procedure") == 0)
            return 30;
        else if (strcmp(word, "write") == 0)
            return 31;
        else if (strcmp(word, "read") == 0)
            return 32;
        else if (strcmp(word, "else") == 0)
            return 33;
        else
            return 2;
}

void printLex()
{
    int i;
    printf("\n-------------------------------------------");
    printf("\nLIST OF LEXEMES/TOKENS:\n");
    fprintf(stdout, "\nInternal Representation:\n");
    for(i = 0; i < lCount; i++)
    {
        fprintf(stdout, "%d ", lexTable[i].token);
        // If an identifier, print variable name
        if(lexTable[i].token == 2)
            fprintf(stdout, "%s ", lexTable[i].name);
        // If number, print its ascii number value
        else if(lexTable[i].token == 3)
            fprintf(stdout, "%d ", lexTable[i].value);
    }
    fprintf(stdout, "\n\nSymbolic Representation:\n");
    for(i = 0; i < lCount; i++)
    {
        fprintf(stdout, "%s ", lexNames[lexTable[i].token - 1]);
        // If an identifier, print variable name
        if(lexTable[i].token == 2)
            fprintf(stdout, "%s ", lexTable[i].name);
        // If number, print its ascii number value
        else if(lexTable[i].token == 3)
            fprintf(stdout, "%d ", lexTable[i].value);
    }
    fprintf(stdout, "\n\n\n");
}

// subject to change
void lexical(char *filename)
{
    FILE *fp;
    char c;
    char *lexName;
    int lookedAhead, inComment, lexVal, tempIndex;

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Invalid File Name\n");
    }

    c = fgetc(fp);
    // Traverses entire file
    while (c != EOF)
    {
        // Continue if it is a space, tab, or whitespace
        if (c == ' ' || c == '\t' || c =='\n' || c == '\r')
        {
            c = fgetc(fp);
            continue;
        }

        // Builds either a restricted word or new identification symbol
        if (isalpha(c))
        {
            tempIndex = 0;
            lexName = calloc(MAX_STRING_LENGTH, sizeof(char));
            lexName[tempIndex++] = c;

            c = fgetc(fp);

            // Keeps reading characters of a variable until it is not a digit or alpha
            while (isalpha(c) || isdigit(c))
            {
                if (tempIndex != MAX_STRING_LENGTH)
                {
                    lexName[tempIndex++] = c;
                    c = fgetc(fp);
                }
                else
                {
                    printf("Error: Identifier too long.");
                    error++;
                }
            }
            
            lexTable[lCount].name = lexName;
            lexTable[lCount++].token = checkReserved(lexName);
        }
        // Builds numbersymbol
        else if (isdigit(c))
        {
            lexVal = c - '0';
            tempIndex = 0;
            lexName = calloc(MAX_STRING_LENGTH, sizeof(char));
            lexName[tempIndex++] = c;

            c = fgetc(fp);

            while (isdigit(c))
            {
                if (tempIndex != MAX_NUM_LENGTH
            )
                {
                    lexName[tempIndex++] = c;
                    lexVal = (lexVal * 10) + (c - '0'); 
                    c = fgetc(fp);
                }  
                else 
                {
                    printf("Error: Number too large");
                    error++;
                }       
            }

            if (isalpha(c))
            {
                printf("Error: Invalid Identifier");
                error++;
            }

            lexTable[lCount].name = lexName;
            lexTable[lCount].value = lexVal;
            lexTable[lCount++].token = 3; // numsym token
        }
        // Checks character c with special symbols
        else 
        {
            lookedAhead = 0;
            switch (c)
            {
                case '+':
                    lexTable[lCount].name = "+";
                    lexTable[lCount++].token = 4;
                    break;
                case '-':
                    lexTable[lCount].name = "-";
                    lexTable[lCount++].token = 5;
                    break;
                case '*':
                    lexTable[lCount].name = "*";
                    lexTable[lCount++].token = 6;
                    break;
                case '/':
                    // Used for comments
                    c = fgetc(fp);
                    if (c == '*')
                    {
                        inComment = 1;
                        c = fgetc(fp);
                        while(inComment)
                        {
                            if (c == '*')
                            {
                                c = fgetc(fp);
                                if (c == '/')
                                    inComment = 0;
                            }
                            else
                            {
                                c = fgetc(fp);
                            }
                        }
                    }
                    // Used for slash
                    else
                    {
                        lookedAhead = 1;
                        lexTable[lCount].name = "/";
                        lexTable[lCount++].token = 7;
                    }
                    break;
                case '%':
                    lexTable[lCount].name = "%";
                    lexTable[lCount++].token = 8;
                    break;
                case '(':
                    lexTable[lCount].name = "(";
                    lexTable[lCount++].token = 15;
                    break;
                case ')':
                    lexTable[lCount].name = ")";
                    lexTable[lCount++].token = 16;
                    break;
                case '=':
                    lexTable[lCount].name = "=";
                    lexTable[lCount++].token = 9;
                    break;
                case ',':
                    lexTable[lCount].name = ",";
                    lexTable[lCount++].token = 17;
                    break;
                case '.':
                    lexTable[lCount].name = ".";
                    lexTable[lCount++].token = 19;
                    break;
                case '<':
                    c = fgetc(fp);
                    if (c == '=')
                    {
                        lexTable[lCount].name = "<=";
                        lexTable[lCount++].token = 12;
                    }
                    else if ( c == '>')
                    {
                        lexTable[lCount].name = "<>";
                        lexTable[lCount++].token = 10;
                    }
                    else
                    {
                        lookedAhead = 1;
                        lexTable[lCount].name = "<";
                        lexTable[lCount++].token = 11;
                    }
                    break;
                case '>':
                    c = fgetc(fp);
                    if (c == '=')
                    {
                        lexTable[lCount].name = ">=";
                        lexTable[lCount++].token = 14;
                    }
                    else
                    {
                        lookedAhead = 1;
                        lexTable[lCount].name = ">";
                        lexTable[lCount++].token = 13;
                    }
                    break;
                case ';':
                    lexTable[lCount].name = ";";
                    lexTable[lCount++].token = 18;
                    break;
                case ':':
                    c = fgetc(fp);
                    // encloses break since : is invalid
                    if (c == '=')
                    {
                        lexTable[lCount].name = ":=";
                        lexTable[lCount++].token = 20;
                        break;
                    }
                default:
                    printf("Error: Invalid symbol.");
                    error++;
            }
            if (!lookedAhead)
                c = fgetc(fp);
        }
    }
    fclose(fp);
}

void printCode()
{
    int i;
    printf("\nCode is syntactically correct. Assembly code generated successfully.\n");
    printf("-------------------------------------------\n");
    printf("GENERATED INTERMEDIATE CODE:\n");
    for (i = 0; i < iCount; i++)
    {
        printf("%3d %s %d %d %d\n", i, opTypes[code[i].op], code[i].r, code[i].l, code[i].m);
    }
}

void getToken()
{
    number = lexTable[tokenCounter].value;
    tokName = lexTable[tokenCounter].name;
    token = lexTable[tokenCounter++].token;
}
void addInstruction(int op, int r, int l, int m)
{
        code[iCount].op = op;
        code[iCount].r = r;
        code[iCount].l = l;
        code[iCount].m = m;
        iCount++;
}
void addSymbol(int kind, char *name, int val, int level, int addr)
{
    symbolTable[sCount].kind = kind;
    symbolTable[sCount].name = name;

    if (kind == 1)
        symbolTable[sCount].val= val;
    else
    {
        symbolTable[sCount].level = level;
        symbolTable[sCount].addr = addr;
    }
    sCount++;
}

int getSymbol(char *name, int level)
{
    int levDif, prevLevDif = 0, ret = -1, difCount = 0;

    for (int i = sCount - 1; i >= 0; i--)
    {
        if (strcmp(name, symbolTable[i].name) == 0)
        {
            if (level >= symbolTable[i].level)
            {
                levDif = level - symbolTable[i].level;

                if (levDif < prevLevDif || difCount == 0)
                {
                    ret = i;
                    prevLevDif = levDif;
                    difCount++;
                }
            }
        }
    }
    return ret;
}

void factor(int level, int reg)
{
    int symPos;
    
    if (token == identsym) 
    {
        symPos = getSymbol(tokName, level); // I think this works
        if (symPos == -1) 
        {
            printf("Error 11: Undeclared Identifier\n");
            error++;
        }
        else 
        {
            if (symbolTable[symPos].kind == 1) 
            {
                // LIT
                addInstruction(1, reg, 0, symbolTable[symPos].val);
            }
            else if (symbolTable[symPos].kind == 2) 
            {
                // LOD
                addInstruction(3, reg, level - symbolTable[symPos].level, symbolTable[symPos].addr);
            }
            else 
            {
                printf("Error 21: Expression must not contain a procedure identifier\n");
                error++;
            }
        }
        getToken();
    }
    else if (token == numbersym) 
    {
        if (number > 99999) 
        {
            printf("Error 25: This number is too large\n");
            error++;
            number = 0;
        }
        addInstruction(1, reg, 0, number);

        getToken();
    }
    else if (token == lparentsym) 
    {
        getToken();
        expression(level, reg % 2);

        if (token == rparentsym) 
        {
            getToken();
        }
        else 
        {
            printf("Error 22: Right parenthesis missing\n");
            error++;
        }
    }

}

void term(int level, int reg)
{
    int lastToken;
    factor(level, reg % 2);
    while(token == multsym || token == slashsym) 
    {
        reg++;
        lastToken = token;
        getToken();
        factor(level, reg % 2);

        if(lastToken == multsym) 
        {
            addInstruction(13, 0, 0, 1);
        }
        else 
        {
            addInstruction(14, 0, 0, 1);
        }
    }
}

void expression(int level, int reg)
{
    int lastToken;
    if (token == plussym || token == minussym)
    {
        lastToken = token;
        getToken();
        term(level, reg % 2);

        if (lastToken == minussym)
        {
            addInstruction(10, 0, 0, 0);
        }
    }
    else
    {
        term(level, reg % 2);
    }

    while (token == plussym || token == minussym)
    {
        reg++;
        lastToken = token;
        getToken();
        term(level, reg % 2);

        if (lastToken == plussym)
        {
            addInstruction(11, 0, 0, 1);
        }
        else
        {
            addInstruction(12, 0, 0, 1);
        }
    }
}

void condition(int level)
{
    int instructionType;
    if (token == oddsym)
    {
        getToken();
        expression(level, 0);
        // ODD
        addInstruction(15, 0, 0, 0);
    }
    else
    {
        expression(level, 0);
        if (token != eqsym && token != neqsym && token != leqsym && token != lessym && token != gtrsym && token!= geqsym)
        {
            printf("Error 20: Relational operator expected\n");
            error++;
        }
        else
        {
            instructionType = token;
            getToken();
            expression(level, 0);

            switch(instructionType)
            {
                case eqsym:
                    // EQL
                    addInstruction(17,0,0,1);
                    break; 
                case neqsym:
                    // NEQ
                    addInstruction(18,0,0,1);
                    break; 
                case lessym:
                    // LSS
                    addInstruction(19,0,0,1);
                    break; 
                case leqsym:
                    // LEQ
                    addInstruction(20,0,0,1);
                    break; 
                case gtrsym:
                    // GTR
                    addInstruction(21,0,0,1);
                    break; 
                case geqsym:
                    // GEQ
                    addInstruction(22,0,0,1);
                    break; 
            }
        }
    }
}

void statement(int level)
{
    int symPos, cx, cx2;

    if (token == identsym)
    {
        symPos = getSymbol(tokName, level);
        if (symPos == -1)
        {
            printf("Error 11: Undeclared Identifier\n");
            error++;
        }
        if (symbolTable[symPos].kind != 2)
        {
            printf("Error 12: Assignment to constant or procedure is not allowed\n");
            error++;
        }
        getToken();
        if (token != becomessym)
        {
            printf("Error 13: Assignment operator expected\n");
            error++;
        }
        getToken();
        expression(level, 0);
        // Store
        addInstruction(4, 0, level - symbolTable[symPos].level, symbolTable[symPos].addr);
    }
    else if (token == callsym)
    {
        getToken();
        if (token != identsym)
        {
            printf("Error 14: Call must be followed by an identifier\n");
            error++;
        }
        symPos = getSymbol(tokName, level);
        if (symPos == -1) 
        {
            printf("Error 11: Undeclared Identifier\n");
            error++;
        }


        // adds CAL instruction if symbol is a procedure
        if (symbolTable[symPos].kind == 3)
            addInstruction(5, 0, level - symbolTable[symPos].level, symbolTable[symPos].addr);
        else 
        {
            printf("Error 15: Call of a constant or variable is meaningless\n");
            error++;
        }

        getToken();
    }
    else if (token == beginsym)
    {
        getToken();
        statement(level);
        while (token == semicolonsym)
        {
            getToken();
            statement(level);
        }
        if (token != endsym)
        {
            printf("Error 17: Semicolon or } expected\n");
            error++;
        }
        getToken();
    }
    else if (token == ifsym)
    {
        getToken();
        condition(level);
        if (token != thensym)
        {
            printf("Error 16: then expected\n");
            error++;
        }
        getToken();
        cx = iCount;
        // JPC
        addInstruction(8, 0, 0, 0);
        statement(level);
        code[cx].m = iCount;
    }
    else if (token == whilesym)
    {
        cx = iCount;
        getToken();
        condition(level);
        cx2 = iCount;
        // JPC
        addInstruction(8, 0, 0, 0);
        if (token != dosym)
        {
            printf("Error 18: do expected\n");
            error++;
        }
        getToken();
        statement(level);
        // JMP back to conditional
        addInstruction(7, 0, 0, cx);
        code[cx2].m = iCount;
    }
    else if (token == writesym)
    {
        getToken();
        expression(level, 0);
        // SIO1
        addInstruction(9, 0, 0, 1);
    }
    else if (token == readsym)
    {
        getToken();
        // SIO2
        addInstruction(10, 0, 0, 2);
        symPos = getSymbol(tokName, level);
        if (symPos == -1)
        {
            printf("Error 11: Undeclared identifier");
            error++;
        }
        else if (symbolTable[symPos].kind != 2)
        {
            printf("Error 12: Assignment to constant or procedure is not allowed");
            error++;
        }
        else
            addInstruction(4, 0, level - symbolTable[symPos].level, symbolTable[symPos].addr);
        getToken();
    }
}

void block(int level)
{
    char *name;
    int num, incCount = 4;

    do 
    {   
        if (token == constsym)
        {
            do 
            {
                getToken();
                if (token != identsym)
                {
                    printf("Error 4: const, var, procedure must be followed by indentifier\n");
                    error++;
                }
                name = tokName;
                getToken();
                if (token != eqsym)
                {
                     printf("Error 3: identifier must be followed by =\n");
                     error++;
                }
                getToken();
                if (token != numbersym)
                {
                    printf("Error 2: = must be followed by a number\n");
                    error++;
                }
                num = number;
                getToken();
                addSymbol(1, name, num, 0, 0);
            } while (token == commasym);
            
            if (token != semicolonsym)
            {
                printf("Error 5: Semicolon or comma missing\n");
                error++;
            }
            getToken();
        }

        if (token == varsym)
        {
            do 
            {
                getToken();
                if (token != identsym)
                {
                    printf("Error 4: const, var, procedure must be followed by indentifier\n");
                    error++;
                }
                name = tokName;
                getToken();
                addSymbol(2, name, 0, level, incCount++);
            } while (token == commasym);
        }

        while (token == procsym)
        {
            getToken();
            if (token != identsym)
            {
                printf("Error 4: const, var, procedure must be followed by indentifier\n");
                error++;
            }
            name = tokName;
            getToken();
            if (token != semicolonsym)
            {
                printf("Error 5: Semicolon or comma missing\n");
                error++;
            }
            getToken();
            block(level + 1);
            if (token != semicolonsym)
            {
                printf("Error 5: Semicolon or comma missing\n");
                error++;
            }
            getToken();
            addSymbol(3, name, 0, level, incCount);
        }
    } while (token == constsym || token == varsym || token == procsym);

    // INC
    addInstruction(6, 0, 0, incCount);
    getToken();
    statement(level);
    // RTN
    if(level != 0)
        addInstruction(2, 0, 0 ,0);
    else
        addInstruction(9, 0, 0, 3);
}

void program()
{
    getToken();
    block(0);

    if (token != periodsym)
    {
        printf("Error 9: Period expected\n");
        error++;
    }
}

int main(int argc, char **argv)
{
    int i, lPrint = 0, aPrint = 0;
    code = calloc(MAX_CODE_LENGTH, sizeof(instruction));
    lexTable = calloc(MAX_LEX_NUMBER, sizeof(lex));
    symbolTable = calloc(MAX_SYMBOL_SIZE, sizeof(symbol));
    tokenCounter = 0;
    iCount = 0;
    sCount = 0;
    lCount = 0;
    error = 0;
    vmPrint = 0;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-l") == 0)
            lPrint = 1;
        else if (strcmp(argv[i], "-a") == 0)
            aPrint = 1;
        else if (strcmp(argv[i], "-v") == 0)
            vmPrint = 1;
    }

    lexical(argv[1]);
    if (error == 0 && lPrint)
        printLex();
    program();
    if (error == 0 && aPrint)
        printCode();
    if (error == 0)
    {
        if (!vmPrint)
        {
            printf("\n-------------------------------------------\n");
            printf("PROGRAM INPUT/OUTPUT:\n");
        }
        vm();
        printf("\nFinished execution. Exiting...\n");
    }
    return 0;
}