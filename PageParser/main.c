#include <stdio.h> 
#include <stdlib.h>
#include <ctype.h>

#include "types.h"


struct string_view {
    usize size;
    u8 *data;
};


void free_sv(struct string_view *c)
{
    free(c->data);
    c->data = 0;
    c->size = 0;
}


internal struct string_view 
read_entire_file(u8 *filename) 
{
    // TODO (Henrique): Move later to a way where text can be streamed in blocks
    // instead of loading the entire file into memory;
    FILE *f = 0; 
    struct string_view result = {0};
    if ((f = fopen(filename, "rb")) == 0) {
        fprintf(stderr, "[ERROR] Could not open file: %s\n", filename); 
        return result;
    } 
    
    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "[ERROR] Could not seek to end of file\n");
        goto cleanup;
    };
    
    i64 file_size = 0; 
    if ((file_size = ftell(f)) < 0) {
        fprintf(stderr, "[ERRRO] Could not calculate size from file pointer with ftell\n");
        goto cleanup;
    }
    
    if (fseek(f, 0, SEEK_SET) != 0) {
        fprintf(stderr, "[ERROR] Could not seek to start of file\n");
        goto cleanup;
    }
    
    result.data = (u8 *)malloc(file_size + 1);
    if (result.data == 0) {
        fprintf(stderr, "[ERROR] Memory allocation failed\n");
        goto cleanup;
    }

    u64 bytes_read = fread(result.data, sizeof(result.data[0]), file_size, f);
    if (bytes_read == CAST(u64, file_size)) {
        result.data[file_size] = '\0'; 
        result.size = file_size;
        goto exit;
    } 
    else {
        if (feof(f)) {
            fprintf(stderr, "[ERROR] Unexpected end of file\n");
        } 
        else if (ferror(f)) {
            fprintf(stderr, "[ERROR] Error reading file\n");
        } 
        else {
            fprintf(stderr, "[ERROR] Unknown error reading file\n");
        }
        goto cleanup;
    }

cleanup:
    if (result.data)
        free_sv(&result);

exit:
    if (f)
        fclose(f);

    return result;
}


enum token_kind {
    TOKEN_UNKNOWN = 0,
    TOKEN_HEAD,
    TOKEN_TITLE,
    TOKEN_SECTION,
    TOKEN_NUMBER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_EQUALS,
    TOKEN_OPEN_BRACKET, 
    TOKEN_CLOSE_BRACKET, 
    TOKEN_OPEN_PARENTHESIS,
    TOKEN_CLOSE_PARENTHESIS,
    TOKEN_OPEN_BRACE,
    TOKEN_CLOSE_BRACE,
    TOKEN_BRA, 
    TOKEN_KET,
    TOKEN_EQUAL,
    TOKEN_WORD,
    TOKEN_SUBSECTION, 
    TOKEN_DOLLAR_SIGN,
    TOKEN_BOLD,
    TOKEN_ITALIC,
    TOKEN_COUNT,
};


struct markdown_parser {
    u8 *contents;
    b32 has_error;
    usize length;
    usize cursor; 
};


internal struct string_view sv(char *s) 
{
    return (struct string_view) {strlen(s), s};
}


struct markdown_token {
    enum token_kind kind;
    struct string_view item;
};


struct markdown_node {
    struct markdown_token token; 
    struct markdown_node *children;
};


struct markdown_tree {
    struct markdown_node *head;
    struct markdown_node *current;
};


internal b32 is_space(u8 c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}


internal b32 not_space(u8 c)
{
    return !is_space(c);
}


internal b32 is_new_line(u8 c)
{
    return (c == '\n') || (c == '\r');
}


internal b32 not_new_line(u8 c)
{
    return !is_new_line(c);
}


internal b32 is_hash(u8 c) 
{
    return c == '#';
}


internal b32 is_in_bounds(struct markdown_parser *p)
{
    b32 result = p->cursor < p->length;
    return result;
}


internal b32 cursor_in_bounds(struct markdown_parser *p, usize cursor)
{
    return is_in_bounds(p) && (p->cursor < cursor);
}


internal u8 get(struct markdown_parser *p) 
{
    return p->contents[p->cursor];
}


internal u8 *at(struct markdown_parser *p) 
{
    return p->contents + p->cursor;
}


internal b32 is_parsing(struct markdown_parser p) 
{
    return is_in_bounds(&p) && (!p.has_error);
}


internal u64 skip_while(
    struct markdown_parser *p, 
    b32 (*predicate)(u8 c)
) {
    usize start = p->cursor; 
    while(is_in_bounds(p) && predicate(get(p))) 
        p->cursor = p->cursor + 1;
    usize end = p->cursor;

    i64 result = end - start; 
    if (result > 0) 
        return CAST(u64, result);
    return 0;
}


internal struct string_view extract_while(
    struct markdown_parser *p, 
    b32 (*predicate)(u8 c)
) {
    struct string_view result = {0};
    u64 _ = skip_while(p, is_space);
    result.data = at(p);
    result.size = skip_while(p, predicate);
    return result;
}


struct parser_snapshot {
    u8 letter; 
    u8 next_letter;
    usize cursor;
};


internal 
struct parser_snapshot get_snapshot(struct markdown_parser *p)
{
    usize next     = p->cursor + 1;
    u8 letter      = get(p);
    u8 next_letter = cursor_in_bounds(p, next) ? p->contents[next] : 0;
    struct parser_snapshot result = {
        .letter      = letter, 
        .next_letter = next_letter, 
        .cursor      = p->cursor
    };
    return result;
}


internal 
struct markdown_token parse_token(struct markdown_parser *parser)
{
    struct markdown_token result = {0}; 
    result.kind = TOKEN_UNKNOWN; 
    result.item = sv("");

    skip_while(parser, is_space);
    if (is_in_bounds(parser)) {
        struct parser_snapshot p = get_snapshot(parser);
        switch (p.letter) {
            case '#': {
                result.kind = p.next_letter == '#' ? TOKEN_SECTION : TOKEN_TITLE; 

                skip_while(parser, is_hash);
                skip_while(parser, is_space);
                parser->has_error = !is_in_bounds(parser);
                
                struct string_view title = extract_while(parser, not_new_line);
                result.item = title;
                skip_while(parser, is_space);
            } break;
            case '(': {
                result.kind = TOKEN_OPEN_PARENTHESIS;
            } break;
            case ')': {
                result.kind = TOKEN_CLOSE_PARENTHESIS;
            } break;
            case '[': {
                result.kind = TOKEN_OPEN_BRACKET;
            } break;
            case ']': {
                result.kind = TOKEN_CLOSE_BRACKET;
            } break;
            case '{': {
                result.kind = TOKEN_OPEN_BRACE;
            } break;
            case '}': {
                result.kind = TOKEN_CLOSE_BRACE;
            } break;
            case '$': {
                result.kind = TOKEN_DOLLAR_SIGN;
            } break;
            case '-': {
                // This is an interesting case. I can be a negative number of some bs;
            } break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
//                result.kind = TOKEN_NUMBER;
            } break;
            default: {
                struct string_view line = extract_while(parser, not_space);
                result.kind = TOKEN_WORD;
                result.item = line;
            } break;
        }
    }
    return result;
}


struct memory_arena {
    u8 *bytes; 
    u64 size;
};


internal 
struct memory_arena arena_init(u64 size)
{
    u8 *bytes = malloc(size);
    if (!bytes) {
        fprintf(stderr, "[ERROR]: Arena failed to allocate. There is no memory to parse this markdown\n");
        exit(1);
    }
    return (struct memory_arena) {bytes, size};
}


internal void *_pushArenaImpl(struct memory_arena *a, usize size)
{
    i64 remaining = a->size - size;
    if (remaining <= 0) 
        return NULL;
    
    void *address = a->bytes + a->size;
    a->size += size;
    return address;
}
#define PUSH(arena, type) _pushArenaImpl(&(arena), sizeof(type))


global_variable struct memory_arena global_arena;


internal struct markdown_node *alloc_markdown_node(struct markdown_token t)
{
    struct markdown_node *n = PUSH(global_arena, struct markdown_node);
    if (!n) {
        fprintf(stderr, "[ERROR]: Failed to allocate markdown node. Entire process is killed.\n");
        exit(1);
    }
    n->token = t;
    n->children = 0;
    return n;
}


internal struct markdown_tree alloc_markdown_tree()
{
    struct markdown_tree result = {0};
    result.head = alloc_markdown_node(
        (struct markdown_token) {TOKEN_HEAD, sv("Head of markdown tree")}
    );
    result.current = result.head;
    return result;
}


internal b32 add_node(struct markdown_tree *t, struct markdown_node *n)
{
    t->current->children = n;
    t->current = n;
    return 1;
}


int main(int argc, char **argv) 
{
#define TEST_FILE "posts/about.md"
    struct string_view contents = read_entire_file(TEST_FILE);
    if (contents.size == 0) {
        fprintf(stderr, "[ERROR] Input file is empty. Nothing to do.");
        exit(0);
    }

    global_arena = arena_init(MEGA(10));

    struct markdown_parser parser = {
        .contents  = contents.data, 
        .has_error = 0,
        .length    = contents.size, 
        .cursor    = 0
    };
    
    struct markdown_tree doc = alloc_markdown_tree();
    do {
        struct markdown_token t = parse_token(&parser);
        parser.has_error = !add_node(&doc, alloc_markdown_node(t));
    } while (is_parsing(parser));

    free_sv(&contents);
    return 0;
}