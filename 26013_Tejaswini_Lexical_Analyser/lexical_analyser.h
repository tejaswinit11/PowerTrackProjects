#ifndef LEXICAL_ANALYSER_H
#define LEXICAL_ANALYSER_H

#include <stdio.h>

#define LINE_SIZE 2048
#define TOKEN_SIZE 256
#define STACK_SIZE 512

typedef struct
{
    int tokens;                          //Total num of tokens identified
    int errors;                          //Total num of errors detected
    int line;                            //Line number info
    char brackets[STACK_SIZE];           //stack to stoe opening brackets
    int bracket_lines[STACK_SIZE];       //Store line numbes of opening braces
    int top;                             //Index of top element in the stack
} Report;

/* Function declarations */

/* Display usage Instructions*/
void display_help(void);

/* Read the source file and identify tokens */
void analyze_file(FILE *fp, Report *report);

/* Display the heading with the source filename */
void print_heading(const char *filename);

/* Display the token with its type and position */
void print_token(int line, int column, const char *type, const char *text, Report *report);

/*Display lexical errors with line and column numbers*/
void print_error(int line, int column, const char *message, Report *report);

/* Display the total number of tokens and errors */
void print_summary(const Report *report);

/* Check whether opening and closing brackets match */
void check_bracket(char symbol, int line, int column, Report *report);

/* Check for unmatched opening brackets */
void finish_brackets(Report *report);

/* Check for possible missing semicolons */
void check_semicolon(const char *line, int line_number, Report *report);


#endif
