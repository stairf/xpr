/*****
 * Copyright (c) 2015-2016, Stefan Reif
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****/


#ifdef DEBUG
#  define dbg(...) printf(__VA_ARGS__)

static inline void dbg_dump_tok(const char *name, const tok *t, const char *end)
{
	if (name)
		dbg("%s: ", name);

	if (CLASS_VALUE == CLASS_GET(t->tag)) {
		switch (t->tag) {
		case BS_SET(BS_NONE,  TK_NUM):  dbg("0[%lf]", t->data.value); goto out;
		case BS_SET(BS_OPEN,  TK_NUM):  dbg("1[%lf]", t->data.value); goto out;
		case BS_SET(BS_PLUS,  TK_NUM):  dbg("2[%lf]", t->data.value); goto out;
		case BS_SET(BS_MUL,   TK_NUM):  dbg("3[%lf]", t->data.value); goto out;
		case BS_SET(BS_EXP,   TK_NUM):  dbg("4[%lf]", t->data.value); goto out;
		case BS_SET(BS_UNARY, TK_NUM):  dbg("5[%lf]", t->data.value); goto out;
		default:                        dbg("?[%lf]", t->data.value); goto out;
		}
	} else if (CLASS_FUNC == CLASS_GET(t->tag)) {
		switch(BS_IGN(t->tag)) {
		case TK_FUN_ACOS:               dbg("acos");                  goto out;
		case TK_FUN_ASIN:               dbg("asin");                  goto out;
		case TK_FUN_ATAN:               dbg("atan");                  goto out;
		case TK_FUN_ACOSH:              dbg("acosh");                 goto out;
		case TK_FUN_ASINH:              dbg("asinh");                 goto out;
		case TK_FUN_ATANH:              dbg("atanh");                 goto out;
		case TK_FUN_COS:                dbg("cos");                   goto out;
		case TK_FUN_SIN:                dbg("sin");                   goto out;
		case TK_FUN_TAN:                dbg("tan");                   goto out;
		case TK_FUN_COSH:               dbg("cosh");                  goto out;
		case TK_FUN_SINH:               dbg("sinh");                  goto out;
		case TK_FUN_TANH:               dbg("tanh");                  goto out;
		case TK_FUN_LOG:                dbg("log");                   goto out;
		case TK_FUN_EXP:                dbg("exp");                   goto out;
		case TK_FUN_SQRT:               dbg("sqrt");                  goto out;
		case TK_FUN_CBRT:               dbg("cbrt");                  goto out;
		case TK_FUN_MIN:                dbg("min");                   goto out;
		case TK_FUN_MAX:                dbg("max");                   goto out;
		default:                        dbg("?()");                   goto out;
		}
	} else {
		switch (BS_IGN(t->tag)) {
		case BS_IGN(TK_SPACE):          dbg("[ ]");                   goto out;
		case BS_IGN(TK_EOF):            dbg("[eof]");                 goto out;
		case BS_IGN(TK_ERR):            dbg("[err]");                 goto out;
		case BS_IGN(TK_PLUS):           dbg("+");                     goto out;
		case BS_IGN(TK_MINUS):          dbg("-");                     goto out;
		case BS_IGN(TK_UPLUS):          dbg("U+");                    goto out;
		case BS_IGN(TK_UMINUS):         dbg("U-");                    goto out;
		case BS_IGN(TK_MUL):            dbg("*");                     goto out;
		case BS_IGN(TK_DIV):            dbg("/");                     goto out;
		case BS_IGN(TK_EXP):            dbg("^");                     goto out;
		case BS_IGN(TK_OPEN):           dbg("(");                     goto out;
		case BS_IGN(TK_CLOSE):          dbg(")");                     goto out;
		case BS_IGN(TK_COMMA):          dbg(",");                     goto out;
		default:                        dbg("%x?", t->tag);           goto out;
		}
	}
out:
	if (end)
		dbg("%s", end);
}

#else
#  define dbg(...)((void)0)

static inline void dbg_dump_tok(const char *name, const tok *t, const char *end)
{
	(void) name;
	(void) t;
	(void) end;
}
#endif

