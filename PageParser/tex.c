#include "page_parser.h"


#define PATH_SEP '/'
#define PATH_SEP_STRING ((char[]){PATH_SEP, 0})


struct strpath {
    usize length;
    u8 path[_MAX_PATH];
};


PGDEF struct strpath replace(struct strpath p, char *find, char *replace)
{
    struct strpath result = {0};

    usize fs = strlen(find);
    usize rl = strlen(replace);
    i64 new_length = p.length + rl - fs;
    if (fs < p.length && new_length > 0 && new_length < _MAX_PATH) {
        usize el = p.length - fs; 

        usize i = 0;
        while (i < el && strncmp(p.path + i, find, fs) != 0)
            i++;

        memcpy(result.path, p.path, i);
        memcpy(result.path + i, replace, rl); 
        memcpy(result.path + i + rl, p.path + i + fs, p.length - i - fs);
        result.length = p.length + rl - fs;
    } else {
        return p;
    }
    return result;
}


PGDEF void move(struct strpath *d, struct strpath s)
{
    memset(d->path, 0, _MAX_PATH);
    memcpy(d->path, s.path, s.length);
    d->length = s.length;
}


PGDEF struct strpath basename(struct strpath p)
{
    struct strpath result = {0};

    usize i = p.length - 1;
    while (i >= 0 && p.path[i] != PATH_SEP)
        i--;

    memcpy(result.path, p.path + i + 1, p.length - i);
    result.length = i + 1;
    return result;
}


PGDEF struct strpath dirname(struct strpath p)
{
    struct strpath result = {0};

    usize i = p.length - 1;
    while (i >= 0 && p.path[i] != PATH_SEP)
        i--;

    memcpy(result.path, p.path, i);
    result.length = i;
    return result;
}


PGDEF struct strpath concat(struct strpath base, struct strpath name)
{
    struct strpath result = {0};
    usize result_length = base.length + name.length + 1;
    if (result_length < _MAX_PATH) {
        memcpy(result.path, base.path, base.length);
        memcpy(result.path, PATH_SEP_STRING, strlen(PATH_SEP_STRING));
        memcpy(result.path, name.path, name.length);
        result.length = result_length;
    }
    return result;
}


PGDEF struct strpath from_cstr(char *cstr) 
{
    struct strpath result = {0};
    usize s = strlen(cstr);
    if (strlen(cstr) < _MAX_PATH) {
        memcpy(result.path, cstr, s);
        result.length = s;
    }
    return result;
}


internal void write_header(FILE *f)
{
    if (f) {
        fprintf(f, "\\documentclass[12pt]{article}\n");
        fprintf(f, "\\usepackage{amsmath}\n");
        fprintf(f, "\n");
        fprintf(f, "\n");
        fprintf(f, "\\begin{document}\n");
    }
}


internal void write_end_document(FILE *f)
{
    if (f) {
        fprintf(f, "\\end{document}\n");
    }
}


internal void emit_tex(char *filepath, struct markdown_tree t) 
{

}