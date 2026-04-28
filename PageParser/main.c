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


internal b32 sv_isequal(
    struct string_view s, 
    struct string_view w
) { 
    return (s.size == w.size && strncmp(s.data, w.data, s.size) == 0);
}


internal b32 sv_startswith(
    struct string_view s,
    struct string_view c
) {
    return (s.size >= c.size && strncmp(s.data, c.data, c.size) == 0);
}


internal b32 sv_endswith(
    struct string_view s, 
    struct string_view c
) {
    return (s.size >= c.size && strncmp(s.data + s.size - c.size, c.data, c.size) == 0);
}



#define STACK_STRING_SIZE 64
internal u64 svtoint(struct string_view s)
{
    if (STACK_STRING_SIZE + 1 < s.size) {
        fprintf(stderr, "Ops, there is nothing implemented here yet\n");
        exit(1);
    } else { 
        char c[STACK_STRING_SIZE];
        memcpy(c, s.data, STACK_STRING_SIZE);
        c[s.size] = 0;
        u64 result = atoi(c);
        return result;
    }
}


internal b32 sv_startswith_any(
    struct string_view s,
    char **cstr_array, 
    usize array_size
) {
    for (int i = 0; i < array_size; i++) 
        if (sv_startswith(s, sv(cstr_array[i])))
            return 1;
    return 0;
}


internal b32 sv_endswith_any(
    struct string_view s, 
    char **cstr_array, 
    usize array_size
) {
    for (int i = 0; i < array_size; i++) 
        if (sv_endswith(s, sv(cstr_array[i])))
            return 1;
    return 0;
}


internal b32 is_whitespace(struct string_view s)
{
    char *c[] = {" ", "\n", "\r\n"};
    return sv_startswith_any(s, c, ARRAY_LEN(c));
}


internal b32 not_whitespace(struct string_view s)
{
    return !is_whitespace(s);
}


internal b32 is_new_line(struct string_view s)
{
    char *c[] = {"\n", "\r\n"};
    return sv_startswith_any(s, c, ARRAY_LEN(c));
}


internal b32 not_new_line(struct string_view s)
{
    return !is_new_line(s);
}


internal b32 is_open_bracket(struct string_view s)
{
    return sv_startswith(s, sv("["));
}


internal b32 is_close_bracket(struct string_view s)
{
    return sv_startswith(s, sv("]"));
}


internal b32 not_close_bracket(struct string_view s)
{
    return !is_close_bracket(s);
}


internal is_open_parenthesis(struct string_view s)
{
    return sv_startswith(s, sv("("));
}


internal is_close_parenthesis(struct string_view s)
{
    return sv_startswith(s, sv(")"));
}


internal not_close_parenthesis(struct string_view s)
{
    return !is_close_parenthesis(s);
}


internal is_dot(struct string_view s)
{
    return sv_startswith(s, sv("."));
}


internal is_list_identifier(struct string_view s)
{
    char *c[] = {"*", "+", "-"};
    return sv_startswith_any(s, c, ARRAY_LEN(c));
}


internal b32 is_numeric(struct string_view s)
{
    char *c[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    return sv_startswith_any(s, c, ARRAY_LEN(c));
}


internal struct string_view svtrim_while(
    struct string_view s, 
    b32 (*predicate)(int)
) {
    usize i = 0;
    while (i < s.size && predicate(s.data[i]))
        i++;

    usize j = 0; 
    while (j < s.size && predicate(s.data[s.size - 1 - j]))
        j++;
    
    usize result_size  = (i + j > s.size) ? 0 : s.size - (i + j);
    u8 *result_pointer = (i + j > s.size) ? 0 : s.data + i;
    struct string_view result = {result_size, result_pointer};
    return result;
}


internal struct string_view svtrim_whitespace(struct string_view s)
{
    return svtrim_while(s, isspace);
}


internal b32 is_hash(struct string_view s) 
{
    return sv_startswith(s, sv("#"));
}


internal b32 is_dollar(struct string_view s)
{
    return sv_startswith(s, sv("$"));
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


internal struct string_view asv(struct markdown_parser *p)
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
    while(is_in_bounds(p) && predicate(asv(p))) 
        p->cursor = p->cursor + 1;
    usize result = (p->cursor > start) ? p->cursor - start : 0;
    return CAST(u64, result);
}


internal struct string_view extract_while(
    struct markdown_parser *p, 
    b32 (*predicate)(struct string_view)
) {
    struct string_view result = {0};
    result.data = at(p);
    result.size = skip_while(p, predicate);
    return svtrim_whitespace(result);
}


internal struct markdown_figure parse_markdown_image(
    struct string_view alt_text, 
    struct string_view figure_info
) {
    struct markdown_figure result = {0};
    result.alt_text = alloc_buffer(alt_text);

    usize c = 0;
    while (c < figure_info.size && figure_info.data[c] != ' ')
        c++;
    struct string_view address = {c, figure_info.data};
    result.address = alloc_buffer(svtrim_whitespace(address));

    usize i = c + 1;
    struct string_view caption = {figure_info.size - i, figure_info.data + i};
    result.caption = alloc_buffer(svtrim_whitespace(caption));
    return result;
}


struct parser_snapshot {
    u8 letter; 
    u8 next_letter;
    usize cursor;
};


internal struct parser_snapshot get_snapshot(struct markdown_parser *p)
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
    b32 result = sv_startswith_any(s, c, ARRAY_LEN(c));
    return !result;
}


internal b32 is_equation(struct string_view s)
{
    return !sv_startswith(s, sv("$$"));
}


internal u8 *errormsg() 
{
    return "[ERROR]: Markdown parser encountered error while parsing ";
}


internal 
struct markdown_token parse_token(struct markdown_parser *parser)
{
    struct markdown_token result = {0, TOKEN_UNKNOWN, 0}; 

    result.new_line_space = skip_while(parser, is_whitespace);
    if (is_in_bounds(parser)) {
        struct parser_snapshot p = get_snapshot(parser);
        switch (p.letter) {
            case '#': {
                result.kind = p.next_letter == '#' ? TOKEN_SECTION : TOKEN_TITLE; 
                u64 amount = skip_while(parser, is_hash);
                if (amount == 1 || amount == 2) {
                    struct string_view title = extract_while(parser, not_new_line);
                    result.item = alloc_buffer(title);
                } else {
                    fprintf(stderr, "%s%s\n", errormsg(), "title");
                    parser->has_error += 1;
                }
            } break;
            case '!': {
                result.kind = TOKEN_FIGURE;
                if (p.next_letter == '[') {
                    parser->cursor += 2;
                    struct string_view alt_text = extract_while(parser, not_close_bracket);
                    if (skip_while(parser, is_close_bracket) == 1 && skip_while(parser, is_open_parenthesis) == 1) {
                        struct string_view info = extract_while(parser, not_close_parenthesis);
                        struct markdown_figure figure = parse_markdown_image(alt_text, info);
                        result.figure = figure;
                    } else {
                        fprintf(stderr, "%s%s\n", errormsg(), "figure");
                        parser->has_error += 1;
                    }
                } else {
                    fprintf(stderr, "%s%s\n", errormsg(), "alt text delimiter");
                    parser->has_error += 1;
                }
            } break;
            case '$': {
                result.kind = p.next_letter == '$' ? TOKEN_EQUATION : TOKEN_INLINE_EQUATION;
                u64 amount = skip_while(parser, is_dollar);
                if (amount == 1 || amount == 2) {
                    struct string_view equation = extract_while(parser, is_equation);
                    result.item = alloc_buffer(equation);
                    
                    u64 remaining = skip_while(parser, is_dollar);
                    if (remaining != 1 && remaining != 2)
                        fprintf(stderr, "%s%s\n", errormsg(), "equation");
                } else {
                    fprintf(stderr, "%s%s\n", errormsg(), "equation");
                    parser->has_error += 1;
                }
            } break;
            case '[': {
                result.kind = TOKEN_LINK; 
                if (skip_while(parser, is_open_bracket) == 1) {
                    struct string_view link_text = extract_while(parser, not_close_bracket);
                    if (skip_while(parser, is_open_parenthesis) == 1) {
                        struct string_view url = extract_while(parser, not_close_parenthesis);
                        result.link = (struct markdown_link) {alloc_buffer(link_text), alloc_buffer(url)};
                        if (skip_while(parser, is_close_parenthesis) != 1) {
                            fprintf(stderr, "%s%s\n", errormsg(), "link");
                            parser->has_error += 1;
                        }
                    } else {
                        fprintf(stderr, "%s%s\n", errormsg(), "link");
                        parser->has_error += 1;
                    }
                } else {
                    fprintf(stderr, "%s%s\n", errormsg(), "link");
                    parser->has_error += 1;
                }
            } break;
            case '-': 
            case '*':
            case '+': {
                result.kind = TOKEN_LIST_ELEMENT;
                if (skip_while(parser, is_list_identifier) == 1) {
                    struct string_view item = extract_while(parser, not_new_line);
                    result.element = (struct markdown_list_element) {0, alloc_buffer(item)};
                } else {
                    fprintf(stderr, "%s%s\n", errormsg(), "list element");
                    parser->has_error += 1;
                }
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
                result.kind = TOKEN_LIST_ELEMENT;
                struct string_view number = extract_while(parser, is_numeric);
                if (number.size != 0) {
                    u64 parsed_number = svtoint(number);
                    if (parsed_number > 0 && skip_while(parser, is_dot) == 1) {
                        struct string_view item = extract_while(parser, not_new_line);
                        result.element = (struct markdown_list_element) {parsed_number, alloc_buffer(item)};
                    } else { 
                        fprintf(stderr, "%s%s\n", errormsg(), "list element");
                        parser->has_error += 1;
                    }
                } else {
                    fprintf(stderr, "%s%s\n", errormsg(), "list number");
                    parser->has_error += 1;
                }
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
    if (n) {
        n->token = t;
        n->children = 0;
    }
    return n;
}


internal struct markdown_tree alloc_markdown_tree()
{
    struct markdown_tree result = {0};
    result.head = alloc_markdown_node(
        (struct markdown_token) {0, TOKEN_HEAD, alloc_buffer(sv("Markdown tree"))}
    );
    result.current  = result.head;
    result.previous = result.head;
    return result;
}


internal b32 add_node(struct markdown_tree *t, struct markdown_token token)
{
    struct markdown_node *n;
    if ((n = alloc_markdown_node(token))) {
        if (t->current->token.kind == token.kind) {
            t->current->next_sibling = n;
        } else {
            t->current = t->previous;
            t->current->children = n;
            t->previous = t->current;
        }
        t->current = n;
        return 1;
    };
    return 0;
}


int main(int argc, char **argv) 
{
#define TEST_FILE "PageParser/tests/unordered_list.md"
    struct strbuffer contents = read_entire_file(TEST_FILE);
    if (contents.size == 0) {
        fprintf(stderr, "[ERROR] Input file is empty. Nothing to do.");
        exit(0);
    }

    global_arena = arena_init(MEGA(2));

    struct markdown_parser parser = {
        .contents  = contents.buffer, 
        .has_error = 0,
        .length    = contents.size, 
        .cursor    = 0
    };
    
    struct markdown_tree doc = alloc_markdown_tree();
    do {
        struct markdown_token t = parse_token(&parser);
        parser.has_error += !add_node(&doc, t); 
    } while (is_parsing(parser));

    free_arena(&global_arena);
    free_strbuffer(&contents);
    return 0;
}