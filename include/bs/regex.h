// Taken from https://github.com/rindag-devs/musl-regex-standalone
//
// ----------------------------------------------------------------------
// Copyright © 2005-2020 Rich Felker, et al.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ----------------------------------------------------------------------

// Authors/contributors include:

// A. Wilcox
// Ada Worcester
// Alex Dowad
// Alex Suykov
// Alexander Monakov
// Andre McCurdy
// Andrew Kelley
// Anthony G. Basile
// Aric Belsito
// Arvid Picciani
// Bartosz Brachaczek
// Benjamin Peterson
// Bobby Bingham
// Boris Brezillon
// Brent Cook
// Chris Spiegel
// Clément Vasseur
// Daniel Micay
// Daniel Sabogal
// Daurnimator
// David Carlier
// David Edelsohn
// Denys Vlasenko
// Dmitry Ivanov
// Dmitry V. Levin
// Drew DeVault
// Emil Renner Berthing
// Fangrui Song
// Felix Fietkau
// Felix Janda
// Gianluca Anzolin
// Hauke Mehrtens
// He X
// Hiltjo Posthuma
// Isaac Dunham
// Jaydeep Patil
// Jens Gustedt
// Jeremy Huntwork
// Jo-Philipp Wich
// Joakim Sindholt
// John Spencer
// Julien Ramseier
// Justin Cormack
// Kaarle Ritvanen
// Khem Raj
// Kylie McClain
// Leah Neukirchen
// Luca Barbato
// Luka Perkov
// M Farkas-Dyck (Strake)
// Mahesh Bodapati
// Markus Wichmann
// Masanori Ogino
// Michael Clark
// Michael Forney
// Mikhail Kremnyov
// Natanael Copa
// Nicholas J. Kain
// orc
// Pascal Cuoq
// Patrick Oppenlander
// Petr Hosek
// Petr Skocik
// Pierre Carrier
// Reini Urban
// Rich Felker
// Richard Pennington
// Ryan Fairfax
// Samuel Holland
// Segev Finer
// Shiz
// sin
// Solar Designer
// Stefan Kristiansson
// Stefan O'Rear
// Szabolcs Nagy
// Timo Teräs
// Trutz Behn
// Valentin Ochs
// Will Dietz
// William Haddon
// William Pitcock

// Portions of this software are derived from third-party works licensed
// under terms compatible with the above MIT license:

// The TRE regular expression implementation (src/regex/reg* and
// src/regex/tre*) is Copyright © 2001-2008 Ville Laurikari and licensed
// under a 2-clause BSD license (license text in the source files). The
// included version has been heavily modified by Rich Felker in 2012, in
// the interests of size, simplicity, and namespace cleanliness.

// Much of the math library code (src/math/* and src/complex/*) is
// Copyright © 1993,2004 Sun Microsystems or
// Copyright © 2003-2011 David Schultz or
// Copyright © 2003-2009 Steven G. Kargl or
// Copyright © 2003-2009 Bruce D. Evans or
// Copyright © 2008 Stephen L. Moshier or
// Copyright © 2017-2018 Arm Limited
// and labelled as such in comments in the individual source files. All
// have been licensed under extremely permissive terms.

// The ARM memcpy code (src/string/arm/memcpy.S) is Copyright © 2008
// The Android Open Source Project and is licensed under a two-clause BSD
// license. It was taken from Bionic libc, used on Android.

// The AArch64 memcpy and memset code (src/string/aarch64/*) are
// Copyright © 1999-2019, Arm Limited.

// The implementation of DES for crypt (src/crypt/crypt_des.c) is
// Copyright © 1994 David Burren. It is licensed under a BSD license.

// The implementation of blowfish crypt (src/crypt/crypt_blowfish.c) was
// originally written by Solar Designer and placed into the public
// domain. The code also comes with a fallback permissive license for use
// in jurisdictions that may not recognize the public domain.

// The smoothsort implementation (src/stdlib/qsort.c) is Copyright © 2011
// Valentin Ochs and is licensed under an MIT-style license.

// The x86_64 port was written by Nicholas J. Kain and is licensed under
// the standard MIT terms.

// The mips and microblaze ports were originally written by Richard
// Pennington for use in the ellcc project. The original code was adapted
// by Rich Felker for build system and code conventions during upstream
// integration. It is licensed under the standard MIT terms.

// The mips64 port was contributed by Imagination Technologies and is
// licensed under the standard MIT terms.

// The powerpc port was also originally written by Richard Pennington,
// and later supplemented and integrated by John Spencer. It is licensed
// under the standard MIT terms.

// All other files which have no copyright comments are original works
// produced specifically for use as part of this library, written either
// by Rich Felker, the main author of the library, or by one or more
// contibutors listed above. Details on authorship of individual files
// can be found in the git version control history of the project. The
// omission of copyright and license comments in each file is in the
// interest of source tree size.

// In addition, permission is hereby granted for all public header files
// (include/* and arch/*/bits/*) and crt files intended to be linked into
// applications (crt/*, ldso/dlstart.c, and arch/*/crt_arch.h) to omit
// the copyright notice and permission notice otherwise required by the
// license, and to use these files without any requirement of
// attribution. These files include substantial contributions from:

// Bobby Bingham
// John Spencer
// Nicholas J. Kain
// Rich Felker
// Richard Pennington
// Stefan Kristiansson
// Szabolcs Nagy

// all of whom have explicitly granted such permission.

// This file previously contained text expressing a belief that most of
// the files covered by the above exception were sufficiently trivial not
// to be subject to copyright, resulting in confusion over whether it
// negated the permissions granted in the license. In the spirit of
// permissive licensing, and of not having licensing issues being an
// obstacle to adoption, that text has been removed.

#ifndef MUSL_REGEX_STANDALONE_H_
#define MUSL_REGEX_STANDALONE_H_

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef ptrdiff_t regoff_t;

typedef struct re_pattern_buffer {
    size_t re_nsub;
    void *__opaque, *__padding[4];
    size_t __nsub2;
    char __padding2;
} regex_t;

typedef struct {
    regoff_t rm_so;
    regoff_t rm_eo;
} regmatch_t;

#define REG_EXTENDED 1
#define REG_ICASE 2
#define REG_NEWLINE 4
#define REG_NOSUB 8

#define REG_NOTBOL 1
#define REG_NOTEOL 2

#define REG_OK 0
#define REG_NOMATCH 1
#define REG_BADPAT 2
#define REG_ECOLLATE 3
#define REG_ECTYPE 4
#define REG_EESCAPE 5
#define REG_ESUBREG 6
#define REG_EBRACK 7
#define REG_EPAREN 8
#define REG_EBRACE 9
#define REG_BADBR 10
#define REG_ERANGE 11
#define REG_ESPACE 12
#define REG_BADRPT 13

#define REG_ENOSYS -1

#ifndef CHARCLASS_NAME_MAX
#    define CHARCLASS_NAME_MAX 2048
#endif

#ifndef RE_DUP_MAX
#    define RE_DUP_MAX 255
#endif

int regcomp(regex_t *, const char *, int);
int regexec(const regex_t *, const char *, size_t, regmatch_t *, int);
void regfree(regex_t *);

size_t regerror(int, const regex_t *, char *, size_t);

/* From regerror.c */

/* Error message strings for error codes listed in `regex.h'.  This list
   needs to be in sync with the codes listed there, naturally. */

/* Converted to single string by Rich Felker to remove the need for
 * data relocations at runtime, 27 Feb 2006. */

static const char musl_regex_messages[] = {"No error\0"
                                           "No match\0"
                                           "Invalid regexp\0"
                                           "Unknown collating element\0"
                                           "Unknown character class name\0"
                                           "Trailing backslash\0"
                                           "Invalid back reference\0"
                                           "Missing ']'\0"
                                           "Missing ')'\0"
                                           "Missing '}'\0"
                                           "Invalid contents of {}\0"
                                           "Invalid character range\0"
                                           "Out of memory\0"
                                           "Repetition not preceded by valid expression\0"
                                           "\0Unknown error"};

inline size_t regerror(int e, const regex_t *preg, char *buf, size_t size) {
    const char *s;
    for (s = musl_regex_messages; e && *s; e--, s += strlen(s) + 1)
        ;
    if (!*s)
        s++;
    return 1 + snprintf(buf, size, "%s", s);
}

/*
  tre-internal.h - TRE internal definitions

  Copyright (c) 2001-2009 Ville Laurikari <vl@iki.fi>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#define TRE_REGEX_T_FIELD __opaque
typedef int reg_errcode_t;

typedef wchar_t tre_char_t;

/* Wide characters. */
typedef wint_t tre_cint_t;
#define TRE_CHAR_MAX 0x10ffff

#define tre_isalnum iswalnum
#define tre_isalpha iswalpha
#define tre_isblank iswblank
#define tre_iscntrl iswcntrl
#define tre_isdigit iswdigit
#define tre_isgraph iswgraph
#define tre_islower iswlower
#define tre_isprint iswprint
#define tre_ispunct iswpunct
#define tre_isspace iswspace
#define tre_isupper iswupper
#define tre_isxdigit iswxdigit

#define tre_tolower towlower
#define tre_toupper towupper
#define tre_strlen wcslen

/* Use system provided iswctype() and wctype(). */
typedef wctype_t tre_ctype_t;
#define tre_isctype iswctype
#define tre_ctype wctype

/* Returns number of bytes to add to (char *)ptr to make it
   properly aligned for the type. */
#define MUSL_REGEX_ALIGN(ptr, type)                                                                \
    ((((intptr_t)ptr) % sizeof(type)) ? (sizeof(type) - (((intptr_t)ptr) % sizeof(type))) : 0)

#undef MUSL_REGEX_MAX
#undef MUSL_REGEX_MIN
#define MUSL_REGEX_MAX(a, b) (((a) >= (b)) ? (a) : (b))
#define MUSL_REGEX_MIN(a, b) (((a) <= (b)) ? (a) : (b))

/* TNFA transition type. A TNFA state is an array of transitions,
   the terminator is a transition with NULL `state'. */
typedef struct tnfa_transition tre_tnfa_transition_t;

struct tnfa_transition {
    /* Range of accepted characters. */
    tre_cint_t code_min;
    tre_cint_t code_max;
    /* Pointer to the destination state. */
    tre_tnfa_transition_t *state;
    /* ID number of the destination state. */
    int state_id;
    /* -1 terminated array of tags (or NULL). */
    int *tags;
    /* Assertion bitmap. */
    int assertions;
    /* Assertion parameters. */
    union {
        /* Character class assertion. */
        tre_ctype_t class1;
        /* Back reference assertion. */
        int backref;
    } u;
    /* Negative character class assertions. */
    tre_ctype_t *neg_classes;
};

/* Assertions. */
#define MUSL_REGEX_ASSERT_AT_BOL 1               /* Beginning of line. */
#define MUSL_REGEX_ASSERT_AT_EOL 2               /* End of line. */
#define MUSL_REGEX_ASSERT_CHAR_CLASS 4           /* Character class in `class'. */
#define MUSL_REGEX_ASSERT_CHAR_CLASS_NEG 8       /* Character classes in `neg_classes'. */
#define MUSL_REGEX_ASSERT_AT_BOW 16              /* Beginning of word. */
#define MUSL_REGEX_ASSERT_AT_EOW 32              /* End of word. */
#define MUSL_REGEX_ASSERT_AT_WB 64               /* Word boundary. */
#define MUSL_REGEX_ASSERT_AT_WB_NEG 128          /* Not a word boundary. */
#define MUSL_REGEX_ASSERT_MUSL_REGEX_BACKREF 256 /* A back reference in `backref'. */
#define MUSL_REGEX_ASSERT_LAST 256

/* Tag directions. */
typedef enum {
    TRE_TAG_MINIMIZE = 0,
    TRE_TAG_MAXIMIZE = 1
} tre_tag_direction_t;

/* Instructions to compute submatch register values from tag values
   after a successful match.  */
struct tre_submatch_data {
    /* Tag that gives the value for rm_so (submatch start offset). */
    int so_tag;
    /* Tag that gives the value for rm_eo (submatch end offset). */
    int eo_tag;
    /* List of submatches this submatch is contained in. */
    int *parents;
};

typedef struct tre_submatch_data tre_submatch_data_t;

/* TNFA definition. */
typedef struct tnfa tre_tnfa_t;

struct tnfa {
    tre_tnfa_transition_t *transitions;
    unsigned int num_transitions;
    tre_tnfa_transition_t *initial;
    tre_tnfa_transition_t *final;
    tre_submatch_data_t *submatch_data;
    char *firstpos_chars;
    int first_char;
    unsigned int num_submatches;
    tre_tag_direction_t *tag_directions;
    int *minimal_tags;
    int num_tags;
    int num_minimals;
    int end_tag;
    int num_states;
    int cflags;
    int have_backrefs;
    int have_approx;
};

/* from tre-mem.h: */

#define TRE_MEM_BLOCK_SIZE 1024

typedef struct tre_list {
    void *data;
    struct tre_list *next;
} tre_list_t;

typedef struct tre_mem_struct {
    tre_list_t *blocks;
    tre_list_t *current;
    char *ptr;
    size_t n;
    int failed;
    void **provided;
} *tre_mem_t;

#define tre_mem_new_impl __tre_mem_new_impl
#define tre_mem_alloc_impl __tre_mem_alloc_impl
#define tre_mem_destroy __tre_mem_destroy

tre_mem_t tre_mem_new_impl(int provided, void *provided_block);
void *tre_mem_alloc_impl(tre_mem_t mem, int provided, void *provided_block, int zero, size_t size);

/* Returns a new memory allocator or NULL if out of memory. */
#define tre_mem_new() tre_mem_new_impl(0, NULL)

/* Allocates a block of `size' bytes from `mem'.  Returns a pointer to the
   allocated block or NULL if an underlying malloc() failed. */
#define tre_mem_alloc(mem, size) tre_mem_alloc_impl(mem, 0, NULL, 0, size)

/* Allocates a block of `size' bytes from `mem'.  Returns a pointer to the
   allocated block or NULL if an underlying malloc() failed.  The memory
   is set to zero. */
#define tre_mem_calloc(mem, size) tre_mem_alloc_impl(mem, 0, NULL, 1, size)

#ifdef TRE_USE_ALLOCA
/* alloca() versions.  Like above, but memory is allocated with alloca()
   instead of malloc(). */

#    define tre_mem_newa() tre_mem_new_impl(1, alloca(sizeof(struct tre_mem_struct)))

#    define tre_mem_alloca(mem, size)                                                              \
        ((mem)->n >= (size) ? tre_mem_alloc_impl((mem), 1, NULL, 0, (size))                        \
                            : tre_mem_alloc_impl((mem), 1, alloca(TRE_MEM_BLOCK_SIZE), 0, (size)))
#endif /* TRE_USE_ALLOCA */

/* Frees the memory allocator and all memory allocated with it. */
void tre_mem_destroy(tre_mem_t mem);

#define musl_regex_xmalloc malloc
#define musl_regex_xcalloc calloc
#define musl_regex_xfree free
#define musl_regex_xrealloc realloc

/*
  tre-mem.c - TRE memory allocator

  Copyright (c) 2001-2009 Ville Laurikari <vl@iki.fi>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*
  This memory allocator is for allocating small memory blocks efficiently
  in terms of memory overhead and execution speed.  The allocated blocks
  cannot be freed individually, only all at once.  There can be multiple
  allocators, though.
*/

/*
  This memory allocator is for allocating small memory blocks efficiently
  in terms of memory overhead and execution speed.  The allocated blocks
  cannot be freed individually, only all at once.  There can be multiple
  allocators, though.
*/

/* Returns a new memory allocator or NULL if out of memory. */
inline tre_mem_t tre_mem_new_impl(int provided, void *provided_block) {
    tre_mem_t mem;
    if (provided) {
        mem = (tre_mem_t)provided_block;
        memset(mem, 0, sizeof(*mem));
    } else
        mem = (tre_mem_t)musl_regex_xcalloc(1, sizeof(*mem));
    if (mem == NULL)
        return NULL;
    return mem;
}

/* Frees the memory allocator and all memory allocated with it. */
inline void tre_mem_destroy(tre_mem_t mem) {
    tre_list_t *tmp, *l = mem->blocks;

    while (l != NULL) {
        musl_regex_xfree(l->data);
        tmp = l->next;
        musl_regex_xfree(l);
        l = tmp;
    }
    musl_regex_xfree(mem);
}

/* Allocates a block of `size' bytes from `mem'.  Returns a pointer to the
   allocated block or NULL if an underlying malloc() failed. */
inline void *
tre_mem_alloc_impl(tre_mem_t mem, int provided, void *provided_block, int zero, size_t size) {
    void *ptr;

    if (mem->failed) {
        return NULL;
    }

    if (mem->n < size) {
        /* We need more memory than is available in the current block.
           Allocate a new block. */
        tre_list_t *l;
        if (provided) {
            if (provided_block == NULL) {
                mem->failed = 1;
                return NULL;
            }
            mem->ptr = (char *)provided_block;
            mem->n = TRE_MEM_BLOCK_SIZE;
        } else {
            int block_size;
            if (size * 8 > TRE_MEM_BLOCK_SIZE)
                block_size = size * 8;
            else
                block_size = TRE_MEM_BLOCK_SIZE;
            l = (tre_list_t *)musl_regex_xmalloc(sizeof(*l));
            if (l == NULL) {
                mem->failed = 1;
                return NULL;
            }
            l->data = musl_regex_xmalloc(block_size);
            if (l->data == NULL) {
                musl_regex_xfree(l);
                mem->failed = 1;
                return NULL;
            }
            l->next = NULL;
            if (mem->current != NULL)
                mem->current->next = l;
            if (mem->blocks == NULL)
                mem->blocks = l;
            mem->current = l;
            mem->ptr = (char *)l->data;
            mem->n = block_size;
        }
    }

    /* Make sure the next pointer will be aligned. */
    size += MUSL_REGEX_ALIGN(mem->ptr + size, intptr_t);

    /* Allocate from current block. */
    ptr = mem->ptr;
    mem->ptr += size;
    mem->n -= size;

    /* Set to zero if needed. */
    if (zero)
        memset(ptr, 0, size);

    return ptr;
}

/*
  regcomp.c - TRE POSIX compatible regex compilation functions.

  Copyright (c) 2001-2009 Ville Laurikari <vl@iki.fi>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/***********************************************************************
 from tre-compile.h
***********************************************************************/

typedef struct {
    int position;
    int code_min;
    int code_max;
    int *tags;
    int assertions;
    tre_ctype_t class1;
    tre_ctype_t *neg_classes;
    int backref;
} tre_pos_and_tags_t;

/***********************************************************************
 from tre-ast.c and tre-ast.h
***********************************************************************/

/* The different AST node types. */
typedef enum {
    LITERAL,
    CATENATION,
    ITERATION,
    UNION
} tre_ast_type_t;

/* Special subtypes of TRE_LITERAL. */
#define MUSL_REGEX_EMPTY -1     /* Empty leaf (denotes empty string). */
#define MUSL_REGEX_ASSERTION -2 /* Assertion leaf. */
#define MUSL_REGEX_TAG -3       /* Tag leaf. */
#define MUSL_REGEX_BACKREF -4   /* Back reference leaf. */

#define MUSL_REGEX_IS_SPECIAL(x) ((x)->code_min < 0)
#define MUSL_REGEX_IS_EMPTY(x) ((x)->code_min == MUSL_REGEX_EMPTY)
#define MUSL_REGEX_IS_ASSERTION(x) ((x)->code_min == MUSL_REGEX_ASSERTION)
#define MUSL_REGEX_IS_TAG(x) ((x)->code_min == MUSL_REGEX_TAG)
#define MUSL_REGEX_IS_BACKREF(x) ((x)->code_min == MUSL_REGEX_BACKREF)

/* A generic AST node.  All AST nodes consist of this node on the top
   level with `obj' pointing to the actual content. */
typedef struct {
    tre_ast_type_t type; /* Type of the node. */
    void *obj;           /* Pointer to actual node. */
    int nullable;
    int submatch_id;
    int num_submatches;
    int num_tags;
    tre_pos_and_tags_t *firstpos;
    tre_pos_and_tags_t *lastpos;
} tre_ast_node_t;

/* A "literal" node.  These are created for assertions, back references,
   tags, matching parameter settings, and all expressions that match one
   character. */
typedef struct {
    long code_min;
    long code_max;
    int position;
    tre_ctype_t class1;
    tre_ctype_t *neg_classes;
} tre_literal_t;

/* A "catenation" node.	 These are created when two regexps are concatenated.
   If there are more than one subexpressions in sequence, the `left' part
   holds all but the last, and `right' part holds the last subexpression
   (catenation is left associative). */
typedef struct {
    tre_ast_node_t *left;
    tre_ast_node_t *right;
} tre_catenation_t;

/* An "iteration" node.	 These are created for the "*", "+", "?", and "{m,n}"
   operators. */
typedef struct {
    /* Subexpression to match. */
    tre_ast_node_t *arg;
    /* Minimum number of consecutive matches. */
    int min;
    /* Maximum number of consecutive matches. */
    int max;
    /* If 0, match as many characters as possible, if 1 match as few as
       possible.	Note that this does not always mean the same thing as
       matching as many/few repetitions as possible. */
    unsigned int minimal : 1;
} tre_iteration_t;

/* An "union" node.  These are created for the "|" operator. */
typedef struct {
    tre_ast_node_t *left;
    tre_ast_node_t *right;
} tre_union_t;

inline static tre_ast_node_t *tre_ast_new_node(tre_mem_t mem, int type, void *obj) {
    tre_ast_node_t *node = (tre_ast_node_t *)tre_mem_calloc(mem, sizeof *node);
    if (!node || !obj)
        return 0;
    node->obj = obj;
    node->type = (tre_ast_type_t)type;
    node->nullable = -1;
    node->submatch_id = -1;
    return node;
}

inline static tre_ast_node_t *
tre_ast_new_literal(tre_mem_t mem, int code_min, int code_max, int position) {
    tre_ast_node_t *node;
    tre_literal_t *lit;

    lit = (tre_literal_t *)tre_mem_calloc(mem, sizeof *lit);
    node = tre_ast_new_node(mem, LITERAL, lit);
    if (!node)
        return 0;
    lit->code_min = code_min;
    lit->code_max = code_max;
    lit->position = position;
    return node;
}

inline static tre_ast_node_t *
tre_ast_new_iter(tre_mem_t mem, tre_ast_node_t *arg, int min, int max, int minimal) {
    tre_ast_node_t *node;
    tre_iteration_t *iter;

    iter = (tre_iteration_t *)tre_mem_calloc(mem, sizeof *iter);
    node = tre_ast_new_node(mem, ITERATION, iter);
    if (!node)
        return 0;
    iter->arg = arg;
    iter->min = min;
    iter->max = max;
    iter->minimal = minimal;
    node->num_submatches = arg->num_submatches;
    return node;
}

inline static tre_ast_node_t *
tre_ast_new_union(tre_mem_t mem, tre_ast_node_t *left, tre_ast_node_t *right) {
    tre_ast_node_t *node;
    tre_union_t *un;

    if (!left)
        return right;
    un = (tre_union_t *)tre_mem_calloc(mem, sizeof *un);
    node = tre_ast_new_node(mem, UNION, un);
    if (!node || !right)
        return 0;
    un->left = left;
    un->right = right;
    node->num_submatches = left->num_submatches + right->num_submatches;
    return node;
}

inline static tre_ast_node_t *
tre_ast_new_catenation(tre_mem_t mem, tre_ast_node_t *left, tre_ast_node_t *right) {
    tre_ast_node_t *node;
    tre_catenation_t *cat;

    if (!left)
        return right;
    cat = (tre_catenation_t *)tre_mem_calloc(mem, sizeof *cat);
    node = tre_ast_new_node(mem, CATENATION, cat);
    if (!node)
        return 0;
    cat->left = left;
    cat->right = right;
    node->num_submatches = left->num_submatches + right->num_submatches;
    return node;
}

/***********************************************************************
 from tre-stack.c and tre-stack.h
***********************************************************************/

typedef struct tre_stack_rec tre_stack_t;

/* Creates a new stack object.	`size' is initial size in bytes, `max_size'
   is maximum size, and `increment' specifies how much more space will be
   allocated with realloc() if all space gets used up.	Returns the stack
   object or NULL if out of memory. */
inline static tre_stack_t *tre_stack_new(int size, int max_size, int increment);

/* Frees the stack object. */
inline static void tre_stack_destroy(tre_stack_t *s);

/* Returns the current number of objects in the stack. */
inline static int tre_stack_num_objects(tre_stack_t *s);

/* Each tre_stack_push_*(tre_stack_t *s, <type> value) function pushes
   `value' on top of stack `s'.  Returns REG_ESPACE if out of memory.
   This tries to realloc() more space before failing if maximum size
   has not yet been reached.  Returns REG_OK if successful. */
#define musl_regex_declare_pushf(typetag, type)                                                    \
    inline static reg_errcode_t tre_stack_push_##typetag(tre_stack_t *s, type value)

musl_regex_declare_pushf(voidptr, void *);
musl_regex_declare_pushf(int, int);

/* Each tre_stack_pop_*(tre_stack_t *s) function pops the topmost
   element off of stack `s' and returns it.  The stack must not be
   empty. */
#define musl_regex_declare_popf(typetag, type)                                                     \
    inline static type tre_stack_pop_##typetag(tre_stack_t *s)

musl_regex_declare_popf(voidptr, void *);
musl_regex_declare_popf(int, int);

/* Just to save some typing. */
#define MUSL_REGEX_STACK_PUSH(s, typetag, value)                                                   \
    do {                                                                                           \
        status = tre_stack_push_##typetag(s, value);                                               \
    } while (/*CONSTCOND*/ 0)

#define MUSL_REGEX_STACK_PUSHX(s, typetag, value)                                                  \
    {                                                                                              \
        status = tre_stack_push_##typetag(s, value);                                               \
        if (status != REG_OK)                                                                      \
            break;                                                                                 \
    }

#define MUSL_REGEX_STACK_PUSHR(s, typetag, value)                                                  \
    {                                                                                              \
        reg_errcode_t _status;                                                                     \
        _status = tre_stack_push_##typetag(s, value);                                              \
        if (_status != REG_OK)                                                                     \
            return _status;                                                                        \
    }

union tre_stack_item {
    void *voidptr_value;
    int int_value;
};

struct tre_stack_rec {
    int size;
    int max_size;
    int increment;
    int ptr;
    union tre_stack_item *stack;
};

inline static tre_stack_t *tre_stack_new(int size, int max_size, int increment) {
    tre_stack_t *s;

    s = (tre_stack_t *)musl_regex_xmalloc(sizeof(*s));
    if (s != NULL) {
        s->stack = (union tre_stack_item *)musl_regex_xmalloc(sizeof(*s->stack) * size);
        if (s->stack == NULL) {
            musl_regex_xfree(s);
            return NULL;
        }
        s->size = size;
        s->max_size = max_size;
        s->increment = increment;
        s->ptr = 0;
    }
    return s;
}

inline static void tre_stack_destroy(tre_stack_t *s) {
    musl_regex_xfree(s->stack);
    musl_regex_xfree(s);
}

inline static int tre_stack_num_objects(tre_stack_t *s) {
    return s->ptr;
}

inline static reg_errcode_t tre_stack_push(tre_stack_t *s, union tre_stack_item value) {
    if (s->ptr < s->size) {
        s->stack[s->ptr] = value;
        s->ptr++;
    } else {
        if (s->size >= s->max_size) {
            return REG_ESPACE;
        } else {
            union tre_stack_item *new_buffer;
            int new_size;
            new_size = s->size + s->increment;
            if (new_size > s->max_size)
                new_size = s->max_size;
            new_buffer = (union tre_stack_item *)musl_regex_xrealloc(
                s->stack, sizeof(*new_buffer) * new_size);
            if (new_buffer == NULL) {
                return REG_ESPACE;
            }
            assert(new_size > s->size);
            s->size = new_size;
            s->stack = new_buffer;
            tre_stack_push(s, value);
        }
    }
    return REG_OK;
}

#define musl_regex_define_pushf(typetag, type)                                                     \
    musl_regex_declare_pushf(typetag, type) {                                                      \
        union tre_stack_item item;                                                                 \
        item.typetag##_value = value;                                                              \
        return tre_stack_push(s, item);                                                            \
    }

musl_regex_define_pushf(int, int);
musl_regex_define_pushf(voidptr, void *);

#define musl_regex_define_popf(typetag, type)                                                      \
    musl_regex_declare_popf(typetag, type) {                                                       \
        return s->stack[--s->ptr].typetag##_value;                                                 \
    }

musl_regex_define_popf(int, int);
musl_regex_define_popf(voidptr, void *);

/***********************************************************************
 from tre-parse.c and tre-parse.h
***********************************************************************/

/* Parse context. */
typedef struct {
    /* Memory allocator. The AST is allocated using this. */
    tre_mem_t mem;
    /* Stack used for keeping track of regexp syntax. */
    tre_stack_t *stack;
    /* The parsed node after a parse function returns. */
    tre_ast_node_t *n;
    /* Position in the regexp pattern after a parse function returns. */
    const char *s;
    /* The first character of the last subexpression parsed. */
    const char *start;
    /* Current submatch ID. */
    int submatch_id;
    /* Current position (number of literal). */
    int position;
    /* The highest back reference or -1 if none seen so far. */
    int max_backref;
    /* Compilation flags. */
    int cflags;
} tre_parse_ctx_t;

/* Some macros for expanding \w, \s, etc. */
static const struct {
    char c;
    const char *expansion;
} tre_macros[] = {
    {'t', "\t"},
    {'n', "\n"},
    {'r', "\r"},
    {'f', "\f"},
    {'a', "\a"},
    {'e', "\033"},
    {'w', "[[:alnum:]_]"},
    {'W', "[^[:alnum:]_]"},
    {'s', "[[:space:]]"},
    {'S', "[^[:space:]]"},
    {'d', "[[:digit:]]"},
    {'D', "[^[:digit:]]"},
    {0, 0}};

/* Expands a macro delimited by `regex' and `regex_end' to `buf', which
   must have at least `len' items.  Sets buf[0] to zero if the there
   is no match in `tre_macros'. */
inline static const char *tre_expand_macro(const char *s) {
    int i;
    for (i = 0; tre_macros[i].c && tre_macros[i].c != *s; i++)
        ;
    return tre_macros[i].expansion;
}

inline static int tre_compare_lit(const void *a, const void *b) {
    const tre_literal_t *const *la = (const tre_literal_t *const *)a;
    const tre_literal_t *const *lb = (const tre_literal_t *const *)b;
    /* assumes the range of valid code_min is < INT_MAX */
    return la[0]->code_min - lb[0]->code_min;
}

struct literals {
    tre_mem_t mem;
    tre_literal_t **a;
    int len;
    int cap;
};

inline static tre_literal_t *tre_new_lit(struct literals *p) {
    tre_literal_t **a;
    if (p->len >= p->cap) {
        if (p->cap >= 1 << 15)
            return 0;
        p->cap *= 2;
        a = (tre_literal_t **)musl_regex_xrealloc(p->a, p->cap * sizeof *p->a);
        if (!a)
            return 0;
        p->a = a;
    }
    a = p->a + p->len++;
    *a = (tre_literal_t *)tre_mem_calloc(p->mem, sizeof **a);
    return *a;
}

inline static int add_icase_literals(struct literals *ls, int min, int max) {
    tre_literal_t *lit;
    int b, e, c;
    for (c = min; c <= max;) {
        /* assumes islower(c) and isupper(c) are exclusive
           and toupper(c)!=c if islower(c).
           multiple opposite case characters are not supported */
        if (tre_islower(c)) {
            b = e = tre_toupper(c);
            for (c++, e++; c <= max; c++, e++)
                if ((int)tre_toupper(c) != e)
                    break;
        } else if (tre_isupper(c)) {
            b = e = tre_tolower(c);
            for (c++, e++; c <= max; c++, e++)
                if ((int)tre_tolower(c) != e)
                    break;
        } else {
            c++;
            continue;
        }
        lit = tre_new_lit(ls);
        if (!lit)
            return -1;
        lit->code_min = b;
        lit->code_max = e - 1;
        lit->position = -1;
    }
    return 0;
}

/* Maximum number of character classes in a negated bracket expression. */
#define MUSL_REGEX_MAX_NEG_CLASSES 64

struct neg {
    int negate;
    int len;
    tre_ctype_t a[MUSL_REGEX_MAX_NEG_CLASSES];
};

/* TODO: parse bracket into a set of non-overlapping [lo,hi] ranges */

/*
bracket grammar:
Bracket  =  '[' List ']'  |  '[^' List ']'
List     =  Term  |  List Term
Term     =  Char  |  Range  |  Chclass  |  Eqclass
Range    =  Char '-' Char  |  Char '-' '-'
Char     =  Coll  |  coll_single
Meta     =  ']'  |  '-'
Coll     =  '[.' coll_single '.]'  |  '[.' coll_multi '.]'  |  '[.' Meta '.]'
Eqclass  =  '[=' coll_single '=]'  |  '[=' coll_multi '=]'
Chclass  =  '[:' class ':]'

coll_single is a single char collating element but it can be
 '-' only at the beginning or end of a List and
 ']' only at the beginning of a List and
 '^' anywhere except after the openning '['
*/

inline static reg_errcode_t
parse_bracket_terms(tre_parse_ctx_t *ctx, const char *s, struct literals *ls, struct neg *neg) {
    const char *start = s;
    tre_ctype_t class1;
    int min, max;
    wchar_t wc;
    int len;

    for (;;) {
        class1 = 0;
        len = mbtowc(&wc, s, -1);
        if (len <= 0)
            return *s ? REG_BADPAT : REG_EBRACK;
        if (*s == ']' && s != start) {
            ctx->s = s + 1;
            return REG_OK;
        }
        if (*s == '-' && s != start && s[1] != ']' &&
            /* extension: [a-z--@] is accepted as [a-z]|[--@] */
            (s[1] != '-' || s[2] == ']'))
            return REG_ERANGE;
        if (*s == '[' && (s[1] == '.' || s[1] == '='))
            /* collating symbols and equivalence classes are not supported */
            return REG_ECOLLATE;
        if (*s == '[' && s[1] == ':') {
            char tmp[CHARCLASS_NAME_MAX + 1];
            s += 2;
            for (len = 0; len < CHARCLASS_NAME_MAX && s[len]; len++) {
                if (s[len] == ':') {
                    memcpy(tmp, s, len);
                    tmp[len] = 0;
                    class1 = tre_ctype(tmp);
                    break;
                }
            }
            if (!class1 || s[len + 1] != ']')
                return REG_ECTYPE;
            min = 0;
            max = TRE_CHAR_MAX;
            s += len + 2;
        } else {
            min = max = wc;
            s += len;
            if (*s == '-' && s[1] != ']') {
                s++;
                len = mbtowc(&wc, s, -1);
                max = wc;
                /* XXX - Should use collation order instead of
                   encoding values in character ranges. */
                if (len <= 0 || min > max)
                    return REG_ERANGE;
                s += len;
            }
        }

        if (class1 && neg->negate) {
            if (neg->len >= MUSL_REGEX_MAX_NEG_CLASSES)
                return REG_ESPACE;
            neg->a[neg->len++] = class1;
        } else {
            tre_literal_t *lit = tre_new_lit(ls);
            if (!lit)
                return REG_ESPACE;
            lit->code_min = min;
            lit->code_max = max;
            lit->class1 = class1;
            lit->position = -1;

            /* Add opposite-case codepoints if REG_ICASE is present.
               It seems that POSIX requires that bracket negation
               should happen before case-folding, but most practical
               implementations do it the other way around. Changing
               the order would need efficient representation of
               case-fold ranges and bracket range sets even with
               simple patterns so this is ok for now. */
            if (ctx->cflags & REG_ICASE && !class1)
                if (add_icase_literals(ls, min, max))
                    return REG_ESPACE;
        }
    }
}

inline static reg_errcode_t parse_bracket(tre_parse_ctx_t *ctx, const char *s) {
    int i, max, min, negmax, negmin;
    tre_ast_node_t *node = 0, *n;
    tre_ctype_t *nc = 0;
    tre_literal_t *lit;
    struct literals ls;
    struct neg neg;
    reg_errcode_t err;

    ls.mem = ctx->mem;
    ls.len = 0;
    ls.cap = 32;
    ls.a = (tre_literal_t **)musl_regex_xmalloc(ls.cap * sizeof *ls.a);
    if (!ls.a)
        return REG_ESPACE;
    neg.len = 0;
    neg.negate = *s == '^';
    if (neg.negate)
        s++;

    err = parse_bracket_terms(ctx, s, &ls, &neg);
    if (err != REG_OK)
        goto parse_bracket_done;

    if (neg.negate) {
        /*
         * With REG_NEWLINE, POSIX requires that newlines are not matched by
         * any form of a non-matching list.
         */
        if (ctx->cflags & REG_NEWLINE) {
            lit = tre_new_lit(&ls);
            if (!lit) {
                err = REG_ESPACE;
                goto parse_bracket_done;
            }
            lit->code_min = '\n';
            lit->code_max = '\n';
            lit->position = -1;
        }
        /* Sort the array if we need to negate it. */
        qsort(ls.a, ls.len, sizeof *ls.a, tre_compare_lit);
        /* extra lit for the last negated range */
        lit = tre_new_lit(&ls);
        if (!lit) {
            err = REG_ESPACE;
            goto parse_bracket_done;
        }
        lit->code_min = TRE_CHAR_MAX + 1;
        lit->code_max = TRE_CHAR_MAX + 1;
        lit->position = -1;
        /* negated classes */
        if (neg.len) {
            nc = (tre_ctype_t *)tre_mem_alloc(ctx->mem, (neg.len + 1) * sizeof *neg.a);
            if (!nc) {
                err = REG_ESPACE;
                goto parse_bracket_done;
            }
            memcpy(nc, neg.a, neg.len * sizeof *neg.a);
            nc[neg.len] = 0;
        }
    }

    /* Build a union of the items in the array, negated if necessary. */
    negmax = negmin = 0;
    for (i = 0; i < ls.len; i++) {
        lit = ls.a[i];
        min = lit->code_min;
        max = lit->code_max;
        if (neg.negate) {
            if (min <= negmin) {
                /* Overlap. */
                negmin = MUSL_REGEX_MAX(max + 1, negmin);
                continue;
            }
            negmax = min - 1;
            lit->code_min = negmin;
            lit->code_max = negmax;
            negmin = max + 1;
        }
        lit->position = ctx->position;
        lit->neg_classes = nc;
        n = tre_ast_new_node(ctx->mem, LITERAL, lit);
        node = tre_ast_new_union(ctx->mem, node, n);
        if (!node) {
            err = REG_ESPACE;
            break;
        }
    }

parse_bracket_done:
    musl_regex_xfree(ls.a);
    ctx->position++;
    ctx->n = node;
    return err;
}

inline static const char *parse_dup_count(const char *s, int *n) {
    *n = -1;
    if (!isdigit(*s))
        return s;
    *n = 0;
    for (;;) {
        *n = 10 * *n + (*s - '0');
        s++;
        if (!isdigit(*s) || *n > RE_DUP_MAX)
            break;
    }
    return s;
}

inline static const char *parse_dup(const char *s, int ere, int *pmin, int *pmax) {
    int min, max;

    s = parse_dup_count(s, &min);
    if (*s == ',')
        s = parse_dup_count(s + 1, &max);
    else
        max = min;

    if ((max < min && max >= 0) || max > RE_DUP_MAX || min > RE_DUP_MAX || min < 0 ||
        (!ere && *s++ != '\\') || *s++ != '}')
        return 0;
    *pmin = min;
    *pmax = max;
    return s;
}

inline static int hexval(unsigned c) {
    if (c - '0' < 10)
        return c - '0';
    c |= 32;
    if (c - 'a' < 6)
        return c - 'a' + 10;
    return -1;
}

inline static reg_errcode_t marksub(tre_parse_ctx_t *ctx, tre_ast_node_t *node, int subid) {
    if (node->submatch_id >= 0) {
        tre_ast_node_t *n = tre_ast_new_literal(ctx->mem, MUSL_REGEX_EMPTY, -1, -1);
        if (!n)
            return REG_ESPACE;
        n = tre_ast_new_catenation(ctx->mem, n, node);
        if (!n)
            return REG_ESPACE;
        n->num_submatches = node->num_submatches;
        node = n;
    }
    node->submatch_id = subid;
    node->num_submatches++;
    ctx->n = node;
    return REG_OK;
}

/*
BRE grammar:
Regex  =  Branch  |  '^'  |  '$'  |  '^$'  |  '^' Branch  |  Branch '$'  |  '^'
Branch '$' Branch =  Atom  |  Branch Atom Atom   =  char  |  quoted_char  |  '.'
|  Bracket  |  Atom Dup  |  '\(' Branch '\)'  |  back_ref Dup    =  '*'  |  '\{'
Count '\}'  |  '\{' Count ',\}'  |  '\{' Count ',' Count '\}'

(leading ^ and trailing $ in a sub expr may be an anchor or literal as well)

ERE grammar:
Regex  =  Branch  |  Regex '|' Branch
Branch =  Atom  |  Branch Atom
Atom   =  char  |  quoted_char  |  '.'  |  Bracket  |  Atom Dup  |  '(' Regex
')'  |  '^'  |  '$' Dup    =  '*'  |  '+'  |  '?'  |  '{' Count '}'  |  '{'
Count ',}'  |  '{' Count ',' Count '}'

(a*+?, ^*, $+, \X, {, (|a) are unspecified)
*/

inline static reg_errcode_t parse_atom(tre_parse_ctx_t *ctx, const char *s) {
    int len, ere = ctx->cflags & REG_EXTENDED;
    const char *p;
    tre_ast_node_t *node;
    wchar_t wc;
    int i, v = 0, c;
    switch (*s) {
    case '[':
        return parse_bracket(ctx, s + 1);
    case '\\':
        p = tre_expand_macro(s + 1);
        if (p) {
            /* assume \X expansion is a single atom */
            reg_errcode_t err = parse_atom(ctx, p);
            ctx->s = s + 2;
            return err;
        }
        /* extensions: \b, \B, \<, \>, \xHH \x{HHHH} */
        switch (*++s) {
        case 0:
            return REG_EESCAPE;
        case 'b':
            node = tre_ast_new_literal(ctx->mem, MUSL_REGEX_ASSERTION, MUSL_REGEX_ASSERT_AT_WB, -1);
            break;
        case 'B':
            node = tre_ast_new_literal(
                ctx->mem, MUSL_REGEX_ASSERTION, MUSL_REGEX_ASSERT_AT_WB_NEG, -1);
            break;
        case '<':
            node =
                tre_ast_new_literal(ctx->mem, MUSL_REGEX_ASSERTION, MUSL_REGEX_ASSERT_AT_BOW, -1);
            break;
        case '>':
            node =
                tre_ast_new_literal(ctx->mem, MUSL_REGEX_ASSERTION, MUSL_REGEX_ASSERT_AT_EOW, -1);
            break;
        case 'x':
            s++;
            len = 2;
            if (*s == '{') {
                len = 8;
                s++;
            }
            for (i = 0; i < len && v < 0x110000; i++) {
                c = hexval(s[i]);
                if (c < 0)
                    break;
                v = 16 * v + c;
            }
            s += i;
            if (len == 8) {
                if (*s != '}')
                    return REG_EBRACE;
                s++;
            }
            node = tre_ast_new_literal(ctx->mem, v, v, ctx->position++);
            s--;
            break;
        case '{':
        case '+':
        case '?':
            /* extension: treat \+, \? as repetitions in BRE */
            /* reject repetitions after empty expression in BRE */
            if (!ere)
                return REG_BADRPT;
        case '|':
            /* extension: treat \| as alternation in BRE */
            if (!ere) {
                node = tre_ast_new_literal(ctx->mem, MUSL_REGEX_EMPTY, -1, -1);
                s--;
                goto end;
            }
            /* fallthrough */
        default:
            if (!ere && (unsigned)*s - '1' < 9) {
                /* back reference */
                int val = *s - '0';
                node = tre_ast_new_literal(ctx->mem, MUSL_REGEX_BACKREF, val, ctx->position++);
                ctx->max_backref = MUSL_REGEX_MAX(val, ctx->max_backref);
            } else {
                /* extension: accept unknown escaped char
                   as a literal */
                goto parse_literal;
            }
        }
        s++;
        break;
    case '.':
        if (ctx->cflags & REG_NEWLINE) {
            tre_ast_node_t *tmp1, *tmp2;
            tmp1 = tre_ast_new_literal(ctx->mem, 0, '\n' - 1, ctx->position++);
            tmp2 = tre_ast_new_literal(ctx->mem, '\n' + 1, TRE_CHAR_MAX, ctx->position++);
            if (tmp1 && tmp2)
                node = tre_ast_new_union(ctx->mem, tmp1, tmp2);
            else
                node = 0;
        } else {
            node = tre_ast_new_literal(ctx->mem, 0, TRE_CHAR_MAX, ctx->position++);
        }
        s++;
        break;
    case '^':
        /* '^' has a special meaning everywhere in EREs, and at beginning of BRE. */
        if (!ere && s != ctx->start)
            goto parse_literal;
        node = tre_ast_new_literal(ctx->mem, MUSL_REGEX_ASSERTION, MUSL_REGEX_ASSERT_AT_BOL, -1);
        s++;
        break;
    case '$':
        /* '$' is special everywhere in EREs, and at the end of a BRE subexpression.
         */
        if (!ere && s[1] && (s[1] != '\\' || (s[2] != ')' && s[2] != '|')))
            goto parse_literal;
        node = tre_ast_new_literal(ctx->mem, MUSL_REGEX_ASSERTION, MUSL_REGEX_ASSERT_AT_EOL, -1);
        s++;
        break;
    case '*':
    case '{':
    case '+':
    case '?':
        /* reject repetitions after empty expression in ERE */
        if (ere)
            return REG_BADRPT;
    case '|':
        if (!ere)
            goto parse_literal;
    case 0:
        node = tre_ast_new_literal(ctx->mem, MUSL_REGEX_EMPTY, -1, -1);
        break;
    default:
    parse_literal:
        len = mbtowc(&wc, s, -1);
        if (len < 0)
            return REG_BADPAT;
        if (ctx->cflags & REG_ICASE && (tre_isupper(wc) || tre_islower(wc))) {
            tre_ast_node_t *tmp1, *tmp2;
            /* multiple opposite case characters are not supported */
            tmp1 = tre_ast_new_literal(ctx->mem, tre_toupper(wc), tre_toupper(wc), ctx->position);
            tmp2 = tre_ast_new_literal(ctx->mem, tre_tolower(wc), tre_tolower(wc), ctx->position);
            if (tmp1 && tmp2)
                node = tre_ast_new_union(ctx->mem, tmp1, tmp2);
            else
                node = 0;
        } else {
            node = tre_ast_new_literal(ctx->mem, wc, wc, ctx->position);
        }
        ctx->position++;
        s += len;
        break;
    }
end:
    if (!node)
        return REG_ESPACE;
    ctx->n = node;
    ctx->s = s;
    return REG_OK;
}

#define MUSL_REGEX_PUSHPTR(err, s, v)                                                              \
    do {                                                                                           \
        if ((err = tre_stack_push_voidptr(s, v)) != REG_OK)                                        \
            return err;                                                                            \
    } while (0)

#define MUSL_REGEX_PUSHINT(err, s, v)                                                              \
    do {                                                                                           \
        if ((err = tre_stack_push_int(s, v)) != REG_OK)                                            \
            return err;                                                                            \
    } while (0)

inline static reg_errcode_t tre_parse(tre_parse_ctx_t *ctx) {
    tre_ast_node_t *nbranch = 0, *nunion = 0;
    int ere = ctx->cflags & REG_EXTENDED;
    const char *s = ctx->start;
    int subid = 0;
    int depth = 0;
    reg_errcode_t err;
    tre_stack_t *stack = ctx->stack;

    MUSL_REGEX_PUSHINT(err, stack, subid++);
    for (;;) {
        if ((!ere && *s == '\\' && s[1] == '(') || (ere && *s == '(')) {
            MUSL_REGEX_PUSHPTR(err, stack, nunion);
            MUSL_REGEX_PUSHPTR(err, stack, nbranch);
            MUSL_REGEX_PUSHINT(err, stack, subid++);
            s++;
            if (!ere)
                s++;
            depth++;
            nbranch = nunion = 0;
            ctx->start = s;
            continue;
        }
        if ((!ere && *s == '\\' && s[1] == ')') || (ere && *s == ')' && depth)) {
            ctx->n = tre_ast_new_literal(ctx->mem, MUSL_REGEX_EMPTY, -1, -1);
            if (!ctx->n)
                return REG_ESPACE;
        } else {
            err = parse_atom(ctx, s);
            if (err != REG_OK)
                return err;
            s = ctx->s;
        }

    parse_iter:
        for (;;) {
            int min, max;

            if (*s != '\\' && *s != '*') {
                if (!ere)
                    break;
                if (*s != '+' && *s != '?' && *s != '{')
                    break;
            }
            if (*s == '\\' && ere)
                break;
            /* extension: treat \+, \? as repetitions in BRE */
            if (*s == '\\' && s[1] != '+' && s[1] != '?' && s[1] != '{')
                break;
            if (*s == '\\')
                s++;

            /* handle ^* at the start of a BRE. */
            if (!ere && s == ctx->start + 1 && s[-1] == '^')
                break;

            /* extension: multiple consecutive *+?{,} is unspecified,
               but (a+)+ has to be supported so accepting a++ makes
               sense, note however that the RE_DUP_MAX limit can be
               circumvented: (a{255}){255} uses a lot of memory.. */
            if (*s == '{') {
                s = parse_dup(s + 1, ere, &min, &max);
                if (!s)
                    return REG_BADBR;
            } else {
                min = 0;
                max = -1;
                if (*s == '+')
                    min = 1;
                if (*s == '?')
                    max = 1;
                s++;
            }
            if (max == 0)
                ctx->n = tre_ast_new_literal(ctx->mem, MUSL_REGEX_EMPTY, -1, -1);
            else
                ctx->n = tre_ast_new_iter(ctx->mem, ctx->n, min, max, 0);
            if (!ctx->n)
                return REG_ESPACE;
        }

        nbranch = tre_ast_new_catenation(ctx->mem, nbranch, ctx->n);
        if ((ere && *s == '|') || (ere && *s == ')' && depth) ||
            (!ere && *s == '\\' && s[1] == ')') ||
            /* extension: treat \| as alternation in BRE */
            (!ere && *s == '\\' && s[1] == '|') || !*s) {
            /* extension: empty branch is unspecified (), (|a), (a|)
               here they are not rejected but match on empty string */
            int c = *s;
            nunion = tre_ast_new_union(ctx->mem, nunion, nbranch);
            nbranch = 0;

            if (c == '\\' && s[1] == '|') {
                s += 2;
                ctx->start = s;
            } else if (c == '|') {
                s++;
                ctx->start = s;
            } else {
                if (c == '\\') {
                    if (!depth)
                        return REG_EPAREN;
                    s += 2;
                } else if (c == ')')
                    s++;
                depth--;
                err = marksub(ctx, nunion, tre_stack_pop_int(stack));
                if (err != REG_OK)
                    return err;
                if (!c && depth < 0) {
                    ctx->submatch_id = subid;
                    return REG_OK;
                }
                if (!c || depth < 0)
                    return REG_EPAREN;
                nbranch = (tre_ast_node_t *)tre_stack_pop_voidptr(stack);
                nunion = (tre_ast_node_t *)tre_stack_pop_voidptr(stack);
                goto parse_iter;
            }
        }
    }
}

/***********************************************************************
 from tre-compile.c
***********************************************************************/

/*
  TODO:
   - Fix tre_ast_to_tnfa() to recurse using a stack instead of recursive
     function calls.
*/

/*
  Algorithms to setup tags so that submatch addressing can be done.
*/

/* Inserts a catenation node to the root of the tree given in `node'.
   As the left child a new tag with number `tag_id' to `node' is added,
   and the right child is the old root. */
inline static reg_errcode_t tre_add_tag_left(tre_mem_t mem, tre_ast_node_t *node, int tag_id) {
    tre_catenation_t *c;

    c = (tre_catenation_t *)tre_mem_alloc(mem, sizeof(*c));
    if (c == NULL)
        return REG_ESPACE;
    c->left = tre_ast_new_literal(mem, MUSL_REGEX_TAG, tag_id, -1);
    if (c->left == NULL)
        return REG_ESPACE;
    c->right = (tre_ast_node_t *)tre_mem_alloc(mem, sizeof(tre_ast_node_t));
    if (c->right == NULL)
        return REG_ESPACE;

    c->right->obj = node->obj;
    c->right->type = node->type;
    c->right->nullable = -1;
    c->right->submatch_id = -1;
    c->right->firstpos = NULL;
    c->right->lastpos = NULL;
    c->right->num_tags = 0;
    c->right->num_submatches = 0;
    node->obj = c;
    node->type = CATENATION;
    return REG_OK;
}

/* Inserts a catenation node to the root of the tree given in `node'.
   As the right child a new tag with number `tag_id' to `node' is added,
   and the left child is the old root. */
inline static reg_errcode_t tre_add_tag_right(tre_mem_t mem, tre_ast_node_t *node, int tag_id) {
    tre_catenation_t *c;

    c = (tre_catenation_t *)tre_mem_alloc(mem, sizeof(*c));
    if (c == NULL)
        return REG_ESPACE;
    c->right = tre_ast_new_literal(mem, MUSL_REGEX_TAG, tag_id, -1);
    if (c->right == NULL)
        return REG_ESPACE;
    c->left = (tre_ast_node_t *)tre_mem_alloc(mem, sizeof(tre_ast_node_t));
    if (c->left == NULL)
        return REG_ESPACE;

    c->left->obj = node->obj;
    c->left->type = node->type;
    c->left->nullable = -1;
    c->left->submatch_id = -1;
    c->left->firstpos = NULL;
    c->left->lastpos = NULL;
    c->left->num_tags = 0;
    c->left->num_submatches = 0;
    node->obj = c;
    node->type = CATENATION;
    return REG_OK;
}

typedef enum {
    ADDTAGS_RECURSE,
    ADDTAGS_AFTER_ITERATION,
    ADDTAGS_AFTER_UNION_LEFT,
    ADDTAGS_AFTER_UNION_RIGHT,
    ADDTAGS_AFTER_CAT_LEFT,
    ADDTAGS_AFTER_CAT_RIGHT,
    ADDTAGS_SET_SUBMATCH_END
} tre_addtags_symbol_t;

typedef struct {
    int tag;
    int next_tag;
} tre_tag_states_t;

/* Go through `regset' and set submatch data for submatches that are
   using this tag. */
inline static void tre_purge_regset(int *regset, tre_tnfa_t *tnfa, int tag) {
    int i;

    for (i = 0; regset[i] >= 0; i++) {
        int id = regset[i] / 2;
        int start = !(regset[i] % 2);
        if (start)
            tnfa->submatch_data[id].so_tag = tag;
        else
            tnfa->submatch_data[id].eo_tag = tag;
    }
    regset[0] = -1;
}

/* Adds tags to appropriate locations in the parse tree in `tree', so that
   subexpressions marked for submatch addressing can be traced. */
inline static reg_errcode_t
tre_add_tags(tre_mem_t mem, tre_stack_t *stack, tre_ast_node_t *tree, tre_tnfa_t *tnfa) {
    reg_errcode_t status = REG_OK;
    tre_addtags_symbol_t symbol;
    tre_ast_node_t *node = tree; /* Tree node we are currently looking at. */
    int bottom = tre_stack_num_objects(stack);
    /* True for first pass (counting number of needed tags) */
    int first_pass = (mem == NULL || tnfa == NULL);
    int *regset, *orig_regset;
    int num_tags = 0;     /* Total number of tags. */
    int num_minimals = 0; /* Number of special minimal tags. */
    int tag = 0;          /* The tag that is to be added next. */
    int next_tag = 1;     /* Next tag to use after this one. */
    int *parents;         /* Stack of submatches the current submatch is
                             contained in. */
    int minimal_tag = -1; /* Tag that marks the beginning of a minimal match. */
    tre_tag_states_t *saved_states;

    tre_tag_direction_t direction = TRE_TAG_MINIMIZE;
    if (!first_pass) {
        tnfa->end_tag = 0;
        tnfa->minimal_tags[0] = -1;
    }

    regset = (int *)musl_regex_xmalloc(sizeof(*regset) * ((tnfa->num_submatches + 1) * 2));
    if (regset == NULL)
        return REG_ESPACE;
    regset[0] = -1;
    orig_regset = regset;

    parents = (int *)musl_regex_xmalloc(sizeof(*parents) * (tnfa->num_submatches + 1));
    if (parents == NULL) {
        musl_regex_xfree(regset);
        return REG_ESPACE;
    }
    parents[0] = -1;

    saved_states =
        (tre_tag_states_t *)musl_regex_xmalloc(sizeof(*saved_states) * (tnfa->num_submatches + 1));
    if (saved_states == NULL) {
        musl_regex_xfree(regset);
        musl_regex_xfree(parents);
        return REG_ESPACE;
    } else {
        unsigned int i;
        for (i = 0; i <= tnfa->num_submatches; i++)
            saved_states[i].tag = -1;
    }

    MUSL_REGEX_STACK_PUSH(stack, voidptr, node);
    MUSL_REGEX_STACK_PUSH(stack, int, ADDTAGS_RECURSE);

    while (tre_stack_num_objects(stack) > bottom) {
        if (status != REG_OK)
            break;

        symbol = (tre_addtags_symbol_t)tre_stack_pop_int(stack);
        switch (symbol) {
        case ADDTAGS_SET_SUBMATCH_END: {
            int id = tre_stack_pop_int(stack);
            int i;

            /* Add end of this submatch to regset. */
            for (i = 0; regset[i] >= 0; i++)
                ;
            regset[i] = id * 2 + 1;
            regset[i + 1] = -1;

            /* Pop this submatch from the parents stack. */
            for (i = 0; parents[i] >= 0; i++)
                ;
            parents[i - 1] = -1;
            break;
        }

        case ADDTAGS_RECURSE:
            node = (tre_ast_node_t *)tre_stack_pop_voidptr(stack);

            if (node->submatch_id >= 0) {
                int id = node->submatch_id;
                int i;

                /* Add start of this submatch to regset. */
                for (i = 0; regset[i] >= 0; i++)
                    ;
                regset[i] = id * 2;
                regset[i + 1] = -1;

                if (!first_pass) {
                    for (i = 0; parents[i] >= 0; i++)
                        ;
                    tnfa->submatch_data[id].parents = NULL;
                    if (i > 0) {
                        int *p = (int *)musl_regex_xmalloc(sizeof(*p) * (i + 1));
                        if (p == NULL) {
                            status = REG_ESPACE;
                            break;
                        }
                        assert(tnfa->submatch_data[id].parents == NULL);
                        tnfa->submatch_data[id].parents = p;
                        for (i = 0; parents[i] >= 0; i++)
                            p[i] = parents[i];
                        p[i] = -1;
                    }
                }

                /* Add end of this submatch to regset after processing this
                   node. */
                MUSL_REGEX_STACK_PUSHX(stack, int, node->submatch_id);
                MUSL_REGEX_STACK_PUSHX(stack, int, ADDTAGS_SET_SUBMATCH_END);
            }

            switch (node->type) {
            case LITERAL: {
                tre_literal_t *lit = (tre_literal_t *)node->obj;

                if (!MUSL_REGEX_IS_SPECIAL(lit) || MUSL_REGEX_IS_BACKREF(lit)) {
                    int i;
                    if (regset[0] >= 0) {
                        /* Regset is not empty, so add a tag before the
                           literal or backref. */
                        if (!first_pass) {
                            status = tre_add_tag_left(mem, node, tag);
                            tnfa->tag_directions[tag] = direction;
                            if (minimal_tag >= 0) {
                                for (i = 0; tnfa->minimal_tags[i] >= 0; i++)
                                    ;
                                tnfa->minimal_tags[i] = tag;
                                tnfa->minimal_tags[i + 1] = minimal_tag;
                                tnfa->minimal_tags[i + 2] = -1;
                                minimal_tag = -1;
                                num_minimals++;
                            }
                            tre_purge_regset(regset, tnfa, tag);
                        } else {
                            node->num_tags = 1;
                        }

                        regset[0] = -1;
                        tag = next_tag;
                        num_tags++;
                        next_tag++;
                    }
                } else {
                    assert(!MUSL_REGEX_IS_TAG(lit));
                }
                break;
            }
            case CATENATION: {
                tre_catenation_t *cat = (tre_catenation_t *)node->obj;
                tre_ast_node_t *left = cat->left;
                tre_ast_node_t *right = cat->right;
                int reserved_tag = -1;

                /* After processing right child. */
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, node);
                MUSL_REGEX_STACK_PUSHX(stack, int, ADDTAGS_AFTER_CAT_RIGHT);

                /* Process right child. */
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, right);
                MUSL_REGEX_STACK_PUSHX(stack, int, ADDTAGS_RECURSE);

                /* After processing left child. */
                MUSL_REGEX_STACK_PUSHX(stack, int, next_tag + left->num_tags);
                if (left->num_tags > 0 && right->num_tags > 0) {
                    /* Reserve the next tag to the right child. */
                    reserved_tag = next_tag;
                    next_tag++;
                }
                MUSL_REGEX_STACK_PUSHX(stack, int, reserved_tag);
                MUSL_REGEX_STACK_PUSHX(stack, int, ADDTAGS_AFTER_CAT_LEFT);

                /* Process left child. */
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, left);
                MUSL_REGEX_STACK_PUSHX(stack, int, ADDTAGS_RECURSE);

            } break;
            case ITERATION: {
                tre_iteration_t *iter = (tre_iteration_t *)node->obj;

                if (first_pass) {
                    MUSL_REGEX_STACK_PUSHX(stack, int, regset[0] >= 0 || iter->minimal);
                } else {
                    MUSL_REGEX_STACK_PUSHX(stack, int, tag);
                    MUSL_REGEX_STACK_PUSHX(stack, int, iter->minimal);
                }
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, node);
                MUSL_REGEX_STACK_PUSHX(stack, int, ADDTAGS_AFTER_ITERATION);

                MUSL_REGEX_STACK_PUSHX(stack, voidptr, iter->arg);
                MUSL_REGEX_STACK_PUSHX(stack, int, ADDTAGS_RECURSE);

                /* Regset is not empty, so add a tag here. */
                if (regset[0] >= 0 || iter->minimal) {
                    if (!first_pass) {
                        int i;
                        status = tre_add_tag_left(mem, node, tag);
                        if (iter->minimal)
                            tnfa->tag_directions[tag] = TRE_TAG_MAXIMIZE;
                        else
                            tnfa->tag_directions[tag] = direction;
                        if (minimal_tag >= 0) {
                            for (i = 0; tnfa->minimal_tags[i] >= 0; i++)
                                ;
                            tnfa->minimal_tags[i] = tag;
                            tnfa->minimal_tags[i + 1] = minimal_tag;
                            tnfa->minimal_tags[i + 2] = -1;
                            minimal_tag = -1;
                            num_minimals++;
                        }
                        tre_purge_regset(regset, tnfa, tag);
                    }

                    regset[0] = -1;
                    tag = next_tag;
                    num_tags++;
                    next_tag++;
                }
                direction = TRE_TAG_MINIMIZE;
            } break;
            case UNION: {
                tre_union_t *uni = (tre_union_t *)node->obj;
                tre_ast_node_t *left = uni->left;
                tre_ast_node_t *right = uni->right;
                int left_tag;
                int right_tag;

                if (regset[0] >= 0) {
                    left_tag = next_tag;
                    right_tag = next_tag + 1;
                } else {
                    left_tag = tag;
                    right_tag = next_tag;
                }

                /* After processing right child. */
                MUSL_REGEX_STACK_PUSHX(stack, int, right_tag);
                MUSL_REGEX_STACK_PUSHX(stack, int, left_tag);
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, regset);
                MUSL_REGEX_STACK_PUSHX(stack, int, regset[0] >= 0);
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, node);
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, right);
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, left);
                MUSL_REGEX_STACK_PUSHX(stack, int, ADDTAGS_AFTER_UNION_RIGHT);

                /* Process right child. */
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, right);
                MUSL_REGEX_STACK_PUSHX(stack, int, ADDTAGS_RECURSE);

                /* After processing left child. */
                MUSL_REGEX_STACK_PUSHX(stack, int, ADDTAGS_AFTER_UNION_LEFT);

                /* Process left child. */
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, left);
                MUSL_REGEX_STACK_PUSHX(stack, int, ADDTAGS_RECURSE);

                /* Regset is not empty, so add a tag here. */
                if (regset[0] >= 0) {
                    if (!first_pass) {
                        int i;
                        status = tre_add_tag_left(mem, node, tag);
                        tnfa->tag_directions[tag] = direction;
                        if (minimal_tag >= 0) {
                            for (i = 0; tnfa->minimal_tags[i] >= 0; i++)
                                ;
                            tnfa->minimal_tags[i] = tag;
                            tnfa->minimal_tags[i + 1] = minimal_tag;
                            tnfa->minimal_tags[i + 2] = -1;
                            minimal_tag = -1;
                            num_minimals++;
                        }
                        tre_purge_regset(regset, tnfa, tag);
                    }

                    regset[0] = -1;
                    tag = next_tag;
                    num_tags++;
                    next_tag++;
                }

                if (node->num_submatches > 0) {
                    /* The next two tags are reserved for markers. */
                    next_tag++;
                    tag = next_tag;
                    next_tag++;
                }

                break;
            }
            }

            if (node->submatch_id >= 0) {
                int i;
                /* Push this submatch on the parents stack. */
                for (i = 0; parents[i] >= 0; i++)
                    ;
                parents[i] = node->submatch_id;
                parents[i + 1] = -1;
            }

            break; /* end case: ADDTAGS_RECURSE */

        case ADDTAGS_AFTER_ITERATION: {
            int minimal = 0;
            int enter_tag;
            node = (tre_ast_node_t *)tre_stack_pop_voidptr(stack);
            if (first_pass) {
                node->num_tags =
                    ((tre_iteration_t *)node->obj)->arg->num_tags + tre_stack_pop_int(stack);
                minimal_tag = -1;
            } else {
                minimal = tre_stack_pop_int(stack);
                enter_tag = tre_stack_pop_int(stack);
                if (minimal)
                    minimal_tag = enter_tag;
            }

            if (!first_pass) {
                if (minimal)
                    direction = TRE_TAG_MINIMIZE;
                else
                    direction = TRE_TAG_MAXIMIZE;
            }
            break;
        }

        case ADDTAGS_AFTER_CAT_LEFT: {
            int new_tag = tre_stack_pop_int(stack);
            next_tag = tre_stack_pop_int(stack);
            if (new_tag >= 0) {
                tag = new_tag;
            }
            break;
        }

        case ADDTAGS_AFTER_CAT_RIGHT:
            node = (tre_ast_node_t *)tre_stack_pop_voidptr(stack);
            if (first_pass)
                node->num_tags = ((tre_catenation_t *)node->obj)->left->num_tags +
                                 ((tre_catenation_t *)node->obj)->right->num_tags;
            break;

        case ADDTAGS_AFTER_UNION_LEFT:
            /* Lift the bottom of the `regset' array so that when processing
               the right operand the items currently in the array are
               invisible.	 The original bottom was saved at ADDTAGS_UNION and
               will be restored at ADDTAGS_AFTER_UNION_RIGHT below. */
            while (*regset >= 0)
                regset++;
            break;

        case ADDTAGS_AFTER_UNION_RIGHT: {
            int added_tags, tag_left, tag_right;
            tre_ast_node_t *left = (tre_ast_node_t *)tre_stack_pop_voidptr(stack);
            tre_ast_node_t *right = (tre_ast_node_t *)tre_stack_pop_voidptr(stack);
            node = (tre_ast_node_t *)tre_stack_pop_voidptr(stack);
            added_tags = tre_stack_pop_int(stack);
            if (first_pass) {
                node->num_tags = ((tre_union_t *)node->obj)->left->num_tags +
                                 ((tre_union_t *)node->obj)->right->num_tags + added_tags +
                                 ((node->num_submatches > 0) ? 2 : 0);
            }
            regset = (int *)tre_stack_pop_voidptr(stack);
            tag_left = tre_stack_pop_int(stack);
            tag_right = tre_stack_pop_int(stack);

            /* Add tags after both children, the left child gets a smaller
               tag than the right child.  This guarantees that we prefer
               the left child over the right child. */
            /* XXX - This is not always necessary (if the children have
               tags which must be seen for every match of that child). */
            /* XXX - Check if this is the only place where tre_add_tag_right
               is used.	 If so, use tre_add_tag_left (putting the tag before
               the child as opposed after the child) and throw away
               tre_add_tag_right. */
            if (node->num_submatches > 0) {
                if (!first_pass) {
                    status = tre_add_tag_right(mem, left, tag_left);
                    tnfa->tag_directions[tag_left] = TRE_TAG_MAXIMIZE;
                    if (status == REG_OK)
                        status = tre_add_tag_right(mem, right, tag_right);
                    tnfa->tag_directions[tag_right] = TRE_TAG_MAXIMIZE;
                }
                num_tags += 2;
            }
            direction = TRE_TAG_MAXIMIZE;
            break;
        }

        default:
            assert(0);
            break;

        } /* end switch(symbol) */
    } /* end while(tre_stack_num_objects(stack) > bottom) */

    if (!first_pass)
        tre_purge_regset(regset, tnfa, tag);

    if (!first_pass && minimal_tag >= 0) {
        int i;
        for (i = 0; tnfa->minimal_tags[i] >= 0; i++)
            ;
        tnfa->minimal_tags[i] = tag;
        tnfa->minimal_tags[i + 1] = minimal_tag;
        tnfa->minimal_tags[i + 2] = -1;
        minimal_tag = -1;
        num_minimals++;
    }

    assert(tree->num_tags == num_tags);
    tnfa->end_tag = num_tags;
    tnfa->num_tags = num_tags;
    tnfa->num_minimals = num_minimals;
    musl_regex_xfree(orig_regset);
    musl_regex_xfree(parents);
    musl_regex_xfree(saved_states);
    return status;
}

/*
  AST to TNFA compilation routines.
*/

typedef enum {
    COPY_RECURSE,
    COPY_SET_RESULT_PTR
} tre_copyast_symbol_t;

/* Flags for tre_copy_ast(). */
#define MUSL_REGEX_COPY_REMOVE_TAGS 1
#define MUSL_REGEX_COPY_MAXIMIZE_FIRST_TAG 2

inline static reg_errcode_t tre_copy_ast(
    tre_mem_t mem,
    tre_stack_t *stack,
    tre_ast_node_t *ast,
    int flags,
    int *pos_add,
    tre_tag_direction_t *tag_directions,
    tre_ast_node_t **copy,
    int *max_pos) {
    reg_errcode_t status = REG_OK;
    int bottom = tre_stack_num_objects(stack);
    int num_copied = 0;
    int first_tag = 1;
    tre_ast_node_t **result = copy;
    tre_copyast_symbol_t symbol;

    MUSL_REGEX_STACK_PUSH(stack, voidptr, ast);
    MUSL_REGEX_STACK_PUSH(stack, int, COPY_RECURSE);

    while (status == REG_OK && tre_stack_num_objects(stack) > bottom) {
        tre_ast_node_t *node;
        if (status != REG_OK)
            break;

        symbol = (tre_copyast_symbol_t)tre_stack_pop_int(stack);
        switch (symbol) {
        case COPY_SET_RESULT_PTR:
            result = (tre_ast_node_t **)tre_stack_pop_voidptr(stack);
            break;
        case COPY_RECURSE:
            node = (tre_ast_node_t *)tre_stack_pop_voidptr(stack);
            switch (node->type) {
            case LITERAL: {
                tre_literal_t *lit = (tre_literal_t *)node->obj;
                int pos = lit->position;
                int min = lit->code_min;
                int max = lit->code_max;
                if (!MUSL_REGEX_IS_SPECIAL(lit) || MUSL_REGEX_IS_BACKREF(lit)) {
                    /* XXX - e.g. [ab] has only one position but two
                       nodes, so we are creating holes in the state space
                       here.  Not fatal, just wastes memory. */
                    pos += *pos_add;
                    num_copied++;
                } else if (MUSL_REGEX_IS_TAG(lit) && (flags & MUSL_REGEX_COPY_REMOVE_TAGS)) {
                    /* Change this tag to empty. */
                    min = MUSL_REGEX_EMPTY;
                    max = pos = -1;
                } else if (
                    MUSL_REGEX_IS_TAG(lit) && (flags & MUSL_REGEX_COPY_MAXIMIZE_FIRST_TAG) &&
                    first_tag) {
                    /* Maximize the first tag. */
                    tag_directions[max] = TRE_TAG_MAXIMIZE;
                    first_tag = 0;
                }
                *result = tre_ast_new_literal(mem, min, max, pos);
                if (*result == NULL)
                    status = REG_ESPACE;
                else {
                    tre_literal_t *p = (tre_literal_t *)(*result)->obj;
                    p->class1 = lit->class1;
                    p->neg_classes = lit->neg_classes;
                }

                if (pos > *max_pos)
                    *max_pos = pos;
                break;
            }
            case UNION: {
                tre_union_t *uni = (tre_union_t *)node->obj;
                tre_union_t *tmp;
                *result = tre_ast_new_union(mem, uni->left, uni->right);
                if (*result == NULL) {
                    status = REG_ESPACE;
                    break;
                }
                tmp = (tre_union_t *)(*result)->obj;
                result = &tmp->left;
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, uni->right);
                MUSL_REGEX_STACK_PUSHX(stack, int, COPY_RECURSE);
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, &tmp->right);
                MUSL_REGEX_STACK_PUSHX(stack, int, COPY_SET_RESULT_PTR);
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, uni->left);
                MUSL_REGEX_STACK_PUSHX(stack, int, COPY_RECURSE);
                break;
            }
            case CATENATION: {
                tre_catenation_t *cat = (tre_catenation_t *)node->obj;
                tre_catenation_t *tmp;
                *result = tre_ast_new_catenation(mem, cat->left, cat->right);
                if (*result == NULL) {
                    status = REG_ESPACE;
                    break;
                }
                tmp = (tre_catenation_t *)(*result)->obj;
                tmp->left = NULL;
                tmp->right = NULL;
                result = &tmp->left;

                MUSL_REGEX_STACK_PUSHX(stack, voidptr, cat->right);
                MUSL_REGEX_STACK_PUSHX(stack, int, COPY_RECURSE);
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, &tmp->right);
                MUSL_REGEX_STACK_PUSHX(stack, int, COPY_SET_RESULT_PTR);
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, cat->left);
                MUSL_REGEX_STACK_PUSHX(stack, int, COPY_RECURSE);
                break;
            }
            case ITERATION: {
                tre_iteration_t *iter = (tre_iteration_t *)node->obj;
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, iter->arg);
                MUSL_REGEX_STACK_PUSHX(stack, int, COPY_RECURSE);
                *result = tre_ast_new_iter(mem, iter->arg, iter->min, iter->max, iter->minimal);
                if (*result == NULL) {
                    status = REG_ESPACE;
                    break;
                }
                iter = (tre_iteration_t *)(*result)->obj;
                result = &iter->arg;
                break;
            }
            default:
                assert(0);
                break;
            }
            break;
        }
    }
    *pos_add += num_copied;
    return status;
}

typedef enum {
    EXPAND_RECURSE,
    EXPAND_AFTER_ITER
} tre_expand_ast_symbol_t;

/* Expands each iteration node that has a finite nonzero minimum or maximum
   iteration count to a catenated sequence of copies of the node. */
inline static reg_errcode_t tre_expand_ast(
    tre_mem_t mem,
    tre_stack_t *stack,
    tre_ast_node_t *ast,
    int *position,
    tre_tag_direction_t *tag_directions) {
    reg_errcode_t status = REG_OK;
    int bottom = tre_stack_num_objects(stack);
    int pos_add = 0;
    int pos_add_total = 0;
    int max_pos = 0;
    int iter_depth = 0;

    MUSL_REGEX_STACK_PUSHR(stack, voidptr, ast);
    MUSL_REGEX_STACK_PUSHR(stack, int, EXPAND_RECURSE);
    while (status == REG_OK && tre_stack_num_objects(stack) > bottom) {
        tre_ast_node_t *node;
        tre_expand_ast_symbol_t symbol;

        if (status != REG_OK)
            break;

        symbol = (tre_expand_ast_symbol_t)tre_stack_pop_int(stack);
        node = (tre_ast_node_t *)tre_stack_pop_voidptr(stack);
        switch (symbol) {
        case EXPAND_RECURSE:
            switch (node->type) {
            case LITERAL: {
                tre_literal_t *lit = (tre_literal_t *)node->obj;
                if (!MUSL_REGEX_IS_SPECIAL(lit) || MUSL_REGEX_IS_BACKREF(lit)) {
                    lit->position += pos_add;
                    if (lit->position > max_pos)
                        max_pos = lit->position;
                }
                break;
            }
            case UNION: {
                tre_union_t *uni = (tre_union_t *)node->obj;
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, uni->right);
                MUSL_REGEX_STACK_PUSHX(stack, int, EXPAND_RECURSE);
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, uni->left);
                MUSL_REGEX_STACK_PUSHX(stack, int, EXPAND_RECURSE);
                break;
            }
            case CATENATION: {
                tre_catenation_t *cat = (tre_catenation_t *)node->obj;
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, cat->right);
                MUSL_REGEX_STACK_PUSHX(stack, int, EXPAND_RECURSE);
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, cat->left);
                MUSL_REGEX_STACK_PUSHX(stack, int, EXPAND_RECURSE);
                break;
            }
            case ITERATION: {
                tre_iteration_t *iter = (tre_iteration_t *)node->obj;
                MUSL_REGEX_STACK_PUSHX(stack, int, pos_add);
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, node);
                MUSL_REGEX_STACK_PUSHX(stack, int, EXPAND_AFTER_ITER);
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, iter->arg);
                MUSL_REGEX_STACK_PUSHX(stack, int, EXPAND_RECURSE);
                /* If we are going to expand this node at EXPAND_AFTER_ITER
                   then don't increase the `pos' fields of the nodes now, it
                   will get done when expanding. */
                if (iter->min > 1 || iter->max > 1)
                    pos_add = 0;
                iter_depth++;
                break;
            }
            default:
                assert(0);
                break;
            }
            break;
        case EXPAND_AFTER_ITER: {
            tre_iteration_t *iter = (tre_iteration_t *)node->obj;
            int pos_add_last;
            pos_add = tre_stack_pop_int(stack);
            pos_add_last = pos_add;
            if (iter->min > 1 || iter->max > 1) {
                tre_ast_node_t *seq1 = NULL, *seq2 = NULL;
                int j;
                int pos_add_save = pos_add;

                /* Create a catenated sequence of copies of the node. */
                for (j = 0; j < iter->min; j++) {
                    tre_ast_node_t *copy;
                    /* Remove tags from all but the last copy. */
                    int flags =
                        ((j + 1 < iter->min) ? MUSL_REGEX_COPY_REMOVE_TAGS
                                             : MUSL_REGEX_COPY_MAXIMIZE_FIRST_TAG);
                    pos_add_save = pos_add;
                    status = tre_copy_ast(
                        mem, stack, iter->arg, flags, &pos_add, tag_directions, &copy, &max_pos);
                    if (status != REG_OK)
                        return status;
                    if (seq1 != NULL)
                        seq1 = tre_ast_new_catenation(mem, seq1, copy);
                    else
                        seq1 = copy;
                    if (seq1 == NULL)
                        return REG_ESPACE;
                }

                if (iter->max == -1) {
                    /* No upper limit. */
                    pos_add_save = pos_add;
                    status =
                        tre_copy_ast(mem, stack, iter->arg, 0, &pos_add, NULL, &seq2, &max_pos);
                    if (status != REG_OK)
                        return status;
                    seq2 = tre_ast_new_iter(mem, seq2, 0, -1, 0);
                    if (seq2 == NULL)
                        return REG_ESPACE;
                } else {
                    for (j = iter->min; j < iter->max; j++) {
                        tre_ast_node_t *tmp, *copy;
                        pos_add_save = pos_add;
                        status =
                            tre_copy_ast(mem, stack, iter->arg, 0, &pos_add, NULL, &copy, &max_pos);
                        if (status != REG_OK)
                            return status;
                        if (seq2 != NULL)
                            seq2 = tre_ast_new_catenation(mem, copy, seq2);
                        else
                            seq2 = copy;
                        if (seq2 == NULL)
                            return REG_ESPACE;
                        tmp = tre_ast_new_literal(mem, MUSL_REGEX_EMPTY, -1, -1);
                        if (tmp == NULL)
                            return REG_ESPACE;
                        seq2 = tre_ast_new_union(mem, tmp, seq2);
                        if (seq2 == NULL)
                            return REG_ESPACE;
                    }
                }

                pos_add = pos_add_save;
                if (seq1 == NULL)
                    seq1 = seq2;
                else if (seq2 != NULL)
                    seq1 = tre_ast_new_catenation(mem, seq1, seq2);
                if (seq1 == NULL)
                    return REG_ESPACE;
                node->obj = seq1->obj;
                node->type = seq1->type;
            }

            iter_depth--;
            pos_add_total += pos_add - pos_add_last;
            if (iter_depth == 0)
                pos_add = pos_add_total;

            break;
        }
        default:
            assert(0);
            break;
        }
    }

    *position += pos_add_total;

    /* `max_pos' should never be larger than `*position' if the above
       code works, but just an extra safeguard let's make sure
       `*position' is set large enough so enough memory will be
       allocated for the transition table. */
    if (max_pos > *position)
        *position = max_pos;

    return status;
}

inline static tre_pos_and_tags_t *tre_set_empty(tre_mem_t mem) {
    tre_pos_and_tags_t *new_set;

    new_set = (tre_pos_and_tags_t *)tre_mem_calloc(mem, sizeof(*new_set));
    if (new_set == NULL)
        return NULL;

    new_set[0].position = -1;
    new_set[0].code_min = -1;
    new_set[0].code_max = -1;

    return new_set;
}

inline static tre_pos_and_tags_t *tre_set_one(
    tre_mem_t mem,
    int position,
    int code_min,
    int code_max,
    tre_ctype_t class1,
    tre_ctype_t *neg_classes,
    int backref) {
    tre_pos_and_tags_t *new_set;

    new_set = (tre_pos_and_tags_t *)tre_mem_calloc(mem, sizeof(*new_set) * 2);
    if (new_set == NULL)
        return NULL;

    new_set[0].position = position;
    new_set[0].code_min = code_min;
    new_set[0].code_max = code_max;
    new_set[0].class1 = class1;
    new_set[0].neg_classes = neg_classes;
    new_set[0].backref = backref;
    new_set[1].position = -1;
    new_set[1].code_min = -1;
    new_set[1].code_max = -1;

    return new_set;
}

inline static tre_pos_and_tags_t *tre_set_union(
    tre_mem_t mem, tre_pos_and_tags_t *set1, tre_pos_and_tags_t *set2, int *tags, int assertions) {
    int s1, s2, i, j;
    tre_pos_and_tags_t *new_set;
    int *new_tags;
    int num_tags;

    for (num_tags = 0; tags != NULL && tags[num_tags] >= 0; num_tags++)
        ;
    for (s1 = 0; set1[s1].position >= 0; s1++)
        ;
    for (s2 = 0; set2[s2].position >= 0; s2++)
        ;
    new_set = (tre_pos_and_tags_t *)tre_mem_calloc(mem, sizeof(*new_set) * (s1 + s2 + 1));
    if (!new_set)
        return NULL;

    for (s1 = 0; set1[s1].position >= 0; s1++) {
        new_set[s1].position = set1[s1].position;
        new_set[s1].code_min = set1[s1].code_min;
        new_set[s1].code_max = set1[s1].code_max;
        new_set[s1].assertions = set1[s1].assertions | assertions;
        new_set[s1].class1 = set1[s1].class1;
        new_set[s1].neg_classes = set1[s1].neg_classes;
        new_set[s1].backref = set1[s1].backref;
        if (set1[s1].tags == NULL && tags == NULL)
            new_set[s1].tags = NULL;
        else {
            for (i = 0; set1[s1].tags != NULL && set1[s1].tags[i] >= 0; i++)
                ;
            new_tags = (int *)tre_mem_alloc(mem, (sizeof(*new_tags) * (i + num_tags + 1)));
            if (new_tags == NULL)
                return NULL;
            for (j = 0; j < i; j++)
                new_tags[j] = set1[s1].tags[j];
            for (i = 0; i < num_tags; i++)
                new_tags[j + i] = tags[i];
            new_tags[j + i] = -1;
            new_set[s1].tags = new_tags;
        }
    }

    for (s2 = 0; set2[s2].position >= 0; s2++) {
        new_set[s1 + s2].position = set2[s2].position;
        new_set[s1 + s2].code_min = set2[s2].code_min;
        new_set[s1 + s2].code_max = set2[s2].code_max;
        /* XXX - why not | assertions here as well? */
        new_set[s1 + s2].assertions = set2[s2].assertions;
        new_set[s1 + s2].class1 = set2[s2].class1;
        new_set[s1 + s2].neg_classes = set2[s2].neg_classes;
        new_set[s1 + s2].backref = set2[s2].backref;
        if (set2[s2].tags == NULL)
            new_set[s1 + s2].tags = NULL;
        else {
            for (i = 0; set2[s2].tags[i] >= 0; i++)
                ;
            new_tags = (int *)tre_mem_alloc(mem, sizeof(*new_tags) * (i + 1));
            if (new_tags == NULL)
                return NULL;
            for (j = 0; j < i; j++)
                new_tags[j] = set2[s2].tags[j];
            new_tags[j] = -1;
            new_set[s1 + s2].tags = new_tags;
        }
    }
    new_set[s1 + s2].position = -1;
    return new_set;
}

/* Finds the empty path through `node' which is the one that should be
   taken according to POSIX.2 rules, and adds the tags on that path to
   `tags'.   `tags' may be NULL.  If `num_tags_seen' is not NULL, it is
   set to the number of tags seen on the path. */
inline static reg_errcode_t tre_match_empty(
    tre_stack_t *stack, tre_ast_node_t *node, int *tags, int *assertions, int *num_tags_seen) {
    tre_literal_t *lit;
    tre_union_t *uni;
    tre_catenation_t *cat;
    tre_iteration_t *iter;
    int i;
    int bottom = tre_stack_num_objects(stack);
    reg_errcode_t status = REG_OK;
    if (num_tags_seen)
        *num_tags_seen = 0;

    status = tre_stack_push_voidptr(stack, node);

    /* Walk through the tree recursively. */
    while (status == REG_OK && tre_stack_num_objects(stack) > bottom) {
        node = (tre_ast_node_t *)tre_stack_pop_voidptr(stack);

        switch (node->type) {
        case LITERAL:
            lit = (tre_literal_t *)node->obj;
            switch (lit->code_min) {
            case MUSL_REGEX_TAG:
                if (lit->code_max >= 0) {
                    if (tags != NULL) {
                        /* Add the tag to `tags'. */
                        for (i = 0; tags[i] >= 0; i++)
                            if (tags[i] == lit->code_max)
                                break;
                        if (tags[i] < 0) {
                            tags[i] = lit->code_max;
                            tags[i + 1] = -1;
                        }
                    }
                    if (num_tags_seen)
                        (*num_tags_seen)++;
                }
                break;
            case MUSL_REGEX_ASSERTION:
                assert(lit->code_max >= 1 || lit->code_max <= MUSL_REGEX_ASSERT_LAST);
                if (assertions != NULL)
                    *assertions |= lit->code_max;
                break;
            case MUSL_REGEX_EMPTY:
                break;
            default:
                assert(0);
                break;
            }
            break;

        case UNION:
            /* Subexpressions starting earlier take priority over ones
               starting later, so we prefer the left subexpression over the
               right subexpression. */
            uni = (tre_union_t *)node->obj;
            if (uni->left->nullable)
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, uni->left)
            else if (uni->right->nullable)
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, uni->right)
            else
                assert(0);
            break;

        case CATENATION:
            /* The path must go through both children. */
            cat = (tre_catenation_t *)node->obj;
            assert(cat->left->nullable);
            assert(cat->right->nullable);
            MUSL_REGEX_STACK_PUSHX(stack, voidptr, cat->left);
            MUSL_REGEX_STACK_PUSHX(stack, voidptr, cat->right);
            break;

        case ITERATION:
            /* A match with an empty string is preferred over no match at
               all, so we go through the argument if possible. */
            iter = (tre_iteration_t *)node->obj;
            if (iter->arg->nullable)
                MUSL_REGEX_STACK_PUSHX(stack, voidptr, iter->arg);
            break;

        default:
            assert(0);
            break;
        }
    }

    return status;
}

typedef enum {
    NFL_RECURSE,
    NFL_POST_UNION,
    NFL_POST_CATENATION,
    NFL_POST_ITERATION
} tre_nfl_stack_symbol_t;

/* Computes and fills in the fields `nullable', `firstpos', and `lastpos' for
   the nodes of the AST `tree'. */
inline static reg_errcode_t
tre_compute_nfl(tre_mem_t mem, tre_stack_t *stack, tre_ast_node_t *tree) {
    int bottom = tre_stack_num_objects(stack);

    MUSL_REGEX_STACK_PUSHR(stack, voidptr, tree);
    MUSL_REGEX_STACK_PUSHR(stack, int, NFL_RECURSE);

    while (tre_stack_num_objects(stack) > bottom) {
        tre_nfl_stack_symbol_t symbol;
        tre_ast_node_t *node;

        symbol = (tre_nfl_stack_symbol_t)tre_stack_pop_int(stack);
        node = (tre_ast_node_t *)tre_stack_pop_voidptr(stack);
        switch (symbol) {
        case NFL_RECURSE:
            switch (node->type) {
            case LITERAL: {
                tre_literal_t *lit = (tre_literal_t *)node->obj;
                if (MUSL_REGEX_IS_BACKREF(lit)) {
                    /* Back references: nullable = false, firstpos = {i},
                       lastpos = {i}. */
                    node->nullable = 0;
                    node->firstpos = tre_set_one(mem, lit->position, 0, TRE_CHAR_MAX, 0, NULL, -1);
                    if (!node->firstpos)
                        return REG_ESPACE;
                    node->lastpos = tre_set_one(
                        mem, lit->position, 0, TRE_CHAR_MAX, 0, NULL, (int)lit->code_max);
                    if (!node->lastpos)
                        return REG_ESPACE;
                } else if (lit->code_min < 0) {
                    /* Tags, empty strings, params, and zero width assertions:
                       nullable = true, firstpos = {}, and lastpos = {}. */
                    node->nullable = 1;
                    node->firstpos = tre_set_empty(mem);
                    if (!node->firstpos)
                        return REG_ESPACE;
                    node->lastpos = tre_set_empty(mem);
                    if (!node->lastpos)
                        return REG_ESPACE;
                } else {
                    /* Literal at position i: nullable = false, firstpos = {i},
                       lastpos = {i}. */
                    node->nullable = 0;
                    node->firstpos = tre_set_one(
                        mem, lit->position, (int)lit->code_min, (int)lit->code_max, 0, NULL, -1);
                    if (!node->firstpos)
                        return REG_ESPACE;
                    node->lastpos = tre_set_one(
                        mem,
                        lit->position,
                        (int)lit->code_min,
                        (int)lit->code_max,
                        lit->class1,
                        lit->neg_classes,
                        -1);
                    if (!node->lastpos)
                        return REG_ESPACE;
                }
                break;
            }

            case UNION:
                /* Compute the attributes for the two subtrees, and after that
                   for this node. */
                MUSL_REGEX_STACK_PUSHR(stack, voidptr, node);
                MUSL_REGEX_STACK_PUSHR(stack, int, NFL_POST_UNION);
                MUSL_REGEX_STACK_PUSHR(stack, voidptr, ((tre_union_t *)node->obj)->right);
                MUSL_REGEX_STACK_PUSHR(stack, int, NFL_RECURSE);
                MUSL_REGEX_STACK_PUSHR(stack, voidptr, ((tre_union_t *)node->obj)->left);
                MUSL_REGEX_STACK_PUSHR(stack, int, NFL_RECURSE);
                break;

            case CATENATION:
                /* Compute the attributes for the two subtrees, and after that
                   for this node. */
                MUSL_REGEX_STACK_PUSHR(stack, voidptr, node);
                MUSL_REGEX_STACK_PUSHR(stack, int, NFL_POST_CATENATION);
                MUSL_REGEX_STACK_PUSHR(stack, voidptr, ((tre_catenation_t *)node->obj)->right);
                MUSL_REGEX_STACK_PUSHR(stack, int, NFL_RECURSE);
                MUSL_REGEX_STACK_PUSHR(stack, voidptr, ((tre_catenation_t *)node->obj)->left);
                MUSL_REGEX_STACK_PUSHR(stack, int, NFL_RECURSE);
                break;

            case ITERATION:
                /* Compute the attributes for the subtree, and after that for
                   this node. */
                MUSL_REGEX_STACK_PUSHR(stack, voidptr, node);
                MUSL_REGEX_STACK_PUSHR(stack, int, NFL_POST_ITERATION);
                MUSL_REGEX_STACK_PUSHR(stack, voidptr, ((tre_iteration_t *)node->obj)->arg);
                MUSL_REGEX_STACK_PUSHR(stack, int, NFL_RECURSE);
                break;
            }
            break; /* end case: NFL_RECURSE */

        case NFL_POST_UNION: {
            tre_union_t *uni = (tre_union_t *)node->obj;
            node->nullable = uni->left->nullable || uni->right->nullable;
            node->firstpos = tre_set_union(mem, uni->left->firstpos, uni->right->firstpos, NULL, 0);
            if (!node->firstpos)
                return REG_ESPACE;
            node->lastpos = tre_set_union(mem, uni->left->lastpos, uni->right->lastpos, NULL, 0);
            if (!node->lastpos)
                return REG_ESPACE;
            break;
        }

        case NFL_POST_ITERATION: {
            tre_iteration_t *iter = (tre_iteration_t *)node->obj;

            if (iter->min == 0 || iter->arg->nullable)
                node->nullable = 1;
            else
                node->nullable = 0;
            node->firstpos = iter->arg->firstpos;
            node->lastpos = iter->arg->lastpos;
            break;
        }

        case NFL_POST_CATENATION: {
            int num_tags, *tags, assertions;
            reg_errcode_t status;
            tre_catenation_t *cat = (tre_catenation_t *)node->obj;
            node->nullable = cat->left->nullable && cat->right->nullable;

            /* Compute firstpos. */
            if (cat->left->nullable) {
                /* The left side matches the empty string.  Make a first pass
                   with tre_match_empty() to get the number of tags and
                   parameters. */
                status = tre_match_empty(stack, cat->left, NULL, NULL, &num_tags);
                if (status != REG_OK)
                    return status;
                /* Allocate arrays for the tags and parameters. */
                tags = (int *)musl_regex_xmalloc(sizeof(*tags) * (num_tags + 1));
                if (!tags)
                    return REG_ESPACE;
                tags[0] = -1;
                assertions = 0;
                /* Second pass with tre_mach_empty() to get the list of
                   tags and parameters. */
                status = tre_match_empty(stack, cat->left, tags, &assertions, NULL);
                if (status != REG_OK) {
                    musl_regex_xfree(tags);
                    return status;
                }
                node->firstpos =
                    tre_set_union(mem, cat->right->firstpos, cat->left->firstpos, tags, assertions);
                musl_regex_xfree(tags);
                if (!node->firstpos)
                    return REG_ESPACE;
            } else {
                node->firstpos = cat->left->firstpos;
            }

            /* Compute lastpos. */
            if (cat->right->nullable) {
                /* The right side matches the empty string.  Make a first pass
                   with tre_match_empty() to get the number of tags and
                   parameters. */
                status = tre_match_empty(stack, cat->right, NULL, NULL, &num_tags);
                if (status != REG_OK)
                    return status;
                /* Allocate arrays for the tags and parameters. */
                tags = (int *)musl_regex_xmalloc(sizeof(int) * (num_tags + 1));
                if (!tags)
                    return REG_ESPACE;
                tags[0] = -1;
                assertions = 0;
                /* Second pass with tre_mach_empty() to get the list of
                   tags and parameters. */
                status = tre_match_empty(stack, cat->right, tags, &assertions, NULL);
                if (status != REG_OK) {
                    musl_regex_xfree(tags);
                    return status;
                }
                node->lastpos =
                    tre_set_union(mem, cat->left->lastpos, cat->right->lastpos, tags, assertions);
                musl_regex_xfree(tags);
                if (!node->lastpos)
                    return REG_ESPACE;
            } else {
                node->lastpos = cat->right->lastpos;
            }
            break;
        }

        default:
            assert(0);
            break;
        }
    }

    return REG_OK;
}

/* Adds a transition from each position in `p1' to each position in `p2'. */
inline static reg_errcode_t tre_make_trans(
    tre_pos_and_tags_t *p1,
    tre_pos_and_tags_t *p2,
    tre_tnfa_transition_t *transitions,
    int *counts,
    int *offs) {
    tre_pos_and_tags_t *orig_p2 = p2;
    tre_tnfa_transition_t *trans;
    int i, j, k, l, dup, prev_p2_pos;

    if (transitions != NULL)
        while (p1->position >= 0) {
            p2 = orig_p2;
            prev_p2_pos = -1;
            while (p2->position >= 0) {
                /* Optimization: if this position was already handled, skip it. */
                if (p2->position == prev_p2_pos) {
                    p2++;
                    continue;
                }
                prev_p2_pos = p2->position;
                /* Set `trans' to point to the next unused transition from
                   position `p1->position'. */
                trans = transitions + offs[p1->position];
                while (trans->state != NULL) {
#if 0
		/* If we find a previous transition from `p1->position' to
		   `p2->position', it is overwritten.  This can happen only
		   if there are nested loops in the regexp, like in "((a)*)*".
		   In POSIX.2 repetition using the outer loop is always
		   preferred over using the inner loop.	 Therefore the
		   transition for the inner loop is useless and can be thrown
		   away. */
		/* XXX - The same position is used for all nodes in a bracket
		   expression, so this optimization cannot be used (it will
		   break bracket expressions) unless I figure out a way to
		   detect it here. */
		if (trans->state_id == p2->position)
		  {
		    break;
		  }
#endif
                    trans++;
                }

                if (trans->state == NULL)
                    (trans + 1)->state = NULL;
                /* Use the character ranges, assertions, etc. from `p1' for
                   the transition from `p1' to `p2'. */
                trans->code_min = p1->code_min;
                trans->code_max = p1->code_max;
                trans->state = transitions + offs[p2->position];
                trans->state_id = p2->position;
                trans->assertions =
                    p1->assertions | p2->assertions |
                    (p1->class1 ? MUSL_REGEX_ASSERT_CHAR_CLASS : 0) |
                    (p1->neg_classes != NULL ? MUSL_REGEX_ASSERT_CHAR_CLASS_NEG : 0);
                if (p1->backref >= 0) {
                    assert((trans->assertions & MUSL_REGEX_ASSERT_CHAR_CLASS) == 0);
                    assert(p2->backref < 0);
                    trans->u.backref = p1->backref;
                    trans->assertions |= MUSL_REGEX_ASSERT_MUSL_REGEX_BACKREF;
                } else
                    trans->u.class1 = p1->class1;
                if (p1->neg_classes != NULL) {
                    for (i = 0; p1->neg_classes[i] != (tre_ctype_t)0; i++)
                        ;
                    trans->neg_classes =
                        (tre_ctype_t *)musl_regex_xmalloc(sizeof(*trans->neg_classes) * (i + 1));
                    if (trans->neg_classes == NULL)
                        return REG_ESPACE;
                    for (i = 0; p1->neg_classes[i] != (tre_ctype_t)0; i++)
                        trans->neg_classes[i] = p1->neg_classes[i];
                    trans->neg_classes[i] = (tre_ctype_t)0;
                } else
                    trans->neg_classes = NULL;

                /* Find out how many tags this transition has. */
                i = 0;
                if (p1->tags != NULL)
                    while (p1->tags[i] >= 0)
                        i++;
                j = 0;
                if (p2->tags != NULL)
                    while (p2->tags[j] >= 0)
                        j++;

                /* If we are overwriting a transition, free the old tag array. */
                if (trans->tags != NULL)
                    musl_regex_xfree(trans->tags);
                trans->tags = NULL;

                /* If there were any tags, allocate an array and fill it. */
                if (i + j > 0) {
                    trans->tags = (int *)musl_regex_xmalloc(sizeof(*trans->tags) * (i + j + 1));
                    if (!trans->tags)
                        return REG_ESPACE;
                    i = 0;
                    if (p1->tags != NULL)
                        while (p1->tags[i] >= 0) {
                            trans->tags[i] = p1->tags[i];
                            i++;
                        }
                    l = i;
                    j = 0;
                    if (p2->tags != NULL)
                        while (p2->tags[j] >= 0) {
                            /* Don't add duplicates. */
                            dup = 0;
                            for (k = 0; k < i; k++)
                                if (trans->tags[k] == p2->tags[j]) {
                                    dup = 1;
                                    break;
                                }
                            if (!dup)
                                trans->tags[l++] = p2->tags[j];
                            j++;
                        }
                    trans->tags[l] = -1;
                }

                p2++;
            }
            p1++;
        }
    else
        /* Compute a maximum limit for the number of transitions leaving
           from each state. */
        while (p1->position >= 0) {
            p2 = orig_p2;
            while (p2->position >= 0) {
                counts[p1->position]++;
                p2++;
            }
            p1++;
        }
    return REG_OK;
}

/* Converts the syntax tree to a TNFA.	All the transitions in the TNFA are
   labelled with one character range (there are no transitions on empty
   strings).  The TNFA takes O(n^2) space in the worst case, `n' is size of
   the regexp. */
inline static reg_errcode_t
tre_ast_to_tnfa(tre_ast_node_t *node, tre_tnfa_transition_t *transitions, int *counts, int *offs) {
    tre_union_t *uni;
    tre_catenation_t *cat;
    tre_iteration_t *iter;
    reg_errcode_t errcode = REG_OK;

    /* XXX - recurse using a stack!. */
    switch (node->type) {
    case LITERAL:
        break;
    case UNION:
        uni = (tre_union_t *)node->obj;
        errcode = tre_ast_to_tnfa(uni->left, transitions, counts, offs);
        if (errcode != REG_OK)
            return errcode;
        errcode = tre_ast_to_tnfa(uni->right, transitions, counts, offs);
        break;

    case CATENATION:
        cat = (tre_catenation_t *)node->obj;
        /* Add a transition from each position in cat->left->lastpos
           to each position in cat->right->firstpos. */
        errcode =
            tre_make_trans(cat->left->lastpos, cat->right->firstpos, transitions, counts, offs);
        if (errcode != REG_OK)
            return errcode;
        errcode = tre_ast_to_tnfa(cat->left, transitions, counts, offs);
        if (errcode != REG_OK)
            return errcode;
        errcode = tre_ast_to_tnfa(cat->right, transitions, counts, offs);
        break;

    case ITERATION:
        iter = (tre_iteration_t *)node->obj;
        assert(iter->max == -1 || iter->max == 1);

        if (iter->max == -1) {
            assert(iter->min == 0 || iter->min == 1);
            /* Add a transition from each last position in the iterated
               expression to each first position. */
            errcode =
                tre_make_trans(iter->arg->lastpos, iter->arg->firstpos, transitions, counts, offs);
            if (errcode != REG_OK)
                return errcode;
        }
        errcode = tre_ast_to_tnfa(iter->arg, transitions, counts, offs);
        break;
    }
    return errcode;
}

#define MUSL_REGEX_ERROR_EXIT(err)                                                                 \
    do {                                                                                           \
        errcode = err;                                                                             \
        if (/*CONSTCOND*/ 1)                                                                       \
            goto error_exit;                                                                       \
    } while (/*CONSTCOND*/ 0)

inline int regcomp(regex_t *preg, const char *regex, int cflags) {
    tre_stack_t *stack;
    tre_ast_node_t *tree, *tmp_ast_l, *tmp_ast_r;
    tre_pos_and_tags_t *p;
    int *counts = NULL, *offs = NULL;
    int i, add = 0;
    tre_tnfa_transition_t *transitions, *initial;
    tre_tnfa_t *tnfa = NULL;
    tre_submatch_data_t *submatch_data;
    tre_tag_direction_t *tag_directions = NULL;
    reg_errcode_t errcode;
    tre_mem_t mem;

    /* Parse context. */
    tre_parse_ctx_t parse_ctx;

    /* Allocate a stack used throughout the compilation process for various
       purposes. */
    stack = tre_stack_new(512, 1024000, 128);
    if (!stack)
        return REG_ESPACE;
    /* Allocate a fast memory allocator. */
    mem = tre_mem_new();
    if (!mem) {
        tre_stack_destroy(stack);
        return REG_ESPACE;
    }

    /* Parse the regexp. */
    memset(&parse_ctx, 0, sizeof(parse_ctx));
    parse_ctx.mem = mem;
    parse_ctx.stack = stack;
    parse_ctx.start = regex;
    parse_ctx.cflags = cflags;
    parse_ctx.max_backref = -1;
    errcode = tre_parse(&parse_ctx);
    if (errcode != REG_OK)
        MUSL_REGEX_ERROR_EXIT(errcode);
    preg->re_nsub = parse_ctx.submatch_id - 1;
    tree = parse_ctx.n;

    /* Referring to nonexistent subexpressions is illegal. */
    if (parse_ctx.max_backref > (int)preg->re_nsub)
        MUSL_REGEX_ERROR_EXIT(REG_ESUBREG);

    /* Allocate the TNFA struct. */
    tnfa = (tre_tnfa_t *)musl_regex_xcalloc(1, sizeof(tre_tnfa_t));
    if (tnfa == NULL)
        MUSL_REGEX_ERROR_EXIT(REG_ESPACE);
    tnfa->have_backrefs = parse_ctx.max_backref >= 0;
    tnfa->have_approx = 0;
    tnfa->num_submatches = parse_ctx.submatch_id;

    /* Set up tags for submatch addressing.  If REG_NOSUB is set and the
       regexp does not have back references, this can be skipped. */
    if (tnfa->have_backrefs || !(cflags & REG_NOSUB)) {
        /* Figure out how many tags we will need. */
        errcode = tre_add_tags(NULL, stack, tree, tnfa);
        if (errcode != REG_OK)
            MUSL_REGEX_ERROR_EXIT(errcode);

        if (tnfa->num_tags > 0) {
            tag_directions = (tre_tag_direction_t *)musl_regex_xmalloc(
                sizeof(*tag_directions) * (tnfa->num_tags + 1));
            if (tag_directions == NULL)
                MUSL_REGEX_ERROR_EXIT(REG_ESPACE);
            tnfa->tag_directions = tag_directions;
            memset(tag_directions, -1, sizeof(*tag_directions) * (tnfa->num_tags + 1));
        }
        tnfa->minimal_tags = (int *)musl_regex_xcalloc(
            (unsigned)tnfa->num_tags * 2 + 1, sizeof(*tnfa->minimal_tags));
        if (tnfa->minimal_tags == NULL)
            MUSL_REGEX_ERROR_EXIT(REG_ESPACE);

        submatch_data = (tre_submatch_data_t *)musl_regex_xcalloc(
            (unsigned)parse_ctx.submatch_id, sizeof(*submatch_data));
        if (submatch_data == NULL)
            MUSL_REGEX_ERROR_EXIT(REG_ESPACE);
        tnfa->submatch_data = submatch_data;

        errcode = tre_add_tags(mem, stack, tree, tnfa);
        if (errcode != REG_OK)
            MUSL_REGEX_ERROR_EXIT(errcode);
    }

    /* Expand iteration nodes. */
    errcode = tre_expand_ast(mem, stack, tree, &parse_ctx.position, tag_directions);
    if (errcode != REG_OK)
        MUSL_REGEX_ERROR_EXIT(errcode);

    /* Add a dummy node for the final state.
       XXX - For certain patterns this dummy node can be optimized away,
             for example "a*" or "ab*".	Figure out a simple way to detect
             this possibility. */
    tmp_ast_l = tree;
    tmp_ast_r = tre_ast_new_literal(mem, 0, 0, parse_ctx.position++);
    if (tmp_ast_r == NULL)
        MUSL_REGEX_ERROR_EXIT(REG_ESPACE);

    tree = tre_ast_new_catenation(mem, tmp_ast_l, tmp_ast_r);
    if (tree == NULL)
        MUSL_REGEX_ERROR_EXIT(REG_ESPACE);

    errcode = tre_compute_nfl(mem, stack, tree);
    if (errcode != REG_OK)
        MUSL_REGEX_ERROR_EXIT(errcode);

    counts = (int *)musl_regex_xmalloc(sizeof(int) * parse_ctx.position);
    if (counts == NULL)
        MUSL_REGEX_ERROR_EXIT(REG_ESPACE);

    offs = (int *)musl_regex_xmalloc(sizeof(int) * parse_ctx.position);
    if (offs == NULL)
        MUSL_REGEX_ERROR_EXIT(REG_ESPACE);

    for (i = 0; i < parse_ctx.position; i++)
        counts[i] = 0;
    tre_ast_to_tnfa(tree, NULL, counts, NULL);

    add = 0;
    for (i = 0; i < parse_ctx.position; i++) {
        offs[i] = add;
        add += counts[i] + 1;
        counts[i] = 0;
    }
    transitions =
        (tre_tnfa_transition_t *)musl_regex_xcalloc((unsigned)add + 1, sizeof(*transitions));
    if (transitions == NULL)
        MUSL_REGEX_ERROR_EXIT(REG_ESPACE);
    tnfa->transitions = transitions;
    tnfa->num_transitions = add;

    errcode = tre_ast_to_tnfa(tree, transitions, counts, offs);
    if (errcode != REG_OK)
        MUSL_REGEX_ERROR_EXIT(errcode);

    tnfa->firstpos_chars = NULL;

    p = tree->firstpos;
    i = 0;
    while (p->position >= 0) {
        i++;
        p++;
    }

    initial =
        (tre_tnfa_transition_t *)musl_regex_xcalloc((unsigned)i + 1, sizeof(tre_tnfa_transition_t));
    if (initial == NULL)
        MUSL_REGEX_ERROR_EXIT(REG_ESPACE);
    tnfa->initial = initial;

    i = 0;
    for (p = tree->firstpos; p->position >= 0; p++) {
        initial[i].state = transitions + offs[p->position];
        initial[i].state_id = p->position;
        initial[i].tags = NULL;
        /* Copy the arrays p->tags, and p->params, they are allocated
           from a tre_mem object. */
        if (p->tags) {
            int j;
            for (j = 0; p->tags[j] >= 0; j++)
                ;
            initial[i].tags = (int *)musl_regex_xmalloc(sizeof(*p->tags) * (j + 1));
            if (!initial[i].tags)
                MUSL_REGEX_ERROR_EXIT(REG_ESPACE);
            memcpy(initial[i].tags, p->tags, sizeof(*p->tags) * (j + 1));
        }
        initial[i].assertions = p->assertions;
        i++;
    }
    initial[i].state = NULL;

    tnfa->num_transitions = add;
    tnfa->final = transitions + offs[tree->lastpos[0].position];
    tnfa->num_states = parse_ctx.position;
    tnfa->cflags = cflags;

    tre_mem_destroy(mem);
    tre_stack_destroy(stack);
    musl_regex_xfree(counts);
    musl_regex_xfree(offs);

    preg->TRE_REGEX_T_FIELD = (void *)tnfa;
    return REG_OK;

error_exit:
    /* Free everything that was allocated and return the error code. */
    tre_mem_destroy(mem);
    if (stack != NULL)
        tre_stack_destroy(stack);
    if (counts != NULL)
        musl_regex_xfree(counts);
    if (offs != NULL)
        musl_regex_xfree(offs);
    preg->TRE_REGEX_T_FIELD = (void *)tnfa;
    regfree(preg);
    return errcode;
}

inline void regfree(regex_t *preg) {
    tre_tnfa_t *tnfa;
    unsigned int i;
    tre_tnfa_transition_t *trans;

    tnfa = (tre_tnfa_t *)preg->TRE_REGEX_T_FIELD;
    if (!tnfa)
        return;

    for (i = 0; i < tnfa->num_transitions; i++)
        if (tnfa->transitions[i].state) {
            if (tnfa->transitions[i].tags)
                musl_regex_xfree(tnfa->transitions[i].tags);
            if (tnfa->transitions[i].neg_classes)
                musl_regex_xfree(tnfa->transitions[i].neg_classes);
        }
    if (tnfa->transitions)
        musl_regex_xfree(tnfa->transitions);

    if (tnfa->initial) {
        for (trans = tnfa->initial; trans->state; trans++) {
            if (trans->tags)
                musl_regex_xfree(trans->tags);
        }
        musl_regex_xfree(tnfa->initial);
    }

    if (tnfa->submatch_data) {
        for (i = 0; i < tnfa->num_submatches; i++)
            if (tnfa->submatch_data[i].parents)
                musl_regex_xfree(tnfa->submatch_data[i].parents);
        musl_regex_xfree(tnfa->submatch_data);
    }

    if (tnfa->tag_directions)
        musl_regex_xfree(tnfa->tag_directions);
    if (tnfa->firstpos_chars)
        musl_regex_xfree(tnfa->firstpos_chars);
    if (tnfa->minimal_tags)
        musl_regex_xfree(tnfa->minimal_tags);
    musl_regex_xfree(tnfa);
}

/*
  regexec.c - TRE POSIX compatible matching functions (and more).

  Copyright (c) 2001-2009 Ville Laurikari <vl@iki.fi>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

inline static void tre_fill_pmatch(
    size_t nmatch,
    regmatch_t pmatch[],
    int cflags,
    const tre_tnfa_t *tnfa,
    regoff_t *tags,
    regoff_t match_eo);

/***********************************************************************
 from tre-match-utils.h
***********************************************************************/

#define MUSL_REGEX_GET_NEXT_WCHAR()                                                                \
    do {                                                                                           \
        prev_c = next_c;                                                                           \
        pos += pos_add_next;                                                                       \
        if ((pos_add_next = mbtowc(&next_c, str_byte, MB_LEN_MAX)) <= 0) {                         \
            if (pos_add_next < 0) {                                                                \
                ret = REG_NOMATCH;                                                                 \
                goto error_exit;                                                                   \
            } else                                                                                 \
                pos_add_next++;                                                                    \
        }                                                                                          \
        str_byte += pos_add_next;                                                                  \
    } while (0)

#define MUSL_REGEX_IS_WORD_CHAR(c) ((c) == L'_' || tre_isalnum(c))

#define MUSL_REGEX_CHECK_ASSERTIONS(assertions)                                                    \
    (((assertions & MUSL_REGEX_ASSERT_AT_BOL) && (pos > 0 || reg_notbol) &&                        \
      (prev_c != L'\n' || !reg_newline)) ||                                                        \
     ((assertions & MUSL_REGEX_ASSERT_AT_EOL) && (next_c != L'\0' || reg_noteol) &&                \
      (next_c != L'\n' || !reg_newline)) ||                                                        \
     ((assertions & MUSL_REGEX_ASSERT_AT_BOW) &&                                                   \
      (MUSL_REGEX_IS_WORD_CHAR(prev_c) || !MUSL_REGEX_IS_WORD_CHAR(next_c))) ||                    \
     ((assertions & MUSL_REGEX_ASSERT_AT_EOW) &&                                                   \
      (!MUSL_REGEX_IS_WORD_CHAR(prev_c) || MUSL_REGEX_IS_WORD_CHAR(next_c))) ||                    \
     ((assertions & MUSL_REGEX_ASSERT_AT_WB) &&                                                    \
      (pos != 0 && next_c != L'\0' &&                                                              \
       MUSL_REGEX_IS_WORD_CHAR(prev_c) == MUSL_REGEX_IS_WORD_CHAR(next_c))) ||                     \
     ((assertions & MUSL_REGEX_ASSERT_AT_WB_NEG) &&                                                \
      (pos == 0 || next_c == L'\0' ||                                                              \
       MUSL_REGEX_IS_WORD_CHAR(prev_c) != MUSL_REGEX_IS_WORD_CHAR(next_c))))

#define MUSL_REGEX_CHECK_CHAR_CLASSES(trans_i, tnfa, eflags)                                       \
    (((trans_i->assertions & MUSL_REGEX_ASSERT_CHAR_CLASS) && !(tnfa->cflags & REG_ICASE) &&       \
      !tre_isctype((tre_cint_t)prev_c, trans_i->u.class1)) ||                                      \
     ((trans_i->assertions & MUSL_REGEX_ASSERT_CHAR_CLASS) && (tnfa->cflags & REG_ICASE) &&        \
      !tre_isctype(tre_tolower((tre_cint_t)prev_c), trans_i->u.class1) &&                          \
      !tre_isctype(tre_toupper((tre_cint_t)prev_c), trans_i->u.class1)) ||                         \
     ((trans_i->assertions & MUSL_REGEX_ASSERT_CHAR_CLASS_NEG) &&                                  \
      tre_neg_char_classes_match(                                                                  \
          trans_i->neg_classes, (tre_cint_t)prev_c, tnfa->cflags & REG_ICASE)))

/* Returns 1 if `t1' wins `t2', 0 otherwise. */
inline static int
tre_tag_order(int num_tags, tre_tag_direction_t *tag_directions, regoff_t *t1, regoff_t *t2) {
    int i;
    for (i = 0; i < num_tags; i++) {
        if (tag_directions[i] == TRE_TAG_MINIMIZE) {
            if (t1[i] < t2[i])
                return 1;
            if (t1[i] > t2[i])
                return 0;
        } else {
            if (t1[i] > t2[i])
                return 1;
            if (t1[i] < t2[i])
                return 0;
        }
    }
    /*  assert(0);*/
    return 0;
}

inline static int tre_neg_char_classes_match(tre_ctype_t *classes, tre_cint_t wc, int icase) {
    while (*classes != (tre_ctype_t)0)
        if ((!icase && tre_isctype(wc, *classes)) ||
            (icase &&
             (tre_isctype(tre_toupper(wc), *classes) || tre_isctype(tre_tolower(wc), *classes))))
            return 1; /* Match. */
        else
            classes++;
    return 0; /* No match. */
}

/***********************************************************************
 from tre-match-parallel.c
***********************************************************************/

/*
  This algorithm searches for matches basically by reading characters
  in the searched string one by one, starting at the beginning.	 All
  matching paths in the TNFA are traversed in parallel.	 When two or
  more paths reach the same state, exactly one is chosen according to
  tag ordering rules; if returning submatches is not required it does
  not matter which path is chosen.

  The worst case time required for finding the leftmost and longest
  match, or determining that there is no match, is always linearly
  dependent on the length of the text being searched.

  This algorithm cannot handle TNFAs with back referencing nodes.
  See `tre-match-backtrack.c'.
*/

typedef struct {
    tre_tnfa_transition_t *state;
    regoff_t *tags;
} tre_tnfa_reach_t;

typedef struct {
    regoff_t pos;
    regoff_t **tags;
} tre_reach_pos_t;

inline static reg_errcode_t tre_tnfa_run_parallel(
    const tre_tnfa_t *tnfa,
    const void *string,
    regoff_t *match_tags,
    int eflags,
    regoff_t *match_end_ofs) {
    /* State variables required by MUSL_REGEX_GET_NEXT_WCHAR. */
    tre_char_t prev_c = 0, next_c = 0;
    const char *str_byte = (const char *)string;
    regoff_t pos = -1;
    regoff_t pos_add_next = 1;
    int reg_notbol = eflags & REG_NOTBOL;
    int reg_noteol = eflags & REG_NOTEOL;
    int reg_newline = tnfa->cflags & REG_NEWLINE;
    reg_errcode_t ret;

    char *buf;
    tre_tnfa_transition_t *trans_i;
    tre_tnfa_reach_t *reach, *reach_next, *reach_i, *reach_next_i;
    tre_reach_pos_t *reach_pos;
    int *tag_i;
    int num_tags, i;

    regoff_t match_eo = -1; /* end offset of match (-1 if no match found yet) */
    int new_match = 0;
    regoff_t *tmp_tags = NULL;
    regoff_t *tmp_iptr;

    if (!match_tags)
        num_tags = 0;
    else
        num_tags = tnfa->num_tags;

    /* Allocate memory for temporary data required for matching.	This needs to
       be done for every matching operation to be thread safe.  This allocates
       everything in a single large block with calloc(). */
    {
        size_t tbytes, rbytes, pbytes, xbytes, total_bytes;
        char *tmp_buf;

        /* Ensure that tbytes and xbytes*num_states cannot overflow, and that
         * they don't contribute more than 1/8 of SIZE_MAX to total_bytes. */
        if ((size_t)num_tags > SIZE_MAX / (8 * sizeof(regoff_t) * tnfa->num_states))
            return REG_ESPACE;

        /* Likewise check rbytes. */
        if ((size_t)(tnfa->num_states + 1) > SIZE_MAX / (8 * sizeof(*reach_next)))
            return REG_ESPACE;

        /* Likewise check pbytes. */
        if ((size_t)tnfa->num_states > SIZE_MAX / (8 * sizeof(*reach_pos)))
            return REG_ESPACE;

        /* Compute the length of the block we need. */
        tbytes = sizeof(*tmp_tags) * num_tags;
        rbytes = sizeof(*reach_next) * (tnfa->num_states + 1);
        pbytes = sizeof(*reach_pos) * tnfa->num_states;
        xbytes = sizeof(regoff_t) * num_tags;
        total_bytes = (sizeof(intptr_t) - 1) * 4 /* for alignment paddings */
                      + (rbytes + xbytes * tnfa->num_states) * 2 + tbytes + pbytes;

        /* Allocate the memory. */
        buf = (char *)calloc(total_bytes, 1);
        if (buf == NULL)
            return REG_ESPACE;

        /* Get the various pointers within tmp_buf (properly aligned). */
        tmp_tags = (regoff_t *)buf;
        tmp_buf = buf + tbytes;
        tmp_buf += MUSL_REGEX_ALIGN(tmp_buf, intptr_t);
        reach_next = (tre_tnfa_reach_t *)tmp_buf;
        tmp_buf += rbytes;
        tmp_buf += MUSL_REGEX_ALIGN(tmp_buf, intptr_t);
        reach = (tre_tnfa_reach_t *)tmp_buf;
        tmp_buf += rbytes;
        tmp_buf += MUSL_REGEX_ALIGN(tmp_buf, intptr_t);
        reach_pos = (tre_reach_pos_t *)tmp_buf;
        tmp_buf += pbytes;
        tmp_buf += MUSL_REGEX_ALIGN(tmp_buf, intptr_t);
        for (i = 0; i < tnfa->num_states; i++) {
            reach[i].tags = (regoff_t *)tmp_buf;
            tmp_buf += xbytes;
            reach_next[i].tags = (regoff_t *)tmp_buf;
            tmp_buf += xbytes;
        }
    }

    for (i = 0; i < tnfa->num_states; i++)
        reach_pos[i].pos = -1;

    MUSL_REGEX_GET_NEXT_WCHAR();
    pos = 0;

    reach_next_i = reach_next;
    while (1) {
        /* If no match found yet, add the initial states to `reach_next'. */
        if (match_eo < 0) {
            trans_i = tnfa->initial;
            while (trans_i->state != NULL) {
                if (reach_pos[trans_i->state_id].pos < pos) {
                    if (trans_i->assertions && MUSL_REGEX_CHECK_ASSERTIONS(trans_i->assertions)) {
                        trans_i++;
                        continue;
                    }

                    reach_next_i->state = trans_i->state;
                    for (i = 0; i < num_tags; i++)
                        reach_next_i->tags[i] = -1;
                    tag_i = trans_i->tags;
                    if (tag_i)
                        while (*tag_i >= 0) {
                            if (*tag_i < num_tags)
                                reach_next_i->tags[*tag_i] = pos;
                            tag_i++;
                        }
                    if (reach_next_i->state == tnfa->final) {
                        match_eo = pos;
                        new_match = 1;
                        for (i = 0; i < num_tags; i++)
                            match_tags[i] = reach_next_i->tags[i];
                    }
                    reach_pos[trans_i->state_id].pos = pos;
                    reach_pos[trans_i->state_id].tags = &reach_next_i->tags;
                    reach_next_i++;
                }
                trans_i++;
            }
            reach_next_i->state = NULL;
        } else {
            if (num_tags == 0 || reach_next_i == reach_next) /* We have found a match. */
                break;
        }

        /* Check for end of string. */
        if (!next_c)
            break;

        MUSL_REGEX_GET_NEXT_WCHAR();

        /* Swap `reach' and `reach_next'. */
        reach_i = reach;
        reach = reach_next;
        reach_next = reach_i;

        /* For each state in `reach', weed out states that don't fulfill the
           minimal matching conditions. */
        if (tnfa->num_minimals && new_match) {
            new_match = 0;
            reach_next_i = reach_next;
            for (reach_i = reach; reach_i->state; reach_i++) {
                int skip = 0;
                for (i = 0; tnfa->minimal_tags[i] >= 0; i += 2) {
                    int end = tnfa->minimal_tags[i];
                    int start = tnfa->minimal_tags[i + 1];
                    if (end >= num_tags) {
                        skip = 1;
                        break;
                    } else if (
                        reach_i->tags[start] == match_tags[start] &&
                        reach_i->tags[end] < match_tags[end]) {
                        skip = 1;
                        break;
                    }
                }
                if (!skip) {
                    reach_next_i->state = reach_i->state;
                    tmp_iptr = reach_next_i->tags;
                    reach_next_i->tags = reach_i->tags;
                    reach_i->tags = tmp_iptr;
                    reach_next_i++;
                }
            }
            reach_next_i->state = NULL;

            /* Swap `reach' and `reach_next'. */
            reach_i = reach;
            reach = reach_next;
            reach_next = reach_i;
        }

        /* For each state in `reach' see if there is a transition leaving with
           the current input symbol to a state not yet in `reach_next', and
           add the destination states to `reach_next'. */
        reach_next_i = reach_next;
        for (reach_i = reach; reach_i->state; reach_i++) {
            for (trans_i = reach_i->state; trans_i->state; trans_i++) {
                /* Does this transition match the input symbol? */
                if (trans_i->code_min <= (tre_cint_t)prev_c &&
                    trans_i->code_max >= (tre_cint_t)prev_c) {
                    if (trans_i->assertions &&
                        (MUSL_REGEX_CHECK_ASSERTIONS(trans_i->assertions) ||
                         MUSL_REGEX_CHECK_CHAR_CLASSES(trans_i, tnfa, eflags))) {
                        continue;
                    }

                    /* Compute the tags after this transition. */
                    for (i = 0; i < num_tags; i++)
                        tmp_tags[i] = reach_i->tags[i];
                    tag_i = trans_i->tags;
                    if (tag_i != NULL)
                        while (*tag_i >= 0) {
                            if (*tag_i < num_tags)
                                tmp_tags[*tag_i] = pos;
                            tag_i++;
                        }

                    if (reach_pos[trans_i->state_id].pos < pos) {
                        /* Found an unvisited node. */
                        reach_next_i->state = trans_i->state;
                        tmp_iptr = reach_next_i->tags;
                        reach_next_i->tags = tmp_tags;
                        tmp_tags = tmp_iptr;
                        reach_pos[trans_i->state_id].pos = pos;
                        reach_pos[trans_i->state_id].tags = &reach_next_i->tags;

                        if (reach_next_i->state == tnfa->final &&
                            (match_eo == -1 ||
                             (num_tags > 0 && reach_next_i->tags[0] <= match_tags[0]))) {
                            match_eo = pos;
                            new_match = 1;
                            for (i = 0; i < num_tags; i++)
                                match_tags[i] = reach_next_i->tags[i];
                        }
                        reach_next_i++;

                    } else {
                        assert(reach_pos[trans_i->state_id].pos == pos);
                        /* Another path has also reached this state.  We choose
                           the winner by examining the tag values for both
                           paths. */
                        if (tre_tag_order(
                                num_tags,
                                tnfa->tag_directions,
                                tmp_tags,
                                *reach_pos[trans_i->state_id].tags)) {
                            /* The new path wins. */
                            tmp_iptr = *reach_pos[trans_i->state_id].tags;
                            *reach_pos[trans_i->state_id].tags = tmp_tags;
                            if (trans_i->state == tnfa->final) {
                                match_eo = pos;
                                new_match = 1;
                                for (i = 0; i < num_tags; i++)
                                    match_tags[i] = tmp_tags[i];
                            }
                            tmp_tags = tmp_iptr;
                        }
                    }
                }
            }
        }
        reach_next_i->state = NULL;
    }

    *match_end_ofs = match_eo;
    ret = match_eo >= 0 ? REG_OK : REG_NOMATCH;
error_exit:
    musl_regex_xfree(buf);
    return ret;
}

/***********************************************************************
 from tre-match-backtrack.c
***********************************************************************/

/*
  This matcher is for regexps that use back referencing.  Regexp matching
  with back referencing is an NP-complete problem on the number of back
  references.  The easiest way to match them is to use a backtracking
  routine which basically goes through all possible paths in the TNFA
  and chooses the one which results in the best (leftmost and longest)
  match.  This can be spectacularly expensive and may run out of stack
  space, but there really is no better known generic algorithm.	 Quoting
  Henry Spencer from comp.compilers:
  <URL: http://compilers.iecc.com/comparch/article/93-03-102>

    POSIX.2 REs require longest match, which is really exciting to
    implement since the obsolete ("basic") variant also includes
    \<digit>.  I haven't found a better way of tackling this than doing
    a preliminary match using a DFA (or simulation) on a modified RE
    that just replicates subREs for \<digit>, and then doing a
    backtracking match to determine whether the subRE matches were
    right.  This can be rather slow, but I console myself with the
    thought that people who use \<digit> deserve very slow execution.
    (Pun unintentional but very appropriate.)

*/

typedef struct {
    regoff_t pos;
    const char *str_byte;
    tre_tnfa_transition_t *state;
    int state_id;
    int next_c;
    regoff_t *tags;
} tre_backtrack_item_t;

typedef struct tre_backtrack_struct {
    tre_backtrack_item_t item;
    struct tre_backtrack_struct *prev;
    struct tre_backtrack_struct *next;
} *tre_backtrack_t;

#define MUSL_REGEX_BT_STACK_MBSTATE_IN
#define MUSL_REGEX_BT_STACK_MBSTATE_OUT

#define tre_bt_mem_new tre_mem_new
#define tre_bt_mem_alloc tre_mem_alloc
#define tre_bt_mem_destroy tre_mem_destroy

#define MUSL_REGEX_BT_STACK_PUSH(                                                                  \
    _pos, _str_byte, _str_wide, _state, _state_id, _next_c, _tags, _mbstate)                       \
    do {                                                                                           \
        int i;                                                                                     \
        if (!stack->next) {                                                                        \
            tre_backtrack_t s;                                                                     \
            s = (tre_backtrack_t)tre_bt_mem_alloc(mem, sizeof(*s));                                \
            if (!s) {                                                                              \
                tre_bt_mem_destroy(mem);                                                           \
                if (tags)                                                                          \
                    musl_regex_xfree(tags);                                                        \
                if (pmatch)                                                                        \
                    musl_regex_xfree(pmatch);                                                      \
                if (states_seen)                                                                   \
                    musl_regex_xfree(states_seen);                                                 \
                return REG_ESPACE;                                                                 \
            }                                                                                      \
            s->prev = stack;                                                                       \
            s->next = NULL;                                                                        \
            s->item.tags = (regoff_t *)tre_bt_mem_alloc(mem, sizeof(*tags) * tnfa->num_tags);      \
            if (!s->item.tags) {                                                                   \
                tre_bt_mem_destroy(mem);                                                           \
                if (tags)                                                                          \
                    musl_regex_xfree(tags);                                                        \
                if (pmatch)                                                                        \
                    musl_regex_xfree(pmatch);                                                      \
                if (states_seen)                                                                   \
                    musl_regex_xfree(states_seen);                                                 \
                return REG_ESPACE;                                                                 \
            }                                                                                      \
            stack->next = s;                                                                       \
            stack = s;                                                                             \
        } else                                                                                     \
            stack = stack->next;                                                                   \
        stack->item.pos = (_pos);                                                                  \
        stack->item.str_byte = (_str_byte);                                                        \
        stack->item.state = (_state);                                                              \
        stack->item.state_id = (_state_id);                                                        \
        stack->item.next_c = (_next_c);                                                            \
        for (i = 0; i < tnfa->num_tags; i++)                                                       \
            stack->item.tags[i] = (_tags)[i];                                                      \
        MUSL_REGEX_BT_STACK_MBSTATE_IN;                                                            \
    } while (0)

#define MUSL_REGEX_BT_STACK_POP()                                                                  \
    do {                                                                                           \
        int i;                                                                                     \
        assert(stack->prev);                                                                       \
        pos = stack->item.pos;                                                                     \
        str_byte = stack->item.str_byte;                                                           \
        state = stack->item.state;                                                                 \
        next_c = stack->item.next_c;                                                               \
        for (i = 0; i < tnfa->num_tags; i++)                                                       \
            tags[i] = stack->item.tags[i];                                                         \
        MUSL_REGEX_BT_STACK_MBSTATE_OUT;                                                           \
        stack = stack->prev;                                                                       \
    } while (0)

#undef MUSL_REGEX_MIN
#define MUSL_REGEX_MIN(a, b) ((a) <= (b) ? (a) : (b))

inline static reg_errcode_t tre_tnfa_run_backtrack(
    const tre_tnfa_t *tnfa,
    const void *string,
    regoff_t *match_tags,
    int eflags,
    regoff_t *match_end_ofs) {
    /* State variables required by MUSL_REGEX_GET_NEXT_WCHAR. */
    tre_char_t prev_c = 0, next_c = 0;
    const char *str_byte = (const char *)string;
    regoff_t pos = 0;
    regoff_t pos_add_next = 1;
    int reg_notbol = eflags & REG_NOTBOL;
    int reg_noteol = eflags & REG_NOTEOL;
    int reg_newline = tnfa->cflags & REG_NEWLINE;

    /* These are used to remember the necessary values of the above
       variables to return to the position where the current search
       started from. */
    int next_c_start;
    const char *str_byte_start;
    regoff_t pos_start = -1;

    /* End offset of best match so far, or -1 if no match found yet. */
    regoff_t match_eo = -1;
    /* Tag arrays. */
    int *next_tags;
    regoff_t *tags = NULL;
    /* Current TNFA state. */
    tre_tnfa_transition_t *state;
    int *states_seen = NULL;

    /* Memory allocator to for allocating the backtracking stack. */
    tre_mem_t mem = tre_bt_mem_new();

    /* The backtracking stack. */
    tre_backtrack_t stack;

    tre_tnfa_transition_t *trans_i;
    regmatch_t *pmatch = NULL;
    int ret;

    if (!mem)
        return REG_ESPACE;
    stack = (tre_backtrack_t)tre_bt_mem_alloc(mem, sizeof(*stack));
    if (!stack) {
        ret = REG_ESPACE;
        goto error_exit;
    }
    stack->prev = NULL;
    stack->next = NULL;

    if (tnfa->num_tags) {
        tags = (regoff_t *)musl_regex_xmalloc(sizeof(*tags) * tnfa->num_tags);
        if (!tags) {
            ret = REG_ESPACE;
            goto error_exit;
        }
    }
    if (tnfa->num_submatches) {
        pmatch = (regmatch_t *)musl_regex_xmalloc(sizeof(*pmatch) * tnfa->num_submatches);
        if (!pmatch) {
            ret = REG_ESPACE;
            goto error_exit;
        }
    }
    if (tnfa->num_states) {
        states_seen = (int *)musl_regex_xmalloc(sizeof(*states_seen) * tnfa->num_states);
        if (!states_seen) {
            ret = REG_ESPACE;
            goto error_exit;
        }
    }

retry: {
    int i;
    for (i = 0; i < tnfa->num_tags; i++) {
        tags[i] = -1;
        if (match_tags)
            match_tags[i] = -1;
    }
    for (i = 0; i < tnfa->num_states; i++)
        states_seen[i] = 0;
}

    state = NULL;
    pos = pos_start;
    MUSL_REGEX_GET_NEXT_WCHAR();
    pos_start = pos;
    next_c_start = next_c;
    str_byte_start = str_byte;
    /* Handle initial states. */
    next_tags = NULL;
    for (trans_i = tnfa->initial; trans_i->state; trans_i++) {
        if (trans_i->assertions && MUSL_REGEX_CHECK_ASSERTIONS(trans_i->assertions)) {
            continue;
        }
        if (state == NULL) {
            /* Start from this state. */
            state = trans_i->state;
            next_tags = trans_i->tags;
        } else {
            /* Backtrack to this state. */
            MUSL_REGEX_BT_STACK_PUSH(
                pos, str_byte, 0, trans_i->state, trans_i->state_id, next_c, tags, mbstate);
            {
                int *tmp = trans_i->tags;
                if (tmp)
                    while (*tmp >= 0)
                        stack->item.tags[*tmp++] = pos;
            }
        }
    }

    if (next_tags)
        for (; *next_tags >= 0; next_tags++)
            tags[*next_tags] = pos;

    if (state == NULL)
        goto backtrack;

    while (1) {
        tre_tnfa_transition_t *next_state;
        int empty_br_match;

        if (state == tnfa->final) {
            if (match_eo < pos ||
                (match_eo == pos && match_tags &&
                 tre_tag_order(tnfa->num_tags, tnfa->tag_directions, tags, match_tags))) {
                int i;
                /* This match wins the previous match. */
                match_eo = pos;
                if (match_tags)
                    for (i = 0; i < tnfa->num_tags; i++)
                        match_tags[i] = tags[i];
            }
            /* Our TNFAs never have transitions leaving from the final state,
               so we jump right to backtracking. */
            goto backtrack;
        }

        /* Go to the next character in the input string. */
        empty_br_match = 0;
        trans_i = state;
        if (trans_i->state && trans_i->assertions & MUSL_REGEX_ASSERT_MUSL_REGEX_BACKREF) {
            /* This is a back reference state.  All transitions leaving from
               this state have the same back reference "assertion".  Instead
               of reading the next character, we match the back reference. */
            regoff_t so, eo;
            int bt = trans_i->u.backref;
            regoff_t bt_len;
            int result;

            /* Get the substring we need to match against.  Remember to
               turn off REG_NOSUB temporarily. */
            tre_fill_pmatch(bt + 1, pmatch, tnfa->cflags & ~REG_NOSUB, tnfa, tags, pos);
            so = pmatch[bt].rm_so;
            eo = pmatch[bt].rm_eo;
            bt_len = eo - so;

            result = strncmp((const char *)string + so, str_byte - 1, (size_t)bt_len);

            if (result == 0) {
                /* Back reference matched.  Check for infinite loop. */
                if (bt_len == 0)
                    empty_br_match = 1;
                if (empty_br_match && states_seen[trans_i->state_id]) {
                    goto backtrack;
                }

                states_seen[trans_i->state_id] = empty_br_match;

                /* Advance in input string and resync `prev_c', `next_c'
                   and pos. */
                str_byte += bt_len - 1;
                pos += bt_len - 1;
                MUSL_REGEX_GET_NEXT_WCHAR();
            } else {
                goto backtrack;
            }
        } else {
            /* Check for end of string. */
            if (next_c == L'\0')
                goto backtrack;

            /* Read the next character. */
            MUSL_REGEX_GET_NEXT_WCHAR();
        }

        next_state = NULL;
        for (trans_i = state; trans_i->state; trans_i++) {
            if (trans_i->code_min <= (tre_cint_t)prev_c &&
                trans_i->code_max >= (tre_cint_t)prev_c) {
                if (trans_i->assertions && (MUSL_REGEX_CHECK_ASSERTIONS(trans_i->assertions) ||
                                            MUSL_REGEX_CHECK_CHAR_CLASSES(trans_i, tnfa, eflags))) {
                    continue;
                }

                if (next_state == NULL) {
                    /* First matching transition. */
                    next_state = trans_i->state;
                    next_tags = trans_i->tags;
                } else {
                    /* Second matching transition.  We may need to backtrack here
                       to take this transition instead of the first one, so we
                       push this transition in the backtracking stack so we can
                       jump back here if needed. */
                    MUSL_REGEX_BT_STACK_PUSH(
                        pos, str_byte, 0, trans_i->state, trans_i->state_id, next_c, tags, mbstate);
                    {
                        int *tmp;
                        for (tmp = trans_i->tags; tmp && *tmp >= 0; tmp++)
                            stack->item.tags[*tmp] = pos;
                    }
#if 0 /* XXX - it's important not to look at all transitions here to keep                          \
         the stack small! */
		  break;
#endif
                }
            }
        }

        if (next_state != NULL) {
            /* Matching transitions were found.  Take the first one. */
            state = next_state;

            /* Update the tag values. */
            if (next_tags)
                while (*next_tags >= 0)
                    tags[*next_tags++] = pos;
        } else {
        backtrack:
            /* A matching transition was not found.  Try to backtrack. */
            if (stack->prev) {
                if (stack->item.state->assertions & MUSL_REGEX_ASSERT_MUSL_REGEX_BACKREF) {
                    states_seen[stack->item.state_id] = 0;
                }

                MUSL_REGEX_BT_STACK_POP();
            } else if (match_eo < 0) {
                /* Try starting from a later position in the input string. */
                /* Check for end of string. */
                if (next_c == L'\0') {
                    break;
                }
                next_c = next_c_start;
                str_byte = str_byte_start;
                goto retry;
            } else {
                break;
            }
        }
    }

    ret = match_eo >= 0 ? REG_OK : REG_NOMATCH;
    *match_end_ofs = match_eo;

error_exit:
    tre_bt_mem_destroy(mem);
#ifndef TRE_USE_ALLOCA
    if (tags)
        musl_regex_xfree(tags);
    if (pmatch)
        musl_regex_xfree(pmatch);
    if (states_seen)
        musl_regex_xfree(states_seen);
#endif /* !TRE_USE_ALLOCA */

    return ret;
}

/***********************************************************************
 from regexec.c
***********************************************************************/

/* Fills the POSIX.2 regmatch_t array according to the TNFA tag and match
   endpoint values. */
inline static void tre_fill_pmatch(
    size_t nmatch,
    regmatch_t pmatch[],
    int cflags,
    const tre_tnfa_t *tnfa,
    regoff_t *tags,
    regoff_t match_eo) {
    tre_submatch_data_t *submatch_data;
    unsigned int i, j;
    int *parents;

    i = 0;
    if (match_eo >= 0 && !(cflags & REG_NOSUB)) {
        /* Construct submatch offsets from the tags. */
        submatch_data = tnfa->submatch_data;
        while (i < tnfa->num_submatches && i < nmatch) {
            if (submatch_data[i].so_tag == tnfa->end_tag)
                pmatch[i].rm_so = match_eo;
            else
                pmatch[i].rm_so = tags[submatch_data[i].so_tag];

            if (submatch_data[i].eo_tag == tnfa->end_tag)
                pmatch[i].rm_eo = match_eo;
            else
                pmatch[i].rm_eo = tags[submatch_data[i].eo_tag];

            /* If either of the endpoints were not used, this submatch
               was not part of the match. */
            if (pmatch[i].rm_so == -1 || pmatch[i].rm_eo == -1)
                pmatch[i].rm_so = pmatch[i].rm_eo = -1;

            i++;
        }
        /* Reset all submatches that are not within all of their parent
           submatches. */
        i = 0;
        while (i < tnfa->num_submatches && i < nmatch) {
            if (pmatch[i].rm_eo == -1)
                assert(pmatch[i].rm_so == -1);
            assert(pmatch[i].rm_so <= pmatch[i].rm_eo);

            parents = submatch_data[i].parents;
            if (parents != NULL)
                for (j = 0; parents[j] >= 0; j++) {
                    if (pmatch[i].rm_so < pmatch[parents[j]].rm_so ||
                        pmatch[i].rm_eo > pmatch[parents[j]].rm_eo)
                        pmatch[i].rm_so = pmatch[i].rm_eo = -1;
                }
            i++;
        }
    }

    while (i < nmatch) {
        pmatch[i].rm_so = -1;
        pmatch[i].rm_eo = -1;
        i++;
    }
}

/*
  Wrapper functions for POSIX compatible regexp matching.
*/

inline int
regexec(const regex_t *preg, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags) {
    tre_tnfa_t *tnfa = (tre_tnfa_t *)preg->TRE_REGEX_T_FIELD;
    reg_errcode_t status;
    regoff_t *tags = NULL, eo;
    if (tnfa->cflags & REG_NOSUB)
        nmatch = 0;
    if (tnfa->num_tags > 0 && nmatch > 0) {
        tags = (regoff_t *)musl_regex_xmalloc(sizeof(*tags) * tnfa->num_tags);
        if (tags == NULL)
            return REG_ESPACE;
    }

    /* Dispatch to the appropriate matcher. */
    if (tnfa->have_backrefs) {
        /* The regex has back references, use the backtracking matcher. */
        status = tre_tnfa_run_backtrack(tnfa, string, tags, eflags, &eo);
    } else {
        /* Exact matching, no back references, use the parallel matcher. */
        status = tre_tnfa_run_parallel(tnfa, string, tags, eflags, &eo);
    }

    if (status == REG_OK) /* A match was found, so fill the submatch registers. */
        tre_fill_pmatch(nmatch, pmatch, tnfa->cflags, tnfa, tags, eo);
    if (tags)
        musl_regex_xfree(tags);
    return status;
}

#ifdef __cplusplus
}
#endif

#endif
