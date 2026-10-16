#include <stdio.h>
#include "lexical_analyser.h"

//Display the heading and col names of lexical analyser
void print_heading(const char *filename)
{
     printf("\n====================== LEXICAL ANALYSER ======================\n");
    printf("Input: %s\n", filename);
    printf("----------------------------------------------------------------\n");
    printf("%-6s %-6s %-18s %s\n", "LINE", "COL", "TYPE", "TOKEN / MESSAGE");
    printf("----------------------------------------------------------------\n");
}

//Dsiplay the identified token with its line num, col and type
void print_token(int line, int column, const char *type, const char *text, Report *report)
{
    printf("%-6d %-6d %-18s %s\n", line, column, type, text);
    //Increment valid token count
    report->tokens++;
}

//Dsiplay error message with its line and col num
void print_error(int line, int column, const char *message, Report *report)
{
    printf("%-6d %-6d %-18s %s\n", line, column, "LEXICAL ERROR", message);
    //Increment error count
    report->errors++;
}

//Dsiplay the summary of valid tokens and errors
void print_summary(const Report *report)
{
    printf("----------------------------------------------------------------\n");
    printf("Valid tokens   : %d\n", report->tokens);
    printf("Lexical errors : %d\n", report->errors);
    printf("================================================================\n");
}
