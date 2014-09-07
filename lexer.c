
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>

#include "lexer.h"

//i.e. what can end a ref?
#define IS_DELIM(X) (X == '(' || X == ')' || X == '=' || X == ';' || X == '.' || X == '|' || X == ':' || X == ' ')
#define IS_TOK(X)  X!='#' && X!='(' && X!=')' && X!='{' && X!='}' && X!='|' && X!='\\' && X!='.' && X!=';' && X!='=' && X!='^'  && X!= ':' && X!= '\''

typedef struct {
	int tid;
	char* value;
} tk;


int bflmt = 100;
char* buffer;
char* code;
char* codePoint;
char* eofcode;
char* oldptr;
int flag_operator = 0;

void clex_load(char* prg) {
	code = prg;
	buffer = malloc(bflmt);
	buffer[0] = '\0';
	eofcode = (code + strlen(code));
}

char* clex_tok_value() {
	return buffer;
}

void clex_putback() {
	code = oldptr;
}

int clex_tok() {
	oldptr = code;
	char c;
	int anticrash = 0;
	int i = 0;
	int comment_mode = 0;
	buffer[0] = '\0';
	c = *code;
	while((anticrash++) < 200 && code != eofcode && (isspace(c) || comment_mode)) {
		if (c == '\n') comment_mode = 0;
		if (c == '#') comment_mode = 1;
		code ++;
		c = *code;
	}
	if (code == eofcode) return FIN;
	if (c == '(') { code++; return PAROPEN; }
	if (c == ')') { code++; return PARCLOSE; }
	if (c == '{') { code++; return BLOCKOPEN; }
	if (c == '}') {  code++; return BLOCKCLOSE; }
	if (c == '|' || c == '\\') { code++; return BLOCKPIPE; }
	if (c == '.') { code++; return DOT; }
	if (c == ',') { code++; return CHAIN; }
	if (c == ':') { code++; return COLON; }
	if (c == '=' && (code+1)<eofcode && (*(code+1)!='=')) { 
		code++;
		return ASSIGNMENT; 
	}
	if (c == '^') { code++; return RET; }
	if (c == '\'') { code++; return QUOTE; }
	if ((c == '-' && (code+1)<eofcode && isdigit(*(code+1))) || isdigit(c)) {
		buffer[i] = c;
		i++;
		code++;
		c = *code;
		while((anticrash++)<100 && (isdigit(c))) {
			buffer[i] = c;
			i++;
			code++;
			c = *code;
		}
		if (c=='.' && (code+1 < eofcode) && !isdigit(*(code+1))) {
			buffer[i] = '\0';
			return NUMBER;
		}
		if (c=='.') {
			buffer[i] = c;
			i++;
			code++;
			c = *code;
		}
		while((anticrash++)<100 && (isdigit(c))) {
			buffer[i] = c;
			i++;
			code++;
			c = *code;
		}
		buffer[i] = '\0';
		return NUMBER;
	}
	if (strncmp(code, "True", 4)==0){
		if (IS_DELIM(*(code + 4))) { 
			code += 4;
			return BOOLEANYES;
		}
	}
	if (strncmp(code, "False", 5)==0){
		if (IS_DELIM(*(code + 5))) { 
			code += 5;
			return BOOLEANNO;
		}
	}
	if (strncmp(code, "Nil", 3)==0){
		if (IS_DELIM(*(code + 3))) { 
			code += 3;
			return NIL;
		}
	}
	if (strncmp(code, ">=", 2)==0){
		if (IS_TOK(*(code + 2))) { 
			code += 2;
			strcat(buffer, ">=\0");
			return REF;
		}
	}
	if (strncmp(code, "<=", 2)==0){
		if (IS_TOK(*(code + 2))) { 
			code += 2;
			strcat(buffer, "<=\0");
			return REF;
		}
	}
	if (strncmp(code, "==", 2)==0){
		if (IS_TOK(*(code + 2))) { 
			code += 2;
			strcat(buffer, "==\0");
			return REF;
		}
	}
	while((anticrash++) < 100 && !isspace(c) && IS_TOK(c) && code!=eofcode) {
		buffer[i] = c;
		i++;
		if (i > bflmt) {
			printf("[ERROR L001]: Token Buffer Exausted. Tokens may not exceed 100 bytes.");
			exit(1);
		}
		code++;
		c = *code;
	}

	buffer[i] = '\0';
	return REF;
}

char* clex_readstr() {
	char* strbuff = malloc(100);
	int antiCrash = 0;
	char c = *code;
	int escape = 0;
	char* beginbuff = strbuff;
	while((antiCrash++)<100 && (c != '\'' || escape == 1)) {
		if (c == '\\' && escape == 0) {
			escape = 1;
			code++;
			c = *code;
			continue;
		}
		escape = 0;
		*strbuff = c;
		strbuff++;
		code++;
		c = *code;
	}
	*strbuff = '\0';
	return beginbuff;
}
