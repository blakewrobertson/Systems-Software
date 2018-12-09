/*
 * Blake Robertson
 * COP 3402 Compiler Assignment
 * NID: bl762962
 */

#include "token.h"
#include "data.h"
#include "symbol.h"
#include <string.h>
#include <stdlib.h>

/**
 * This pointer is set when by codeGenerator() func and used by printEmittedCode() func.
 *
 * You are not required to use it anywhere. The implemented part of the skeleton
 * handles the printing. Instead, you are required to fill the vmCode properly by making
 * use of emit() func.
 * */
FILE* _out;

/**
 * Token list iterator used by the code generator. It will be set once entered to
 * codeGenerator() and reset before exiting codeGenerator().
 *
 * It is better to use the given helper functions to make use of token list iterator.
 * */
TokenListIterator _token_list_it;

/**
 * Current level. Use this to keep track of the current level for the symbol table entries.
 * */
unsigned int currentLevel;

/**
 * Current scope. Use this to keep track of the current scope for the symbol table entries.
 * NULL means global scope.
 * */
Symbol* currentScope;

/**
 * Symbol table.
 * */
SymbolTable symbolTable;

/**
 * The array of instructions that the generated(emitted) code will be held.
 * */
Instruction vmCode[MAX_CODE_LENGTH];

/**
 * The next index in the array of instructions (vmCode) to be filled.
 * */
int nextCodeIndex;

/**
 * The id of the register currently being used.
 * */
int currentReg;

/**
 * The number of variables
 * */
int numVar;

/**
 * Emits the instruction whose fields are given as parameters.
 * Internally, writes the instruction to vmCode[nextCodeIndex] and returns the
 * nextCodeIndex by post-incrementing it.
 * If MAX_CODE_LENGTH is reached, prints an error message on stderr and exits.
 * */
int emit(int OP, int R, int L, int M);

/**
 * Prints the emitted code array (vmCode) to output file.
 *
 * This func is called in the given codeGenerator() function. You are not required
 * to have another call to this function in your code.
 * */
void printEmittedCodes();

/**
 * Returns the current token using the token list iterator.
 * If it is the end of tokens, returns token with id nulsym.
 * */
Token getCurrentToken();

/**
 * Returns the type of the current token. Returns nulsym if it is the end of tokens.
 * */
int getCurrentTokenType();

/**
 * Advances the position of TokenListIterator by incrementing the current token
 * index by one.
 * */
void nextToken();

/**
 * Functions used for non-terminals of the grammar
 *
 * rel-op func is removed on purpose. For code generation, it is easier to parse
 * rel-op as a part of condition.
 * */
int program();
int block();
int const_declaration();
int var_declaration();
int proc_declaration();
int statement();
int condition();
int expression();
int term();
int factor();

/******************************************************************************/
/* Definitions of helper functions starts *************************************/
/******************************************************************************/

Token getCurrentToken()
{
    return getCurrentTokenFromIterator(_token_list_it);
}

int getCurrentTokenType()
{
    return getCurrentToken().id;
}

void nextToken()
{
    _token_list_it.currentTokenInd++;
}

/**
 * Given the code generator error code, prints error message on file by applying
 * required formatting.
 * */
void printCGErr(int errCode, FILE* fp)
{
    if(!fp || !errCode) return;
    
    fprintf(fp, "CODE GENERATOR ERROR[%d]: %s.\n", errCode, codeGeneratorErrMsg[errCode]);
}

int emit(int OP, int R, int L, int M)
{
    if(nextCodeIndex == MAX_CODE_LENGTH)
    {
        fprintf(stderr, "MAX_CODE_LENGTH(%d) reached. Emit is unsuccessful: terminating code generator..\n", MAX_CODE_LENGTH);
        exit(0);
    }
    
    vmCode[nextCodeIndex] = (Instruction){ .op = OP, .r = R, .l = L, .m = M};
    
    return nextCodeIndex++;
}

void printEmittedCodes()
{
    for(int i = 0; i < nextCodeIndex; i++)
    {
        Instruction c = vmCode[i];
        fprintf(_out, "%d %d %d %d\n", c.op, c.r, c.l, c.m);
    }
}

/******************************************************************************/
/* Definitions of helper functions ends ***************************************/
/******************************************************************************/

/**
 * Advertised codeGenerator function. Given token list, which is possibly the
 * output of the lexer, parses a program out of tokens and generates code.
 * If encountered, returns the error code.
 *
 * Returning 0 signals successful code generation.
 * Otherwise, returns a non-zero code generator error code.
 * */
int codeGenerator(TokenList tokenList, FILE* out)
{
    // Set output file pointer
    _out = out;
    
    /**
     * Create a token list iterator, which helps to keep track of the current
     * token being parsed.
     * */
    _token_list_it = getTokenListIterator(&tokenList);
    
    // Initialize current level to 0, which is the global level
    currentLevel = 0;
    
    // Initialize current scope to NULL, which is the global scope
    currentScope = NULL;
    
    // The index on the vmCode array that the next emitted code will be written
    nextCodeIndex = 0;
    
    // The id of the register currently being used
    currentReg = 0;
    
    numVar = 0;
    
    // Initialize symbol table
    initSymbolTable(&symbolTable);
    
    // Start parsing by parsing program as the grammar suggests.
    int err = program();
    
    // Print symbol table - if no error occured
    if(!err)
    {
        // Print the emitted codes to the file
        printEmittedCodes();
    }
    // Reset output file pointer
    _out = NULL;
    
    // Reset the global TokenListIterator
    _token_list_it.currentTokenInd = 0;
    _token_list_it.tokenList = NULL;
    
    // Delete symbol table
    deleteSymbolTable(&symbolTable);
    
    // Return err code - which is 0 if parsing was successful
    return err;
}

// Already implemented.
int program()
{
    // Generate code for block
    int err = block();
    if(err) return err;
    
    // After parsing block, periodsym should show up
    if( getCurrentTokenType() == periodsym )
    {
        // Consume token
        nextToken();
        
        // End of program, emit halt code
        emit(SIO_HALT, 0, 0, 3);
        
        return 0;
    }
    else
    {
        // Periodsym was expected. Return error code 6.
        return 6;
    }
}

int block()
{
    currentLevel++;
    int space = 4;
    int jmpIndex = 0;
    Symbol* prev = currentScope;
    if (getCurrentTokenType() == constsym)
    {
        int err = const_declaration();
        if (err) return err;
    }
    if (getCurrentTokenType() == varsym)
    {
        int err = var_declaration();
        if (err) return err;
    }
    space += numVar;
    emit(INC, 0, 0, numVar);
    while (getCurrentTokenType() == procsym)
    {
        jmpIndex = nextCodeIndex;
        emit (JMP, 0, 0, 0);
        proc_declaration();
    }
    vmCode[jmpIndex].m = nextCodeIndex;
    int err = statement();
    if (err) return err;
    emit(2, 0, 0, 0);
    currentScope = prev;
    currentLevel--;
    return 0;
}

int const_declaration()
{
    Token token;
    Symbol symbol;
    do
    {
        nextToken();
        if (getCurrentTokenType() != identsym)
            return 3;
        token = getCurrentToken();
        strcpy(symbol.name, token.lexeme);
        nextToken();
        if (getCurrentTokenType() != eqsym)
            return 2;
        nextToken();
        if (getCurrentTokenType() != numbersym)
            return 1;
        token = getCurrentToken();
        symbol.value = atoi(token.lexeme);
        symbol.level = currentLevel;
        symbol.scope = currentScope;
        symbol.type = CONST;
        symbol.address = 0;
        addSymbol(&symbolTable, symbol);
        nextToken();
    }
    while (getCurrentTokenType() == commasym);
    {
        if (getCurrentTokenType() != semicolonsym)
            return 4;
        nextToken();
    }

    
    return 0;
}

int var_declaration()
{
    Token token;
    Symbol symbol;
    do
    {
        nextToken();
        if (getCurrentTokenType() != identsym)
            return 3;
        token = getCurrentToken();
        strcpy(symbol.name, token.lexeme);
        symbol.type = VAR;
        symbol.value = 0;
        symbol.scope = currentScope;
        symbol.level = currentLevel;
        symbol.address = numVar;
        addSymbol(&symbolTable, symbol);
        nextToken();
        numVar++;
    }
    while (getCurrentTokenType() == commasym);
    if (getCurrentTokenType() != semicolonsym)
        return 4;
    nextToken();
    return 0;
}

int proc_declaration()
{
    Token token;
    Symbol symbol;
    nextToken();
    if (getCurrentTokenType() != identsym)
        return 3;
    token = getCurrentToken();
    strcpy(symbol.name, token.lexeme);
    symbol.type = PROC;
    symbol.value = 0;
    symbol.level = currentLevel;
    symbol.scope = currentScope;
    //printf("1. %s\n", symbol.scope->name);
    symbol.address = nextCodeIndex;
    addSymbol(&symbolTable, symbol);
    nextToken();
    if (getCurrentTokenType() != semicolonsym)
        return 5;
    nextToken();
    //emit(INC, 0, 0, 4);
    int err = block();
    if (err) return err;
    if (getCurrentTokenType() != semicolonsym)
        return 5;
    nextToken();
    return 0;
}

int statement()
{
    if (getCurrentTokenType() == identsym)
    {
        Symbol* symbol = findSymbol(&symbolTable, currentScope, getCurrentToken().lexeme);
        if (symbol == NULL)
            return 15;
        else if (symbol->type != VAR)
            return 16;
        nextToken();
        if (getCurrentTokenType() != becomessym)
            return 7;
        nextToken();
        int err = expression();
        if (err) return err;
        emit (STO, currentReg--, symbol->level - currentLevel, symbol->address);
    }
    else if (getCurrentTokenType() == callsym)
    {
        nextToken();
        if (getCurrentTokenType() != identsym)
            return 8;
        Symbol* symbol = findSymbol(&symbolTable, currentScope, getCurrentToken().lexeme);
        if (symbol == NULL)
            return 15;
        else if (symbol->type != PROC)
            return 17;
        emit (CAL, 0, currentLevel - symbol->level, symbol->address);
        nextToken();
    }
    else if (getCurrentTokenType() == beginsym)
    {
        nextToken();
        int err = statement();
        if (err) return err;
        while (getCurrentTokenType() == semicolonsym)
        {
            nextToken();
            err = statement();
            if (err) return err;
        }
        if (getCurrentTokenType() != endsym)
            return 10;
        nextToken();
    }
    else if (getCurrentTokenType() == ifsym)
    {
        nextToken();
        int err = condition();
        if (err) return err;
        if (getCurrentTokenType() != thensym)
            return 9;
        nextToken();
        int jmpcIndex = nextCodeIndex;
        emit(JPC, 0, 0, 0);
        err = statement();
        if (err) return err;
        int jmpIndex = nextCodeIndex;
        emit (JMP, 0, 0, 0);
        vmCode[jmpcIndex].m = nextCodeIndex;
        if (getCurrentTokenType() == elsesym)
        {
            nextToken();
            err = statement();
            if (err) return err;
        }
        vmCode[jmpIndex].m = nextCodeIndex;
    }
    else if (getCurrentTokenType() == whilesym)
    {
        int codeIndex1 = nextCodeIndex;
        nextToken();
        int err = condition();
        if (err) return err;
        int codeIndex2 = nextCodeIndex;
        emit (JPC, 0, 0, 0);
        if (getCurrentTokenType() != dosym)
            return 11;
        nextToken();
        err = statement();
        if (err) return err;
        emit (JMP, 0, 0, codeIndex1);
        vmCode[codeIndex2].m = nextCodeIndex;
    }
    else if (getCurrentTokenType() == writesym || getCurrentTokenType() == readsym)
    {
        int tokenVal = getCurrentTokenType();
        nextToken();
        if (getCurrentTokenType() != identsym)
            return 3;
        //printf("%s7.\n", currentScope->name);
        Symbol* symbol = findSymbol(&symbolTable, currentScope, getCurrentToken().lexeme);
        //printf("4. %d %d %s %d\n", symbol->level, currentLevel, symbol->name
        //,symbol->address);
        if (symbol == NULL)
            return 15;
        if (tokenVal == readsym)
        {
            if (symbol->type != VAR)
                return 19;
            emit (SIO_READ, currentReg, 0, 2);
            emit (STO, currentReg--, currentLevel - symbol->level, symbol->address);
        }
        else if (tokenVal == writesym)
        {
            if (symbol->type == CONST)
            {
                emit (LIT, ++currentReg, 0, symbol->value);
                emit (SIO_WRITE, currentReg, 0, 1);
            }
            else if (symbol->type == VAR)
            {
                //printf("5. %s\n", symbol->name);
                emit (LOD, currentReg, symbol->level - currentLevel,
                      symbol->address);
                emit (SIO_WRITE, currentReg, 0, 1);
            }
            else
                return 18;
        }
        nextToken();
    }
    
    return 0;
}

int condition()
{
    if (getCurrentTokenType() == oddsym)
    {
        nextToken();
        int err = expression();
        if (err) return err;
        emit(ODD, currentReg, 0, currentReg %2);
    }
    else
    {
        int err = expression();
        if (err) return err;
        if (getCurrentTokenType() == eqsym || getCurrentTokenType() == neqsym || getCurrentTokenType() == lessym
            || getCurrentTokenType() == leqsym || getCurrentTokenType() == gtrsym || getCurrentTokenType() == geqsym)
        {
            int relop = getCurrentTokenType();
            nextToken();
            err = expression();
                if (err) return err;
            if (relop == eqsym)
                emit(EQL, currentReg, currentReg-1, currentReg-2);
            else if (relop == neqsym)
                emit(NEQ, currentReg, currentReg-1, currentReg-2);
            else if (relop == lessym)
                emit(LSS, currentReg, currentReg-1, currentReg-2);
            else if (relop == leqsym)
                emit(LEQ, currentReg, currentReg-1, currentReg-2);
            else if (relop == gtrsym)
                emit(GTR, currentReg, currentReg-1, currentReg-2);
            else if (relop == geqsym)
                emit(GEQ, currentReg, currentReg-1, currentReg-2);
        }
        else
            return 12;
    }
    return 0;
}

int expression()
{
    //Checks if there is a '+' or '-'
    if (getCurrentTokenType() == plussym || getCurrentTokenType() == minussym)
    {
        int operation = getCurrentTokenType();
        nextToken();
        int err = term();
        if (err) return err;
        if (operation == minussym)
            emit (NEG, currentReg, currentReg-1, 0);
    }
    else
    {
        int err = term();
        if (err) return err;
    }
    
    while (getCurrentTokenType() == plussym || getCurrentTokenType() == minussym)
    {
        int operation = getCurrentTokenType();
        nextToken();
        int err = term();
        if (err) return err;
        if (operation == plussym)
        {
            printf("currentReg: %d\n", currentReg);
            emit(ADD, currentReg-1, currentReg-1, currentReg);
            currentReg--;
        }
        else if (operation == minussym)
        {
            emit(SUB, currentReg-1, currentReg-1, currentReg);
            currentReg--;
        }
    }
    return 0;
}

int term()
{
    int err = factor();
    if (err) return err;
    while (getCurrentTokenType() == multsym || getCurrentTokenType() == slashsym)
    {
        int operation = getCurrentTokenType();
        nextToken();
        int err = factor();
        if (err) return err;
        if (operation == multsym)
        {
            emit(MUL, currentReg-1, currentReg-1, currentReg);
            currentReg--;
        }
        else if (operation == slashsym)
        {
            emit(DIV, currentReg-1, currentReg-1, currentReg);
            currentReg--;
        }
    }
    
    return 0;
}

int factor()
{
    if (getCurrentTokenType() == identsym)
    {
        Symbol* symbol = findSymbol(&symbolTable, currentScope, getCurrentToken().lexeme);
        if (symbol == NULL)
            return 15;
        if (symbol->type == CONST)
        {
            emit(LIT, ++currentReg, 0, symbol->value);
        }
        else if (symbol->type == VAR)
        {
            emit (STO, currentReg--, symbol->level - currentLevel, symbol->address);
        }
        else
        {
            return 16;
        }
        nextToken();
    }
    else if (getCurrentTokenType() == numbersym)
    {
        int value = atoi(getCurrentToken().lexeme);
        emit(LIT, ++currentReg, 0, value);
        nextToken();
    }
    else if (getCurrentTokenType() == lparentsym)
    {
        nextToken();
        int err = expression();
        if (err) return err;
        if (getCurrentTokenType() != rparentsym)
            return 13;
        nextToken();
    }
    else
        return 14;
    
    return 0;
}

