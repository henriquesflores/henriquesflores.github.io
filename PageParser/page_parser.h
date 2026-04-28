#ifndef PAGE_PARSER_H
#define PAGE_PARSER_H

#include <assert.h>
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "types.h"


#ifndef PGDEF
    #define PGDEF 
#endif


struct string_view {
    usize size;
    u8 *data;
};


PGDEF struct string_view sv(char *s);


struct memory_arena {
    u64 used;
    u64 capacity;
    u8 *bytes; 
};

global_variable struct memory_arena global_arena;

PGDEF struct memory_arena arena_init(u64 size);
PGDEF void free_arena(struct memory_arena *a);
PGDEF void *_pushStructArenaImpl(struct memory_arena *a, usize size);
PGDEF void *_pushStructArenaImpl(struct memory_arena *a, usize size);
#define PUSH_STRUCT(arena, type) _pushStructArenaImpl(&(arena), sizeof(type))
PGDEF void *_pushArenaArrayImpl(struct memory_arena *a, usize struct_size, usize array_size);
#define PUSH_ARRAY(arena, type, size) _pushArenaArrayImpl(&(arena), sizeof(type), size)


struct strbuffer {
    u64 size; 
    u8 *buffer;
};


PGDEF struct strbuffer alloc_buffer(struct string_view s);
PGDEF void free_strbuffer(struct strbuffer *c);
PGDEF struct strbuffer read_entire_file(char *filename);


enum token_kind {
    TOKEN_UNKNOWN = 0,
    TOKEN_HEAD,
    TOKEN_TITLE,
    TOKEN_SECTION,
    TOKEN_SUBSECTION, 
    TOKEN_LIST_ELEMENT,
    TOKEN_LINK,
    TOKEN_PARAGRAPH,
    TOKEN_INLINE_EQUATION,
    TOKEN_EQUATION,
    TOKEN_FIGURE,
    TOKEN_BOLD,
    TOKEN_ITALIC,
    TOKEN_KIND_COUNT,
};


struct markdown_parser {
    u8 *contents;
    b32 has_error;
    usize length;
    usize cursor; 
};


struct parser_snapshot {
    u8 letter; 
    u8 next_letter;
    usize cursor;
};


struct markdown_figure {
    struct strbuffer alt_text;
    struct strbuffer address;
    struct strbuffer caption;
};


struct markdown_list_element {
    u64 item_number; 
    struct strbuffer item;
};


struct markdown_link {
    struct strbuffer text; 
    struct strbuffer url;
};


struct markdown_token {
    u64 new_line_space;
    enum token_kind kind;
    union {
        struct strbuffer item;
        struct markdown_link link;
        struct markdown_figure figure;
        struct markdown_list_element element;
    };
};


struct markdown_node {
    struct markdown_token token; 
    struct markdown_node *children;
    struct markdown_node *next_sibling;
};


struct markdown_tree {
    struct markdown_node *head;
    struct markdown_node *current;
    struct markdown_node *previous;
};


PGDEF b32 sv_isequal(struct string_view s, struct string_view w);
PGDEF b32 sv_startswith(struct string_view s, struct string_view c);
PGDEF b32 sv_endswith(struct string_view s, struct string_view c);
PGDEF u64 svtoint(struct string_view s);
PGDEF b32 sv_startswith_any(struct string_view s, char **cstr_array, usize array_size);
PGDEF b32 sv_endswith_any(struct string_view s, char **cstr_array, usize array_size);

PGDEF b32 is_whitespace(struct string_view s);
PGDEF b32 not_whitespace(struct string_view s);
PGDEF b32 is_new_line(struct string_view s);
PGDEF b32 not_new_line(struct string_view s);
PGDEF b32 is_open_bracket(struct string_view s);
PGDEF b32 is_close_bracket(struct string_view s);
PGDEF b32 not_close_bracket(struct string_view s);
PGDEF b32 is_open_parenthesis(struct string_view s);
PGDEF b32 is_close_parenthesis(struct string_view s);
PGDEF b32 not_close_parenthesis(struct string_view s);
PGDEF b32 is_dot(struct string_view s);
PGDEF b32 is_list_identifier(struct string_view s);
PGDEF b32 is_numeric(struct string_view s);
PGDEF struct string_view svtrim_while(
    struct string_view s, 
    b32 (*predicate)(int)
);
PGDEF struct string_view svtrim_whitespace(struct string_view s);
PGDEF b32 is_hash(struct string_view s);
PGDEF b32 is_dollar(struct string_view s);
PGDEF b32 is_in_bounds(struct markdown_parser *p);
PGDEF b32 cursor_in_bounds(struct markdown_parser *p, usize cursor);
PGDEF u8 get(struct markdown_parser *p);
PGDEF u8 *at(struct markdown_parser *p);
PGDEF struct string_view asv(struct markdown_parser *p);
PGDEF b32 is_parsing(struct markdown_parser p);
PGDEF u64 skip_while(
    struct markdown_parser *p, 
    b32 (*predicate)(struct string_view)
);
PGDEF struct string_view extract_while(
    struct markdown_parser *p, 
    b32 (*predicate)(struct string_view)
);
PGDEF struct markdown_figure parse_markdown_image(
    struct string_view alt_text, 
    struct string_view figure_info
);

PGDEF struct parser_snapshot get_snapshot(struct markdown_parser *p);
PGDEF b32 is_paragraph(struct string_view s);
PGDEF b32 is_equation(struct string_view s);

PGDEF struct markdown_token parse_token(struct markdown_parser *parser);
PGDEF struct markdown_node *alloc_markdown_node(struct markdown_token t);
PGDEF struct markdown_tree alloc_markdown_tree(void);
PGDEF b32 add_node(struct markdown_tree *t, struct markdown_token token);
PGDEF struct markdown_tree parse_markdown(char *filepath);

PGDEF void emit_tex(char *filepath, struct markdown_tree t);
PGDEF void emit_html(char *filepath, struct markdown_tree t);
#endif