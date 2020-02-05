/*

  ----------------------------------------------------
  vsm - vector space model data similarity
  ----------------------------------------------------

  Copyright (c) 2008 Jason Bittel <jason.bittel@gmail.com>
 
*/

#ifndef _HAVE_INDEX_H
#define _HAVE_INDEX_H

typedef struct index_stats INDEX_STATS;
struct index_stats {
        int max_height;
        int max_freq;
        int num_nodes;
        int num_insertions;
};

extern INDEX_STATS stats;

void initialize_index();
void insert_word(char *w);
float calculate_similarity(char **query);
void destroy_index();
void free_index();

#endif /* ! _HAVE_INDEX_H */
