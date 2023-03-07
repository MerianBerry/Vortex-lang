#pragma once

enum Tokens
{
    tok_new = -400,
    tok_glob,
    tok_func,
    tok_fcall,
    tok_ret,
    tok_true,
    tok_false,
    tok_if,
    tok_else,
    tok_while,
    tok_for,
    tok_break,
    tok_scope,
    tok_extern,
    tok_number, // start Identity stuff
    tok_string,
    tok_ident, // end Identity stuff
    tok_op,
    tok_1lc,
    tok_mlc,
    tok_mlce
};