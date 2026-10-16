#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "lexical_analyser.h"

/*List of C Keywords*/
static const char *keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "inline", "int", "long", "register", "restrict", "return", "short",
    "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
    "unsigned", "void", "volatile", "while", "_Bool", "_Complex", "_Imaginary",
    "_Alignas", "_Alignof", "_Atomic", "_Generic", "_Noreturn", "_Static_assert",
    "_Thread_local"
};

/* Check whether the given word is a C keyword*/
static int is_keyword(const char *word)
{
    int i;

    //Compare the word with each keyword
    for (i = 0; i < (int)(sizeof(keywords) / sizeof(keywords[0])); i++)
        if (strcmp(word, keywords[i]) == 0)
            return 1;
    return 0;
}

/*Check whether the character is an operator*/
static int is_operator_char(char c)
{
    return strchr("+-*/%=!<>|&^~?:", c) != NULL && c != '\0';
}

/* Check whether the character is a punctuator*/
static int is_punctuation(char c)
{
    return strchr(";,(){}[].", c) != NULL && c != '\0';
}

/* Extract the token from the line and display it*/
static void emit(const char *line, int start, int end, int row,
                 const char *type, Report *report)
{
    char word[TOKEN_SIZE];
   
    //Calculate the length of the token
    int length = end - start;

    //Check whether the token exceeds the buffer size
    if (length >= TOKEN_SIZE)
    {
        print_error(row, start + 1, "Token too long", report);
        return;
    }

    //Copy the token into a seperate buffer
    memcpy(word, line + start, length);

    //Add null character to terminate the string
    word[length] = '\0';

    //Display the token with its type and pos
    print_token(row, start + 1, type, word, report);
}

/* Read the source file and identify different types of tokens */
void analyze_file(FILE *fp, Report *report)
{
    char line[LINE_SIZE];

    //Initialize line number and comment tracking variables
    int row = 0;
    int in_comment = 0;
    int comment_start = 0;

    //Read the source file line by line
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        int i = 0;
        int started_in_comment = in_comment;
        int saw_comment = 0;

        //Find the length of the current line
        int length = (int)strlen(line);
        //Increment the line number
        row++;

        /* A long input line must not be silently treated as a complete line. */
        if (length > 0 && line[length - 1] != '\n' && !feof(fp))
        {
            int ch;
            print_error(row, 1, "Line exceeds input buffer size", report);

            //skip the remaining characters of the long line
            while ((ch = fgetc(fp)) != '\n' && ch != EOF) { }
            continue;
        }

        //Process each character in the character
        while (line[i] != '\0')
        {
            int start;
            unsigned char c = (unsigned char)line[i];

            //Check whether the current character is inside a block comment
            if (in_comment)
            {
                if (line[i] == '*' && line[i + 1] == '/')
                {
                    in_comment = 0;
                    i += 2;
                }
                else
                    i++;
                continue;
            }
            //Ignore whitespace characters
            if (isspace(c))
            {
                i++;
                continue;
            }
            //Detect single line comments
            if (line[i] == '/' && line[i + 1] == '/')
            {
                saw_comment = 1;
                break;
            }
            //Detect the beginning of a block comment
            if (line[i] == '/' && line[i + 1] == '*')
            {
                in_comment = 1;
                comment_start = row;
                saw_comment = 1;
                i += 2;
                continue;
            }
            //Identify preprocessor directives
            if (line[i] == '#' )
            {
                start = i;

                //Read the complete directive until end of the line
                while (line[i] && line[i] != '\n')
                    i++;
                emit(line, start, i, row, "DIRECTIVE", report);
                break;
            }
            //Identify keywords and identifiers
            if (isalpha(c) || c == '_')
            {
                start = i++;
                //Read all the valid identifiers
                while (isalnum((unsigned char)line[i]) || line[i] == '_')
                    i++;
                {
                    char word[TOKEN_SIZE];
                    int size = i - start;
                    //Check whether the identifier exceeds the buffer size
                    if (size >= TOKEN_SIZE)
                        print_error(row, start + 1, "Identifier too long", report);
                    else
                    {
                        //Extract the identifier
                        memcpy(word, line + start, size);
                        word[size] = '\0';

                        //Classify the word as a keyword or identifier
                        print_token(row, start + 1,
                                    is_keyword(word) ? "KEYWORD" : "IDENTIFIER", word, report);
                    }
                }
                continue;
            }
            //Identify integer and floating point constants
            if (isdigit(c) || (c == '.' && isdigit((unsigned char)line[i + 1])))
            {
                int dots = 0, valid = 1;
                start = i;

                //Read the complete numeric tokens
                while (isalnum((unsigned char)line[i]) || line[i] == '.' || line[i] == '_')
                {
                    //count decimal points
                    if (line[i] == '.')
                        dots++;
                    //mark constant invalid if letters or underscores found
                    if (isalpha((unsigned char)line[i]) || line[i] == '_')
                        valid = 0;
                    i++;
                }
                //more than one decimal point makes the constant invalid
                if (dots > 1)
                    valid = 0;
                //check for invalid numeric constants 
                if (!valid || (line[i - 1] == '.'))
                    print_error(row, start + 1, "Invalid numeric constant", report);
                else
                    emit(line, start, i, row, "CONSTANT", report);
                continue;
            }
            //Identify string and character literals
            if (c == '"' || c == '\'')
            {
                char quote = (char)c;
                int closed = 0;
                int escaped = 0;
                int chars = 0;
                int invalid_escape = 0;
                start = i++;
                //Read characters until the closing quote marks
                while (line[i] && line[i] != '\n')
                {
                    if (line[i] == quote)
                    {
                        i++;
                        closed = 1;
                        break;
                    }
                    //Handle escape sequences
                    if (line[i] == '\\')
                    {
                        const char *esc = "\\\"'0abfnrtv?";
                        //Check for an incomplete escape sequence
                        if (line[i + 1] == '\0' || line[i + 1] == '\n')
                            break;
                        //Validate the escape character
                        if (!strchr(esc, line[i + 1]) &&
                            line[i + 1] != 'x' && line[i + 1] != 'u' && line[i + 1] != 'U' &&
                            !(line[i + 1] >= '0' && line[i + 1] <= '7'))
                            invalid_escape = 1;
                        //Skip the backslash and escape character
                        i += 2;
                        chars++;
                        escaped = 1;
                    }
                    else
                    {
                        //Count normal characters
                        chars++;
                        i++;
                    }
                }
                //Check whether the literal is properly closed
                if (!closed)
                    print_error(row, start + 1, "Unterminated string/character literal", report);
                //Report invalid escape sequences    
                else if (invalid_escape)
                    print_error(row, start + 1, "Invalid escape sequence", report);
                //Validate character literals
                else if (quote == '\'' && (chars != 1 || (!escaped && chars == 0)))
                    print_error(row, start + 1, "Invalid character literal", report);
                //Display the valid string or character literal    
                else
                    emit(line, start, i, row, quote == '"' ? "STRING" : "CHAR_LITERAL", report);
                continue;
            }
            //Identify single character and multi character operations
            if (is_operator_char((char)c))
            {
                start = i++;
                //Check whether the next character forms a valid operator
                if (line[i] && ((strchr("=+-&|<>*/%", line[i]) && line[i] != '\n') ||
                    (line[start] == '-' && line[i] == '>')))
                {
                    /* Only collect combinations commonly used as C operators. */
                    char pair[3] = {line[start], line[i], 0};
                    const char *ops[] = {"==", "!=", "<=", ">=", "++", "--", "&&", "||",
                                         "<<", ">>", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "->"};
                    int k, found = 0;
                    //Compare with the operator with supported ops
                    for (k = 0; k < (int)(sizeof(ops) / sizeof(ops[0])); k++)
                        if (strcmp(pair, ops[k]) == 0)
                            found = 1;
                    //Move forward if a valid operator is found
                    if (found)
                    {
                        i++;
                        if ((pair[0] == '<' || pair[0] == '>') && line[i] == '=')
                            i++;
                    }
                }
                //Display identified operator
                emit(line, start, i, row, "OPERATOR", report);
                continue;
            }
            /* Identify punctuators */
            if (is_punctuation((char)c))
            {
                //check for ellipsis (...) 
                if (line[i] == '.' &&line[i + 1] == '.' && line[i + 2] == '.')
                {
                    emit(line, i, i + 3, row, "PUNCTUATOR", report);
                    i += 3;
                    continue;
                }
                /*check opening and closing brackets */
                if (strchr("(){}[]", c))
                    check_bracket((char)c, row, i + 1, report);

                /* Display the punctuator */
                emit(line, i, i + 1, row, "PUNCTUATOR", report);

                i++;
                continue;
            }
            //Report characters that do not belong to any category
            print_error(row, i + 1, "Unknown character", report);
            i++;
        }
        /* check for semicolons in complete, comment-free lines */
        if (!in_comment && !started_in_comment && !saw_comment)
            check_semicolon(line, row, report);
    }
    //Report an unclosed block comment at end of file
    if (in_comment)
        print_error(comment_start, 1, "Unterminated block comment", report);

    //Check for any unmatched brackets
    finish_brackets(report);
}
