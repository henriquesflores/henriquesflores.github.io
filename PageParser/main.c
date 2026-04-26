#include <assert.h>
#include <stdio.h> 
#include <stdlib.h>
#include <ctype.h>

#include "types.h"


struct string_view {
    usize size;
    u8 *data;
};


internal struct string_view sv(char *s) 
{
    return (struct string_view) {strlen(s), s};
}


struct memory_arena {
    u64 used;
    u64 capacity;
    u8 *bytes; 
};


internal struct memory_arena arena_init(u64 size)
{
    u8 *bytes = malloc(size);
    if (!bytes) {
        fprintf(stderr, "[ERROR]: Arena failed to allocate. There is no memory to parse this markdown\n");
        exit(1);
    }
    return (struct memory_arena) {0, size, bytes};
}


internal void free_arena(struct memory_arena *a)
{
    if (a->bytes) {
        free(a->bytes);
        a->used  = 0;
        a->bytes = 0;
        a->capacity = 0;
    }
}


global_variable struct memory_arena global_arena;
internal void *_pushStructArenaImpl(struct memory_arena *a, usize size)
{
    if (size && a->used + size <= a->capacity) {
        void *address = a->bytes + a->used;
        a->used += size;
        return address;
    }
    fprintf(stderr, "[ERROR]: Buffer allocation exceeds arena capacity\n");
    return 0;
}
#define PUSH_STRUCT(arena, type) _pushStructArenaImpl(&(arena), sizeof(type))


internal void *_pushArenaArrayImpl(struct memory_arena *a, usize struct_size, usize array_size)
{
    usize total_size = struct_size * array_size;
    return _pushStructArenaImpl(a, total_size);
}
#define PUSH_ARRAY(arena, type, size) _pushArenaArrayImpl(&(arena), sizeof(type), size)


struct strbuffer {
    u64 size; 
    u8 *buffer;
};


internal struct strbuffer alloc_buffer(struct string_view s) 
{
    u8 *buffer = 0; 
    if ((buffer = PUSH_ARRAY(global_arena, u8, s.size)))
        memcpy(buffer, s.data, s.size);
    struct strbuffer result = {s.size, buffer};
    return result;
}


internal void free_strbuffer(struct strbuffer *c)
{
    if (c->buffer) {
        free(c->buffer);
        c->buffer = 0;
        c->size = 0;
    }
}


internal struct strbuffer read_entire_file(char *filename) 
{
    // TODO (Henrique): Move later to a way where text can be streamed in blocks
    // instead of loading the entire file into memory;
    FILE *f = 0; 
    struct strbuffer result = {0};
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
    
    result.buffer = (u8 *)malloc(file_size + 1);
    if (result.buffer == 0) {
        fprintf(stderr, "[ERROR] Memory allocation failed\n");
        goto cleanup;
    }

    u64 bytes_read = fread(result.buffer, sizeof(result.buffer[0]), file_size, f);
    if (bytes_read == CAST(u64, file_size)) {
        result.buffer[file_size] = 0; 
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
    free_strbuffer(&result);

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
    TOKEN_SUBSECTION, 
    TOKEN_NUMBER,
    TOKEN_OPEN_BRACKET, 
    TOKEN_CLOSE_BRACKET, 
    TOKEN_OPEN_PARENTHESIS,
    TOKEN_CLOSE_PARENTHESIS,
    TOKEN_OPEN_BRACE,
    TOKEN_CLOSE_BRACE,
    TOKEN_EQUAL,
    TOKEN_PARAGRAPH,
    TOKEN_INLINE_EQUATION,
    TOKEN_EQUATION,
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


struct markdown_token {
    enum token_kind kind;
    struct strbuffer item;
};


struct markdown_node {
    struct markdown_token token; 
    struct markdown_node *children;
};


struct markdown_tree {
    struct markdown_node *head;
    struct markdown_node *current;
};


internal b32 sv_isequal(
    struct string_view s,
    char *cstr
) {
    usize cs = strlen(cstr);
    return (s.size >= cs && strncmp(s.data, cstr, cs) == 0);
}


internal b32 sv_isequal_any(
    struct string_view s,
    char **cstr_array, 
    usize array_size
) {
    for (int i = 0; i < array_size; i++) 
        if (sv_isequal(s, cstr_array[i]))
            return 1;
    return 0;
}


internal b32 is_space(struct string_view s)
{
    char *c[] = {" ", "\n", "\r\n"};
    return sv_isequal_any(s, c, ARRAY_LEN(c));
}


internal b32 not_space(struct string_view s)
{
    return !is_space(s);
}


internal b32 is_new_line(struct string_view s)
{
    char *c[] = {"\n", "\r\n"};
    return sv_isequal_any(s, c, ARRAY_LEN(c));
}


internal b32 not_new_line(struct string_view s)
{
    return !is_new_line(s);
}


internal struct string_view svtrim(struct string_view s)
{
    usize i = 0;
    while (i < s.size && s.data[i] == ' ')
        i++;
    
    usize j = 1; 
    while (j < s.size && s.data[s.size - 1 - j] == ' ')
        j++;
    
    usize result_size  = (i + j > s.size) ? 0 : s.size - (i + j);
    u8 *result_pointer = (i + j > s.size) ? 0 : s.data + i;
    struct string_view result = {result_size, result_pointer};
    return result;
}


internal b32 is_hash(struct string_view s) 
{
    return sv_isequal(s, "#");
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


internal struct string_view as_sv(struct markdown_parser *p)
{
    return (struct string_view) {p->length - p->cursor, at(p)};
}


internal b32 is_parsing(struct markdown_parser p) 
{
    return is_in_bounds(&p) && (!p.has_error);
}


internal u64 skip_while(
    struct markdown_parser *p, 
    b32 (*predicate)(struct string_view)
) {
    usize start = p->cursor; 
    while(is_in_bounds(p) && predicate(as_sv(p))) 
        p->cursor = p->cursor + 1;
    usize end = p->cursor;

    i64 result = end - start; 
    if (result > 0) 
        return CAST(u64, result);
    return 0;
}


internal struct string_view extract_while(
    struct markdown_parser *p, 
    b32 (*predicate)(struct string_view)
) {
    struct string_view result = {0};
    result.data = at(p);
    result.size = skip_while(p, predicate);
    return svtrim(result);
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
    u8 next_letter = cursor_in_bounds(p, next) ? p->contents[next] : 0;
    struct parser_snapshot result = {
        .letter      = get(p), 
        .next_letter = next_letter, 
        .cursor      = p->cursor
    };
    return result;
}


internal b32 is_paragraph(struct string_view s)
{
    char *c[] = {"$$", "##", "###"};
    b32 result = sv_isequal_any(s, c, ARRAY_LEN(c));
    return !result;
}


internal 
struct markdown_token parse_token(struct markdown_parser *parser)
{
    struct markdown_token result = {TOKEN_UNKNOWN, 0}; 

    skip_while(parser, is_space);
    if (is_in_bounds(parser)) {
        struct parser_snapshot p = get_snapshot(parser);
        switch (p.letter) {
            case '#': {
                result.kind = p.next_letter == '#' ? TOKEN_SECTION : TOKEN_TITLE; 
                skip_while(parser, is_hash);
                struct string_view title = extract_while(parser, not_new_line);
                result.item = alloc_buffer(title);
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
                result.kind = TOKEN_INLINE_EQUATION;
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
                result.kind = TOKEN_PARAGRAPH;
                struct string_view paragraph = extract_while(parser, is_paragraph);
                result.item = alloc_buffer(paragraph);
            } break;
        }
    }
    return result;
}


internal struct markdown_node *alloc_markdown_node(struct markdown_token t)
{
    assert(global_arena.capacity != 0);
    struct markdown_node *n = PUSH_STRUCT(global_arena, struct markdown_node);
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
        (struct markdown_token) {TOKEN_HEAD, alloc_buffer(sv("Head of markdown tree"))}
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
#define TEST_FILE "PageParser/tests/empty_title.md"
    struct strbuffer contents = read_entire_file(TEST_FILE);
    if (contents.size == 0) {
        fprintf(stderr, "[ERROR] Input file is empty. Nothing to do.");
        exit(0);
    }

    global_arena = arena_init(KILO(10));

    struct markdown_parser parser = {
        .contents  = contents.buffer, 
        .has_error = 0,
        .length    = contents.size, 
        .cursor    = 0
    };
    
    struct markdown_tree doc = alloc_markdown_tree();
    do {
        struct markdown_token t = parse_token(&parser);
        if (!parser.has_error)
            parser.has_error = !add_node(&doc, alloc_markdown_node(t));
    } while (is_parsing(parser));

    free_arena(&global_arena);
    free_strbuffer(&contents);
    return 0;
}