/*
 * Copyright (C) 1995       Sam Rushing <rushing@nightmare.com>,
 * Copyright (C) 2012-2018  Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
 */

/* $Id: avl.h,v 1.7 2003/07/07 01:10:14 brendan Exp $ */

#ifndef _LIBIGLOO__AVL_H_
#define _LIBIGLOO__AVL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <igloo/config.h>

#define AVL_KEY_PRINTER_BUFLEN (256)

#include <igloo/thread.h>

typedef struct igloo_avl_node_tag {
  void *        key;
  struct igloo_avl_node_tag *    left;
  struct igloo_avl_node_tag *    right;  
  struct igloo_avl_node_tag *    parent;
  /*
   * The lower 2 bits of <rank_and_balance> specify the balance
   * factor: 00==-1, 01==0, 10==+1.
   * The rest of the bits are used for <rank>
   */
  unsigned int        rank_and_balance;
#if defined(IGLOO_CTC_HAVE_AVL_NODE_LOCK)
  igloo_rwlock_t rwlock;
#endif
} igloo_avl_node;

#define AVL_GET_BALANCE(n)    ((int)(((n)->rank_and_balance & 3) - 1))

#define AVL_GET_RANK(n)    (((n)->rank_and_balance >> 2))

#define AVL_SET_BALANCE(n,b) \
  ((n)->rank_and_balance) = \
    (((n)->rank_and_balance & (~3)) | ((int)((b) + 1)))

#define AVL_SET_RANK(n,r) \
  ((n)->rank_and_balance) = \
    (((n)->rank_and_balance & 3) | (r << 2))

struct igloo__avl_tree;

typedef int (*igloo_avl_key_compare_fun_type)    (void * compare_arg, void * a, void * b);
typedef int (*igloo_avl_iter_fun_type)    (void * key, void * iter_arg);
typedef int (*igloo_avl_iter_index_fun_type)    (unsigned long index, void * key, void * iter_arg);
typedef int (*igloo_avl_free_key_fun_type)    (void * key);
typedef int (*igloo_avl_key_printer_fun_type)    (char *, void *);

/*
 * <compare_fun> and <compare_arg> let us associate a particular compare
 * function with each tree, separately.
 */

typedef struct igloo__avl_tree {
  igloo_avl_node *            root;
  unsigned int          height;
  unsigned int          length;
  igloo_avl_key_compare_fun_type    compare_fun;
  void *             compare_arg;
  igloo_rwlock_t rwlock;
} igloo_avl_tree;

igloo_avl_tree * igloo_avl_tree_new (igloo_avl_key_compare_fun_type compare_fun, void * compare_arg);
igloo_avl_node * igloo_avl_node_new (void * key, igloo_avl_node * parent);

void igloo_avl_tree_free (
  igloo_avl_tree *        tree,
  igloo_avl_free_key_fun_type    free_key_fun
  );

int igloo_avl_insert (
  igloo_avl_tree *        ob,
  void *        key
  );

int igloo_avl_delete (
  igloo_avl_tree *        tree,
  void *        key,
  igloo_avl_free_key_fun_type    free_key_fun
  );

int igloo_avl_get_by_index (
  igloo_avl_tree *        tree,
  unsigned long        index,
  void **        value_address
  );

int igloo_avl_get_by_key (
  igloo_avl_tree *        tree,
  void *        key,
  void **        value_address
  );

int igloo_avl_iterate_inorder (
  igloo_avl_tree *        tree,
  igloo_avl_iter_fun_type    iter_fun,
  void *        iter_arg
  );

int igloo_avl_iterate_index_range (
  igloo_avl_tree *        tree,
  igloo_avl_iter_index_fun_type iter_fun,
  unsigned long        low,
  unsigned long        high,
  void *        iter_arg
  );

int igloo_avl_get_span_by_key (
  igloo_avl_tree *        tree,
  void *        key,
  unsigned long *    low,
  unsigned long *    high
  );

int igloo_avl_get_span_by_two_keys (
  igloo_avl_tree *        tree,
  void *        key_a,
  void *        key_b,
  unsigned long *    low,
  unsigned long *    high
  );

int igloo_avl_verify (igloo_avl_tree * tree);

void igloo_avl_print_tree (
  igloo_avl_tree *        tree,
  igloo_avl_key_printer_fun_type key_printer
  );

igloo_avl_node *igloo_avl_get_first(igloo_avl_tree *tree);

igloo_avl_node *igloo_avl_get_prev(igloo_avl_node * node);

igloo_avl_node *igloo_avl_get_next(igloo_avl_node * node);

/* These two are from David Ascher <david_ascher@brown.edu> */

int igloo_avl_get_item_by_key_most (
  igloo_avl_tree *        tree,
  void *        key,
  void **        value_address
  );

int igloo_avl_get_item_by_key_least (
  igloo_avl_tree *        tree,
  void *        key,
  void **        value_address
  );

/* optional locking stuff */
void igloo_avl_tree_rlock(igloo_avl_tree *tree);
void igloo_avl_tree_wlock(igloo_avl_tree *tree);
void igloo_avl_tree_unlock(igloo_avl_tree *tree);
void avl_node_rlock(igloo_avl_node *node);
void avl_node_wlock(igloo_avl_node *node);
void avl_node_unlock(igloo_avl_node *node);

#ifdef __cplusplus
}
#endif

#endif /* __AVL_H */
