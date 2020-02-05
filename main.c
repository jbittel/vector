/*

  ----------------------------------------------------
  vsm - vector space model data similarity
  ----------------------------------------------------

  Copyright (c) 2008 Jason Bittel <jason.bittel@gmail.com>

*/

#define PROG_NAME "vsm"
#define PROG_VER "0.0.1"
#define MAX_LINE_LEN 2048
#define QUERY_BLOCKSIZE 50

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "index.h"
#include "stem.h"

int getopt(int, char * const *, const char *);
void build_query(char *filename);
void destroy_query();
void build_index(char *filename);
char *standardize_line(char *str);
int stop_word(char *word);
void handle_signal(int sig);
void cleanup();
void display_usage();

/* Query vector data structure */
static char **query = NULL;

/* Command line arguments */
static int min_len = 0;
static int do_stemming = 1;
static int do_stop_words = 1;
static char *termfile = NULL;
int quiet_mode = 0;               /* Defined as extern in error.h */

/* Read query terms from input file and insert them into a dynamic array */
void build_query(char *filename) {
        FILE *fp;
        char buf[MAX_LINE_LEN];
        char *line, *p, *term;
        char **mv, **tmp;
        int size = 0;

        if ((fp = fopen(filename, "r")) == NULL) {
                DIE("Cannot open file '%s'", filename);
        }
        PRINT("Reading query file '%s'", filename);

        if ((query = (char **) malloc(QUERY_BLOCKSIZE * sizeof(char *))) == NULL) {
                DIE("Cannot malloc memory for term array");
        }

        mv = query;
        while ((line = fgets(buf, sizeof(buf) - 1, fp))) {
                if (*line == '#') continue; /* Skip comments */
 
                line = standardize_line(line);

                for (p = line; (term = strtok(p, " ")); p = NULL) {
                        if (min_len && (strlen(term) < min_len)) continue;
                        if (do_stop_words && stop_word(term)) continue;
                        if (do_stemming) stem(term);

                        if ((*mv = (char *) malloc(strlen(term) + 1)) == NULL) {
                                *mv = NULL;
                                destroy_query();
                                DIE("Cannot malloc memory for query term");
                        }
                        strcpy(*mv, term);

                        if (++size % QUERY_BLOCKSIZE == 0) {
                                tmp = realloc(query, ((size + QUERY_BLOCKSIZE) * sizeof(char *)));
                                if (!tmp) {
                                        *mv = NULL;
                                        destroy_query();
                                        DIE("Cannot realloc memory for query term array");
                                }
                                query = tmp;
                                mv = query + size - 1;
                        }
 
                        mv++;
                }
        }

        *mv = NULL;
        fclose(fp);

        if (size == 0) DIE("No query terms found in '%s'", filename);

        PRINT("Query vector constructed with dimensionality of %d", size);

        return;
}

/* Free memory allocated for query */
void destroy_query() {
        char **i;

        if (!query) return;

        for (i = query; *i; i++) {
                free(*i);
        }

        free(query);

        return;
}

/* Read data from file and insert into an index structure; if filename
   is NULL, read from STDIN */
void build_index(char *filename) {
        FILE *fp;
        char buf[MAX_LINE_LEN];
        char *line, *p, *word;

        if (filename) {
                if ((fp = fopen(filename, "r")) == NULL) {
                        DIE("\nCannot open file '%s'", filename);
                }
                PRINT("\nReading data file '%s'", filename);
        } else {
                fp = stdin;
                PRINT("\nReading data from STDIN");
        }

        initialize_index();

        while ((line = fgets(buf, sizeof(buf) - 1, fp))) {
                line = standardize_line(line);

                /* TODO: Instead of merely stripping unwanted characters and
                 * tokenizing on spaces, we ought to implement a word delimeters
                 * table and split words using those. Really, it will probably be
                 * an inverted table: everything *not* specified is considered a
                 * word boundary */
                for (p = line; (word = strtok(p, " ")); p = NULL) {
                        if (min_len && (strlen(word) < min_len)) continue;
                        if (do_stop_words && stop_word(word)) continue;
                        if (do_stemming) stem(word);

                        insert_word(word);
                }
        }

        fclose(fp);

        if (stats.num_nodes == 0) {
                WARN("No data found in '%s'", filename);
        } else {
                PRINT("Data file contained %d valid terms", stats.num_insertions);
                PRINT("Index constructed with %d nodes and height %d", stats.num_nodes, stats.max_height);
                PRINT("Maximum term frequency encountered was %d", stats.max_freq);
        }

        return;
}

/* Normalize string into a sequence of single space delimited alphanumeric words (modifies str);
   initial concept borrowed (and heavily adapted) from xref.c by Bert Bos */
char *standardize_line(char *str) {
        char *i, *j;

#ifdef DEBUG
        ASSERT(str);
#endif

        for (j = str, i = str; *j != '\0'; j++) {
                if (isupper(*j)) *i++ = tolower(*j);            /* Convert to lowercase */
                else if (isalnum(*j) || *j == '-') *i++ = *j;   /* Keep only alphanumerics and dashes */
                else if (isspace(*j) && i != str &&            
                            *(i - 1) != ' ') *i++ = ' ';        /* Separate all words with a single space */
        }
        *i = '\0';

        return str;
}

/* Remove common correlative words that typically convey no meaning */
int stop_word(char *word) {

#ifdef DEBUG
        ASSERT(word);
#endif

        switch (*word) {
                case 'a':
                        if (!strcmp(word, "a")) return 1;
                        if (!strcmp(word, "after")) return 1;
                        if (!strcmp(word, "also")) return 1;
                        if (!strcmp(word, "although")) return 1;
                        if (!strcmp(word, "an")) return 1;
                        if (!strcmp(word, "and")) return 1;
                        break;
                case 'b':
                        if (!strcmp(word, "because")) return 1;
                        if (!strcmp(word, "both")) return 1;
                        if (!strcmp(word, "but")) return 1;
                        break;
                case 'e':
                        if (!strcmp(word, "either")) return 1;
                        break;
                case 'f':
                        if (!strcmp(word, "for")) return 1;
                        break;
                case 'i':
                        if (!strcmp(word, "if")) return 1;
                        break;
                case 'n':
                        if (!strcmp(word, "nor")) return 1;
                        if (!strcmp(word, "not")) return 1;
                        break;
                case 'o':
                        if (!strcmp(word, "or")) return 1;
                        break;
                case 's':
                        if (!strcmp(word, "so")) return 1;
                        break;
                case 't':
                        if (!strcmp(word, "the")) return 1;
                        break;
                case 'u':
                        if (!strcmp(word, "unless")) return 1;
                        break;
                case 'y':
                        if (!strcmp(word, "yet")) return 1;
                        break;
                default: return 0;
        }

        return 0;
}

/* Attempt a clean shutdown if a monitored signal is received */
void handle_signal(int sig) {
        switch (sig) {
                case SIGINT:
                        cleanup();
                        break;
                default:
                        PRINT("Ignoring unknown signal '%d'", sig);
                        return;
        }

        exit(sig);
}

/* Centralize cleanup functions for exit conditions */
void cleanup() {
        destroy_query();
        destroy_index();

        return;
}

/* Display program help/usage information */
void display_usage() {
        printf("%s version %s\n", PROG_NAME, PROG_VER);
        printf("Usage: %s [OPTION] -t TERMFILE [DATAFILE]...\n\n", PROG_NAME);

        printf("If no datafile, read standard input\n"
              "    -h   display this help information and exit\n"
              "    -m   specify a minimum word length\n"
              "    -q   disable non-critical output\n"
              "    -s   disable term stemming\n"
              "    -t   input file containing query terms\n"
              "    -w   disable removal of stop words\n\n");

        printf("Additional information can be found at:\n"
              "    http://dumpsterventures.com/jason/vsm\n\n");

        exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
        int opt;
        extern char *optarg;
        extern int optind;

        signal(SIGINT, handle_signal);
 
        /* Process command line arguments */
        while ((opt = getopt(argc, argv, "hm:qst:w")) != -1) {
                switch (opt) {
                        case 'h': display_usage(); break;
                        case 'm': min_len = atoi(optarg); break;
                        case 'q': quiet_mode = 1; break;
                        case 's': do_stemming = 0; break;
                        case 't': termfile = optarg; break;
                        case 'w': do_stop_words = 0; break;
                        default: display_usage();
                }
        }

        if (!termfile) DIE("No query term file provided");

        if (min_len < 0) {
                WARN("Invalid -m value, setting to 0");
                min_len = 0;
        }

        if (min_len != 0) PRINT("Minimum word length set to %d", min_len);
        if (do_stemming == 0) PRINT("Term stemming disabled");
        if (do_stop_words == 0) PRINT("Stop words disabled");

        build_query(termfile);

        if (optind == argc) {
                /* No datafile provided, read from STDIN */
                build_index(NULL);
                printf("Similarity: %.4f\n", calculate_similarity(query));
        } else {
                /* One or more datafiles given on command line */
                while (optind < argc) {
                        build_index(argv[optind++]);
                        printf("Similarity: %.4f\n", calculate_similarity(query));
                }
        }

        cleanup();

        return EXIT_SUCCESS;
}
