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
} lex;

typedef struct instruction
{
    int op; // Op code
    int r; // Register
    int l;  // L
    int m;  // M
    char *type; // Name of instruction
} instruction;

typedef struct counters
{
    int instCount;
    int lex;
    int bp;
    int sp;
    int pc;
} counters;

typedef struct 
{ 
    int kind; // const = 1, var = 2, proc = 3
    char name[12];// name up to 11 chars
    int val; // number (ASCII value) 
    int level; // L level
    int addr; // M address
} symbol; 

instruction *code;
lex *lexTable;
symbol *symbolTable;
int tokenCounter;
char **lexNames = {nulsym, identsym, numbersym, plussym, minussym,multsym,  
                   slashsym, oddsym, eqsym, neqsym, lessym, leqsym,gtrsym, 
                   geqsym, lparentsym, rparentsym, commasym, semicolonsym,
                   periodsym, becomessym, beginsym, endsym, ifsym, thensym, 
                   whilesym, dosym, callsym, constsym, varsym, procsym, writesym, readsym , elsesym};

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
    bool sioEnd = false;

    printf("\n OP   Rg Lx Vl[ PC BP SP]\n");
    while (!sioEnd)
    {
        inst = code[counter->pc];
        switch(inst.op)
        {
            // lit
            case 1:
                inst.type = "LIT";
                registers[inst.r] = inst.m;
                counter->pc++;
                break;

            // rtn
            case 2:
                inst.type = "RTN";
                counter->sp = counter->bp - 1;
                counter->bp = stack[counter->sp + 3];
                counter->pc = stack[counter->sp + 4];
                counter->lex--;
                break;

            // lod
            case 3:
                inst.type = "LOD";
                registers[inst.r] = stack[base(inst.l, counter->bp, stack) + inst.m];
                counter->pc++;
                break;

            // sto
            case 4:
                inst.type = "STO";
                stack[base(inst.l, counter->bp, stack) + inst.m] = registers[inst.r];
                counter->pc++;
                break;

            // cal
            case 5:
                inst.type = "CAL";
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
                inst.type = "INC";
                counter->sp += inst.m;
                counter->pc++;
                break;

            // jmp
            case 7:
                inst.type = "JMP";
                counter->pc = inst.m;
                break;

            // jpc
            case 8:
                inst.type = "JPC";
                if (registers[inst.r] == 0)
                    counter->pc = inst.m;
                else
                    counter->pc++;
                break;

            // sio
            case 9:
                inst.type = "SIO";
                counter->pc++;
                if (inst.m == 1)
                    printf("%d\n", registers[inst.r]);
                else if (inst.m == 2)
                    scanf("%d", &registers[inst.r]);
                else if (inst.m == 3)
                    sioEnd = true;
                break;

            // neg
            case 10:
                inst.type = "NEG";
                registers[inst.r] = -registers[inst.l];
                counter->pc++;
                break;

            // add
            case 11:
                inst.type = "ADD";
                registers[inst.r] = registers[inst.l] + registers[inst.m];
                counter->pc++;
                break;

            // sub
            case 12:
                inst.type = "SUB";
                registers[inst.r] = registers[inst.l] - registers[inst.m];
                counter->pc++;
                break;

            // mul
            case 13:
                inst.type = "MUL";
                registers[inst.r] = registers[inst.l] * registers[inst.m];
                counter->pc++;
                break;

            // div
            case 14:
                inst.type = "DIV";
                registers[inst.r] = registers[inst.l] / registers[inst.m];
                counter->pc++;
                break;

            // odd
            case 15:
                inst.type = "ODD";
                registers[inst.r] = registers[inst.r] % 2;
                counter->pc++;
                break;

            // mod
            case 16:
                inst.type = "MOD";
                registers[inst.r] = registers[inst.l] % registers[inst.m];
                counter->pc++;
                break;

            // eql
            case 17:
                inst.type = "EQL";
                registers[inst.r] = registers[inst.l] == registers[inst.m];
                break;

            // neq
            case 18:
                inst.type = "NEQ";
                registers[inst.r] = registers[inst.l] != registers[inst.m];
                counter->pc++;
                break;

            // lss
            case 19:
                inst.type = "LSS";
                registers[inst.r] = registers[inst.l] < registers[inst.m];
                counter->pc++;
                break;

            // leq
            case 20:
                inst.type = "LEQ";
                registers[inst.r] = registers[inst.l] <= registers[inst.m];
                counter->pc++;
                break;

            // gtr
            case 21:
                inst.type = "GTR";
                registers[inst.r] = registers[inst.l] > registers[inst.m];
                counter->pc++;
                break;

            // geq
            case 22:
                inst.type = "GEQ";
                registers[inst.r] = registers[inst.l] >= registers[inst.m];
                counter->pc++;
                break;   
        }
        printf("%-4s%3d%3d%3d[%3d%3d%3d] ", inst.type, inst.r, inst.l, inst.m, counter->pc, counter->bp, counter->sp);
        printStack(counter->sp, counter->bp, stack, counter->lex);
        printf("\n\tRegisters:[%3d%3d%3d%3d%3d%3d%3d%3d]\n",
                registers[0], registers[1], registers[2], registers[3], registers[4], registers[5], registers[6], registers[7]);
    }
}

void fetch(char *filename, counters *counter)
{
    FILE *fp;
    char *line = NULL;
	size_t len = 0;
	ssize_t read;
    int i;
    char *c;
    instruction inst;
 
	fp = fopen(filename, "r");
	if (fp == NULL)
		return;

	while ((read = getline(&line, &len, fp)) != -1) {
        c = strtok(line, " ");
        inst.op = atoi(c);
        c = strtok(NULL, " ");
        inst.r = atoi(c);
        c = strtok(NULL, " ");
        inst.l = atoi(c);
        c = strtok(NULL, " ");
        inst.m = atoi(c);

        code[counter->instCount++] = inst;
	}
    fclose(fp);
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

    fetch(argv[1], counter);
    execute(stack, registers, counter);
}

void readAndPrintFile(char *filename)
{
    FILE *fp;
    char *line = NULL;
	size_t len = 0;
	ssize_t read;
 
	fp = fopen(filename, "r");
	if (fp == NULL)
		return;

    fprintf(stdout, "Source Program:%s\n", filename);
	while ((read = getline(&line, &len, fp)) != -1) {
        fprintf(stdout, "%s", line);
	}
    fclose(fp);
    fprintf(stdout, "\n");
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

void printLexTable(lex *lexTable, int lexTableIndex)
{
    int i;

    fprintf(stdout, "\nLexeme Table:\n");
    fprintf(stdout, "lexeme\t\ttoken type\n");
    for(i = 0; i < lexTableIndex; i++)
        fprintf(stdout, "%s\t\t%d\n", lexTable[i].name, lexTable[i].token);


    fprintf(stdout, "\nLexeme List:\n");
    for(i = 0; i < lexTableIndex; i++)
    {
        fprintf(stdout, "%d ", lexTable[i].token);
        // If an identifier, print variable name
        if(lexTable[i].token == 2)
            fprintf(stdout, "%s ", lexTable[i].name);
        // If number, print its ascii number value
        else if(lexTable[i].token == 3)
            fprintf(stdout, "%d ", lexTable[i].value);
    }
    fprintf(stdout, "\n");
}

// subject to change
void lex()
{
    FILE *fp;
    char c;
    char *lexName;
    int lookedAhead, inComment, lexVal, tempIndex, lexTableIndex = 0;

    readAndPrintFile(argv[1]);

    fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
        printf("Invalid File Name\n");
        return 0;
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
                    return 0;
                }
            }
            
            lexTable[lexTableIndex].name = lexName;
            lexTable[lexTableIndex++].token = checkReserved(lexName);
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
                    return 0;
                }       
            }

            if (isalpha(c))
            {
                printf("Error: Invalid Identifier");
                return 0;
            }

            lexTable[lexTableIndex].name = lexName;
            lexTable[lexTableIndex].value = lexVal;
            lexTable[lexTableIndex++].token = 3; // numsym token
        }
        // Checks character c with special symbols
        else 
        {
            lookedAhead = 0;
            switch (c)
            {
                case '+':
                    lexTable[lexTableIndex].name = "+";
                    lexTable[lexTableIndex++].token = 4;
                    break;
                case '-':
                    lexTable[lexTableIndex].name = "-";
                    lexTable[lexTableIndex++].token = 5;
                    break;
                case '*':
                    lexTable[lexTableIndex].name = "*";
                    lexTable[lexTableIndex++].token = 6;
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
                        lexTable[lexTableIndex].name = "/";
                        lexTable[lexTableIndex++].token = 7;
                    }
                    break;
                case '%':
                    lexTable[lexTableIndex].name = "%";
                    lexTable[lexTableIndex++].token = 8;
                    break;
                case '(':
                    lexTable[lexTableIndex].name = "(";
                    lexTable[lexTableIndex++].token = 15;
                    break;
                case ')':
                    lexTable[lexTableIndex].name = ")";
                    lexTable[lexTableIndex++].token = 16;
                    break;
                case '=':
                    lexTable[lexTableIndex].name = "=";
                    lexTable[lexTableIndex++].token = 9;
                    break;
                case ',':
                    lexTable[lexTableIndex].name = ",";
                    lexTable[lexTableIndex++].token = 17;
                    break;
                case '.':
                    lexTable[lexTableIndex].name = ".";
                    lexTable[lexTableIndex++].token = 19;
                    break;
                case '<':
                    c = fgetc(fp);
                    if (c == '=')
                    {
                        lexTable[lexTableIndex].name = "<=";
                        lexTable[lexTableIndex++].token = 12;
                    }
                    else if ( c == '>')
                    {
                        lexTable[lexTableIndex].name = "<>";
                        lexTable[lexTableIndex++].token = 10;
                    }
                    else
                    {
                        lookedAhead = 1;
                        lexTable[lexTableIndex].name = "<";
                        lexTable[lexTableIndex++].token = 11;
                    }
                    break;
                case '>':
                    c = fgetc(fp);
                    if (c == '=')
                    {
                        lexTable[lexTableIndex].name = ">=";
                        lexTable[lexTableIndex++].token = 14;
                    }
                    else
                    {
                        lookedAhead = 1;
                        lexTable[lexTableIndex].name = ">";
                        lexTable[lexTableIndex++].token = 13;
                    }
                    break;
                case ';':
                    lexTable[lexTableIndex].name = ";";
                    lexTable[lexTableIndex++].token = 18;
                    break;
                case ':':
                    c = fgetc(fp);
                    // encloses break since : is invalid
                    if (c == '=')
                    {
                        lexTable[lexTableIndex].name = ":=";
                        lexTable[lexTableIndex++].token = 20;
                        break;
                    }
                default:
                    printf("Error: Invalid symbol.");
                    return 0;
            }
            if (!lookedAhead)
                c = fgetc(fp);
        }
    }

    printLexTable(lexTable, lexTableIndex);

    fclose(fp);
    return 0;
}

int getToken()
{
    return lexTable[tokenCounter++].token;
}

void factor()
{

}

void term()
{

}

void expression()
{

}

void condition()
{

}

void statement()
{
    // will have write and read
}

void block()
{

}

void program()
{

}

int main(int argc, char **argv)
{
    code = calloc(MAX_CODE_LENGTH, sizeof(instruction);
    lexTable = calloc(MAX_LEX_NUMBER, sizeof(lex);
    symbolTable = calloc(MAX_SYMBOL_SIZE, sizeof(symbol));
    tokenCounter = 0;
}