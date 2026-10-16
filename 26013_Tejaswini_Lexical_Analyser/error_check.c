#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "lexical_analyser.h"

//Check whether character is an open bracket
static int is_open(char ch)
{
    return ch == '(' || ch == '{' || ch == '[';
}

//Check whether the opening and closing brackets match
static int matching(char open, char close)
{
    return (open == '(' && close == ')') ||
           (open == '{' && close == '}') ||
           (open == '[' && close == ']');
}
//Check brackets using stack
void check_bracket(char symbol, int line, int column, Report *report)
{
    char message[100];

    //Check whether the symbol is an opening bracket
    if (is_open(symbol))
    {
        //Check for stack overflow
        if (report->top == STACK_SIZE)
        {
            print_error(line, column, "Too many nested brackets", report);
            return;
        }
        //Push the opening bracket and its line number onto the stack
        report->brackets[report->top] = symbol;
        report->bracket_lines[report->top] = line;
        report->top++;
    }
    //Check whether the symbol is a closing bracket
    else if (symbol == ')' || symbol == '}' || symbol == ']')
    {
        //Check for an unmatched closing bracket
        if (report->top == 0 || !matching(report->brackets[report->top - 1], symbol))
        {
            //Generate error message
            snprintf(message, sizeof(message), "Unmatched closing bracket '%c'", symbol);
            print_error(line, column, message, report);
        }
        else
        {
            //Pop the matching opening bracket from stack
            report->top--;
        }
    }
}

//Check for unclosed opening bracket
void finish_brackets(Report *report)
{
    char message[100];
    while (report->top > 0)
    {
        report->top--;
        //Generate error message for unclosed bracket
        snprintf(message, sizeof(message), "Unclosed opening bracket '%c'", report->brackets[report->top]);
        //Displayerror message with its line number
        print_error(report->bracket_lines[report->top], 1, message, report);
    }
}

/* Check for possible missing semicolons in statements */
void check_semicolon(const char *line, int line_number, Report *report)
{
    char text[LINE_SIZE];
    size_t n = strlen(line);
    size_t start = 0;
    int assignment = 0;
    int i;
    char first[64] = "";

    /* Check whether the line fits inside the buffer */
    if (n >= sizeof(text))
        return;
    /* Copy the input line into a temporary buffer */
    strcpy(text, line);
    /* Remove trailing whitespace characters */
    while (n > 0 && isspace((unsigned char)text[n - 1]))
        text[--n] = '\0';
    /* Skip leading whitespace characters */
    while (isspace((unsigned char)text[start]))
        start++;

    /* Ignore lines that do not require semicolon checking */
    if (!text[start] || text[start] == '#' || text[start] == '/' || text[start] == '*' ||
        text[n - 1] == ';' || text[n - 1] == '{' || text[n - 1] == '}' ||
        text[n - 1] == ':' || text[n - 1] == ',' || text[n - 1] == '\\' ||
        text[n - 1] == '(' || text[n - 1] == '+' || text[n - 1] == '-' ||
        text[n - 1] == '=' || text[n - 1] == '&' || text[n - 1] == '|')
        return;

    sscanf(text + start, "%63[A-Za-z_]", first);

    /* A function definition header does not need a semicolon. */
    //Ignore function definition headers
    if (text[n - 1] == ')' && strchr(text + start, '(') &&
        (!strcmp(first, "int") || !strcmp(first, "void") ||
         !strcmp(first, "char") || !strcmp(first, "float") ||
         !strcmp(first, "double")))
        return;
    
    //Ignore control statements and other special declarations
    if (!strcmp(first, "if") || !strcmp(first, "else") || !strcmp(first, "for") ||
        !strcmp(first, "while") || !strcmp(first, "switch") || !strcmp(first, "do") ||
        !strcmp(first, "case") || !strcmp(first, "default") ||
        !strcmp(first, "struct") || !strcmp(first, "union") ||
        !strcmp(first, "enum") || !strcmp(first, "typedef"))
        return;

    //check whether the statement contains an assignment operator
    for (i = (int)start; i < (int)n; i++)
        if (text[i] == '=' && text[i + 1] != '=' && (i == 0 || text[i - 1] != '='))
            assignment = 1;

    //Report a possible missing semicolon
    if (!strcmp(first, "return") || !strcmp(first, "break") ||
        !strcmp(first, "continue") || !strcmp(first, "int") ||
        !strcmp(first, "float") || !strcmp(first, "double") ||
        !strcmp(first, "char") || !strcmp(first, "long") ||
        !strcmp(first, "short") || !strcmp(first, "unsigned") ||
        !strcmp(first, "signed") || !strcmp(first, "void") || assignment)
        print_error(line_number, (int)n, "Possible missing semicolon ';'", report);
}
