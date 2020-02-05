/*

  ----------------------------------------------------
  vsm - vector space model data similarity
  ----------------------------------------------------

  Copyright (c) 2008 Jason Bittel <jason.bittel@gmail.com>

*/

/*
  These functions define, build, and operate on the index. This index is currently
  maintained as an unbalanced binary tree. We refer to it as an 'index' and not a
  'tree' so that the underlying data structure could change without significant
  refactoring.
*/

#define NODE_BLOCKSIZE 100
#define ALLOC_BLOCKSIZE 10

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "error.h"
#include "index.h"

typedef struct index_node INDEX_NODE;
struct index_node {
        char *word;
        unsigned int freq;
        INDEX_NODE *left, *right;
};

static INDEX_NODE *terms = NULL;
static INDEX_NODE *stack = NULL;
static INDEX_NODE **alloc = NULL;
INDEX_STATS stats;

float sum_norm_component(INDEX_NODE *node, int max_freq);
int get_frequency(char *w);
INDEX_NODE *get_node(INDEX_NODE **stack);
INDEX_NODE *free_nodes(INDEX_NODE *node, INDEX_NODE *stack);

/* Initialize the index and associated statistics */
void initialize_index() {
        if (terms) free_index(); /* Make sure we start fresh */
 
        stats.max_height = 0;
        stats.max_freq = 1;
        stats.num_nodes = 0;
        stats.num_insertions = 0;

        return;
}

/* Insert a new node into the index in its proper position;
   if node already exists, increment its frequency count */
void insert_word(char *w) {
        INDEX_NODE **node = &terms;
        int cmp;
        int curr_height = 0;

#ifdef DEBUG
        ASSERT(w);
#endif

        /* Search for node; if found increment its frequency and
           update maximum frequency as necessary */
        while (*node) {
                curr_height++;
                cmp = strcmp(w, (*node)->word);
                if (cmp > 0) {
                        node = &(*node)->right;
                } else if (cmp < 0) {
                        node = &(*node)->left;
                } else {
                        (*node)->freq++;
                        stats.num_insertions++;
                        if ((*node)->freq > stats.max_freq)
                                stats.max_freq = (*node)->freq;

                        return;
                }
        }

        /* Node not found, so insert a new one into the index */
        *node = get_node(&stack);
 
        if (((*node)->word = (char *) malloc(strlen(w) + 1)) == NULL) {
                DIE("Cannot allocate memory for node word");
        }

        strcpy((*node)->word, w);
        (*node)->left = (*node)->right = NULL;
        (*node)->freq = 1;
        stats.num_nodes++;
        stats.num_insertions++;

        /* Update maximum height encountered */
        curr_height++;
        if (curr_height > stats.max_height)
                stats.max_height = curr_height;

        return;
}

/* Iterate over the query vector and calculate the similarity against the index */
float calculate_similarity(char **query) {
        float term_freq, term_weight, norm_comp;
        float similarity = 0;
        char **i;

#ifdef DEBUG
        ASSERT(query);
#endif

        /* Bail if index is empty */
        if (!terms) return -1;

        /* 
         * Calculate the cosine normalization component across the index:
         *    1 / sqrt(summation((tf / max tf)^2))
         */
        norm_comp = sqrt(sum_norm_component(terms, stats.max_freq));
        PRINT("Cosine normalization component is %.2f", norm_comp);

        /* 
         * Iterate through each term in the query vector and calculate the
         * corresponding document term weight:
         *    (tf / max tf) / normalization component
         *
         * Since each query term has a weight of 1, the similarity is merely
         * the sum of the document term weights. Since the same query term can
         * appear multiple times, technically they are weighted by frequency.
         */
        for (i = query; *i; i++) {
                term_freq = get_frequency(*i) / (float) stats.max_freq;
                term_weight = term_freq / norm_comp;
                PRINT("   '%s' occurs %.2f times with weight %.2f", *i, term_freq, term_weight);

                similarity += term_weight;
        }

        PRINT("Similarity: %.4f", similarity);

        return similarity;
}

/* Calculate the index summation portion of the cosine normalization component */
float sum_norm_component(INDEX_NODE *node, int max_freq) {
        float term_freq;

#ifdef DEBUG
        ASSERT(max_freq > 0);
#endif

        if (!node) return 0;

        term_freq = node->freq / (float) max_freq;
        term_freq *= term_freq;

        term_freq += sum_norm_component(node->left, max_freq);
        term_freq += sum_norm_component(node->right, max_freq);

        return term_freq;
}

/* Return the frequency of the parameter word; return 0 if not found */
int get_frequency(char *w) {
        INDEX_NODE *node = terms;
        int cmp;

        while (node) {
                cmp = strcmp(w, node->word);
                if (cmp > 0) {
                        node = node->right;
                } else if (cmp < 0) {
                        node = node->left;
                } else {
                        return node->freq;
                }
        }

        return 0;
}

/*** MEMORY MANAGEMENT/ALLOCATION FUNCTIONS ***/

/* Get a new node from either the free stack or the allocated block;
   if the block is empty, allocate a new chunk of memory */
INDEX_NODE *get_node(INDEX_NODE **stack) {
        static INDEX_NODE *block = NULL, *tail, **mv;
        static int alloc_size = 0;
        INDEX_NODE *head, **tmp;
 
        if (*stack != NULL) {
                head = *stack;
                *stack = (*stack)->left;
                head->left = NULL;
        } else if (block != NULL) {
                head = block;
                if (block == tail)
                        block = NULL;
                else
                        block++;
        } else {
                if ((block = (INDEX_NODE *) calloc(NODE_BLOCKSIZE, sizeof(INDEX_NODE))) == NULL) {
                        DIE("Cannot calloc memory for node block");
                }

                /* Store pointer to allocated block so we can free it later */
                if (alloc == NULL) {
                        if ((alloc = (INDEX_NODE **) malloc(ALLOC_BLOCKSIZE * sizeof(INDEX_NODE *))) == NULL) {
                                DIE("Cannot malloc memory for blocks array");
                        }

                        mv = alloc;
                }

                *mv = block;
 
                if (++alloc_size % ALLOC_BLOCKSIZE == 0) {
                        tmp = realloc(alloc, ((alloc_size + ALLOC_BLOCKSIZE) * sizeof(INDEX_NODE *)));
                        if (!tmp) {
                                DIE("Cannot realloc memory for blocks array");
                        }
                        alloc = tmp;
                        mv = alloc + alloc_size - 1;
                }
 
                mv++;
                *mv = NULL;

                /* Update pointers with new block information */
                tail = block + NODE_BLOCKSIZE - 1;
                head = block;
                if (block == tail) {
                        block = NULL;
                } else {
                        block++;
                }
        }

        return head;
}

/* Release all memory allocated by get_node() back to the OS; only
   called at program termination */
void destroy_index() {
        INDEX_NODE **i;

        if (!alloc) return;
        if (terms) free_index();

        for (i = alloc; *i; i++) {
                free(*i);
        }

        free(alloc);

        return;
}

/* Return all nodes to internal stack; this wrapper function is
   necessary as neither index or stack is visible outside this file */
void free_index() {

#ifdef DEBUG
        ASSERT(terms);
#endif

        stack = free_nodes(terms, stack);
        terms = NULL;

        return;
}

/* Recursively traverse the index returning nodes to the free stack */
INDEX_NODE *free_nodes(INDEX_NODE *node, INDEX_NODE *stack) {
        if (!node) return NULL;

        free_nodes(node->left, stack);
        free_nodes(node->right, stack);

        free(node->word);
        memset(node, 0, sizeof(INDEX_NODE));

        /* Reuse the left pointer as a "next" pointer
           so that we don't waste space in each node */
        node->left = stack;
        stack = node;

        return stack;
}
