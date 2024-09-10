/*
 * Name: David Young
 * CWID: 10891350
 * Date: September 06, 2022
 * Course: CSCI 442
 * Project 1
 */


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
    char *line;
    struct node *next;
};

struct node *top_stack = NULL;

void push(char *, long);

/*
 * Push function:
 * - every node that has been "malloc"ed has a test to see if it fails and will exit with exit code 1
 * - takes the line from input file and stores it into a string
 * - a new node is created and the line is stored into it
 * - the top of the stack gets changed
 */

void push(char *s, long len) {
    struct node *new_node = malloc(sizeof(struct node));
    if (new_node == NULL) {
        fprintf(stderr, "error: malloc failed\n");
        exit(1);
    }
    char *node_string = malloc(sizeof(char) * (len + 1));
    if (node_string == NULL) {
        fprintf(stderr, "error: malloc failed\n");
        exit(1);
    }
    for (int i = 0; i < len; i++) {
        node_string[i] = s[i];
    }
    node_string[len] = '\0';
    new_node->line = node_string;
    new_node->next = top_stack;    
    top_stack = new_node;
}

/*
 * main function:
 * - reads in arguments and checks to see if it is valid input
 * - reads and opens file using getline
 * - exit condition to stop reading in lines (or printing them depending what the user chose) when top_stack == NULL
 */

int main(int argc, char *argv[])
{   
    FILE *istream, *ostream;
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    if (argc == 2) {
        ostream = stdout;
    } else if (argc == 3) {
        if (strncmp(argv[1], argv[2], strlen(argv[1])) == 0) {
            fprintf(stderr, "error: input and output file must differ\n");
            exit(1);
        }
        ostream = fopen(argv[2], "w+");
    } else {
        fprintf(stderr, "usage: reverse <input> <output>\n");
        exit(1);
    }

    istream = fopen(argv[1], "r");
    
    if (istream == NULL) {
        fprintf(stderr, "error: cannot open file '%s'\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    while ((nread = getline(&line, &len, istream)) != -1) {
        push(line, (long) nread);
    }
    
    while (top_stack != NULL) {
        fprintf(ostream, "%s", top_stack->line);
        free(top_stack->line);
        struct node *tmp = top_stack;
        top_stack = top_stack->next;
        free(tmp);
    }

    free(line);
    fclose(istream);
    fclose(ostream);
    exit(EXIT_SUCCESS);
}
