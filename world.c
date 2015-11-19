
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include "citrine.h"

ctr_object* World = NULL;
ctr_object* contexts[100];
int cid = 0;
ctr_object* Object;
ctr_object* CBlock;
ctr_object* TextString;
ctr_object* Number;
ctr_object* BoolX;
ctr_object* Console;
ctr_object* Nil;
ctr_object* GC;
ctr_object* CMap;
ctr_object* CArray;
ctr_object* CFile;
ctr_object* error;
ctr_object* CSystem;
ctr_object* CDice;
ctr_object* CCommand;
ctr_object* CShell;
ctr_object* CCoin;
ctr_object* CClock;

int gc_dust = 0;
int gc_object_count = 0;
int debug;

//measures the size of character
int utf8size(char c) {
	if ((c & CTR_UTF8_BYTE3) == CTR_UTF8_BYTE3) return 4;
	if ((c & CTR_UTF8_BYTE2) == CTR_UTF8_BYTE2) return 3;
	if ((c & CTR_UTF8_BYTE1) == CTR_UTF8_BYTE1) return 2;
	return 1;
}
//measures the length of an utf8 string in utf8 chars
long getutf8len(char* strval, long max) {
	long i;
	long j = 0;
	int s = 0;
	for(i = 0; i < max; i++) {
		s = utf8size(strval[i]);
		j += (s - 1);
	}
	return (i-j);
}

long getBytesUtf8(char* strval, long startByte, long lenUChar) {
	long i = 0;
	long bytes = 0;
	int s = 0;
	int x = 0;
	long index = 0;
	while(x < lenUChar) {
		index = startByte + i;
		char c = strval[index];
		s = utf8size(c);
		bytes = bytes + s;
		i = i + s;
		x ++;
	}
	return bytes;
}

char* readf(char* file_name) {
   char* prg;
   char ch;
   FILE* fp;
   fp = fopen(file_name,"r");
   if( fp == NULL )
   {
      printf("Error while opening the file.\n");
      exit(1);
   }
   int prev = ftell(fp);
   fseek(fp,0L,SEEK_END);
   int sz = ftell(fp);
   fseek(fp,prev,SEEK_SET);
   prg = malloc((sz+1)*sizeof(char));
   ctr_program_length=0;
   while( ( ch = fgetc(fp) ) != EOF ) prg[ctr_program_length++]=ch;
   fclose(fp);
   return prg;
}

void tree(ctr_tnode* ti, int indent) {
	if (indent>20) exit(1); 
	ctr_tlistitem* li = ti->nodes;
	ctr_tnode* t = li->node;
	while(1) {
		int i = 0;
		for (i=0; i<indent; i++) printf(" ");
		char* str = calloc(40, sizeof(char));
		if (t->type == CTR_AST_NODE_EXPRASSIGNMENT) 		str = "ASSIGN\0";
		else if (t->type == CTR_AST_NODE_EXPRMESSAGE) 	str = "MESSAG\0";
		else if (t->type == CTR_AST_NODE_UNAMESSAGE) 	str = "UMSSAG\0";
		else if (t->type == CTR_AST_NODE_KWMESSAGE) 		str = "KMSSAG\0";
		else if (t->type == CTR_AST_NODE_BINMESSAGE) 	str = "BMSSAG\0";
		else if (t->type == CTR_AST_NODE_LTRSTRING) 		str = "STRING\0";
		else if (t->type == CTR_AST_NODE_REFERENCE) 		str = "REFRNC\0";
		else if (t->type == CTR_AST_NODE_LTRNUM) 		str = "NUMBER\0";
		else if (t->type == CTR_AST_NODE_CODEBLOCK) 		str = "CODEBL\0";
		else if (t->type == CTR_AST_NODE_RETURNFROMBLOCK)str = "RETURN\0";
		else if (t->type == CTR_AST_NODE_PARAMLIST)		str = "PARAMS\0";
		else if (t->type == CTR_AST_NODE_INSTRLIST)		str = "INSTRS\0";
		else if (t->type == CTR_AST_NODE_ENDOFPROGRAM)	str = "EOPROG\0";
		else if (t->type == CTR_AST_NODE_NESTED)	        str = "NESTED\0";
		else if (t->type == CTR_AST_NODE_LTRBOOLFALSE)	str = "BFALSE\0";
		else if (t->type == CTR_AST_NODE_LTRBOOLTRUE)	str = "BLTRUE\0";
		else if (t->type == CTR_AST_NODE_LTRNIL)	        str = "LTRNIL\0";
		else 								str = "UNKNW?\0";
		printf("%d:%s %s (vlen: %lu) \n", t->type, str, t->value, t->vlen);
		if (t->nodes) tree(t, indent + 1);
		if (!li->next) break; 
		li = li->next;
		t = li->node;
	}
}


int ctr_internal_object_is_equal(ctr_object* object1, ctr_object* object2) {
	
	if (object1->info.type == CTR_OBJECT_TYPE_OTSTRING && object2->info.type == CTR_OBJECT_TYPE_OTSTRING) {
		char* string1 = object1->value.svalue->value;
		char* string2 = object2->value.svalue->value;
		long len1 = object1->value.svalue->vlen;
		long len2 = object2->value.svalue->vlen;
		if (len1 != len2) return 0;
		int d = memcmp(string1, string2, len1);
		if (d==0) return 1;
		return 0;
	}
	
	if (object1->info.type == CTR_OBJECT_TYPE_OTNUMBER && object2->info.type == CTR_OBJECT_TYPE_OTNUMBER) {
		ctr_number num1 = object1->value.nvalue;
		ctr_number num2 = object2->value.nvalue;
		if (num1 == num2) return 1;
		return 0;
	}
	
	if (object1->info.type == CTR_OBJECT_TYPE_OTBOOL && object2->info.type == CTR_OBJECT_TYPE_OTBOOL) {
		int b1 = object1->value.bvalue;
		int b2 = object2->value.bvalue;
		if (b1 == b2) return 1;
		return 0;
	}
	
	if (object1 == object2) return 1;
	return 0;
		
}

ctr_object* ctr_internal_object_find_property(ctr_object* owner, ctr_object* key, int is_method) {
	
	ctr_mapitem* head;
	if (is_method) {
		if (owner->methods->size == 0) {
			return NULL;
		}
		head = owner->methods->head; 
	} else {
		if (owner->properties->size == 0) {
			return NULL;
		}
		head = owner->properties->head; 
	}
	
	while(head) {
		if (ctr_internal_object_is_equal(head->key, key)) {
			return head->value;
		}
		head = head->next;
	}
	return NULL;
}


void ctr_internal_object_delete_property(ctr_object* owner, ctr_object* key, int is_method) {
	ctr_mapitem* head;
	if (is_method) {
		if (owner->methods->size == 0) {
			return;
		}
		head = owner->methods->head; 
	} else {
		if (owner->properties->size == 0) {
			return;
		}
		head = owner->properties->head; 
	}
	
	while(head) {
		if (ctr_internal_object_is_equal(head->key, key)) {
			if (head->next && head->prev) {
				head->next->prev = head->prev;
				head->prev->next = head->next;
			} else {
				if (head->next) {
					head->next->prev = NULL;
				}
				if (head->prev) {
					head->prev->next = NULL;
				}
			}
			if (is_method) {
				if (owner->methods->head == head) {
					if (head->next) {
						owner->methods->head = head->next;
					} else {
						owner->methods->head = NULL;
					}
				}
				owner->methods->size --; 
			} else {
				if (owner->properties->head == head) {
					if (head->next) {
						owner->properties->head = head->next;
					} else {
						owner->properties->head = NULL;
					}
				}
				owner->properties->size --;
			}
			return;
		}
		head = head->next;
	}
	return;
}

void ctr_internal_object_add_property(ctr_object* owner, ctr_object* key, ctr_object* value, int m) {
	ctr_mapitem* new_item = malloc(sizeof(ctr_mapitem));
	ctr_mapitem* current_head = NULL;
	new_item->key = key;
	new_item->value = value;
	new_item->next = NULL;
	new_item->prev = NULL;
	if (m) {
		if (owner->methods->size == 0) {
			owner->methods->head = new_item;
		} else {
			current_head = owner->methods->head;
			current_head->prev = new_item;
			new_item->next = current_head;
			owner->methods->head = new_item;
		}
		owner->methods->size ++;
	} else {
		if (owner->properties->size == 0) {
			owner->properties->head = new_item;
		} else {
			current_head = owner->properties->head;
			current_head->prev = new_item;
			new_item->next = current_head;
			owner->properties->head = new_item;
		}
		owner->properties->size ++;
	}
}

void ctr_internal_object_set_property(ctr_object* owner, ctr_object* key, ctr_object* value, int is_method) {
	ctr_internal_object_delete_property(owner, key, is_method);
	ctr_internal_object_add_property(owner, key, value, is_method);
}


char* ctr_internal_memmem(char* haystack, long hlen, char* needle, long nlen, int reverse ) {
	char* cur;
	char* last;
	char* begin;
	int step = (1 - reverse * 2); //1 if reverse = 0, -1 if reverse = 1
	if (nlen == 0) return NULL;
	if (hlen == 0) return NULL;
	if (hlen < nlen) return NULL;
	if (!reverse) {
		begin = haystack;
		last = haystack + hlen - nlen + 1;
	} else {
		begin = haystack + hlen;
		last = haystack + nlen - 2;
	}
	for(cur = begin; cur!=last; cur += step) {
		if (memcmp(cur,needle,nlen) == 0) return cur;
	}
	return NULL;
}

ctr_object* ctr_internal_create_object(int type) {
	ctr_object* o = malloc(sizeof(ctr_object));
	o->properties = malloc(sizeof(ctr_map));
	o->methods = malloc(sizeof(ctr_map));
	o->properties->size = 0;
	o->methods->size = 0;
	o->properties->head = NULL;
	o->methods->head = NULL;
	o->info.type = type;
	o->info.sticky = 1;
	o->info.mark = 0;
	if (type==CTR_OBJECT_TYPE_OTBOOL) o->value.bvalue = 0;
	if (type==CTR_OBJECT_TYPE_OTNUMBER) o->value.nvalue = 0;
	if (type==CTR_OBJECT_TYPE_OTSTRING) {
		o->value.svalue = malloc(sizeof(ctr_string));
		o->value.svalue->value = "";
		o->value.svalue->vlen = 0;
	}
	o->gnext = NULL;
	if (ctr_first_object == NULL) {
		ctr_first_object = o;
	} else {
		o->gnext = ctr_first_object;
		ctr_first_object = o;
	}
	return o;
	
}

void ctr_internal_create_func(ctr_object* o, ctr_object* key, void* f ) {
	ctr_object* methodObject = ctr_internal_create_object(CTR_OBJECT_TYPE_OTNATFUNC);
	methodObject->value.rvalue = (void*) f;
	ctr_internal_object_add_property(o, key, methodObject, 1);
}

ctr_object* ctr_internal_cast2number(ctr_object* o) {
	if (o->info.type == CTR_OBJECT_TYPE_OTNUMBER) return o;
	if (o->info.type == CTR_OBJECT_TYPE_OTSTRING) {
		char* cstring = malloc((o->value.svalue->vlen+1)*sizeof(char));
		memcpy(cstring, o->value.svalue->value, o->value.svalue->vlen);
		memcpy((char*)((long)cstring+(o->value.svalue->vlen*sizeof(char))),"\0", sizeof(char));
		return ctr_build_number(cstring);
	}
	return ctr_build_number("0");
}

ctr_object* ctr_internal_cast2string( ctr_object* o ) {
	if (o->info.type == CTR_OBJECT_TYPE_OTSTRING) return o;
	else if (o->info.type == CTR_OBJECT_TYPE_OTNIL) { return ctr_build_string("[Nil]", 5); }
	else if (o->info.type == CTR_OBJECT_TYPE_OTBOOL && o->value.bvalue == 1) { return ctr_build_string("[True]", 6); }
	else if (o->info.type == CTR_OBJECT_TYPE_OTBOOL && o->value.bvalue == 0) { return ctr_build_string("[False]", 7); }
	else if (o->info.type == CTR_OBJECT_TYPE_OTNUMBER) {
		char* s = calloc(80, sizeof(char));
		CTR_CONVFP(s,o->value.nvalue);
		int slen = strlen(s);
		return ctr_build_string(s, slen);
	}
	else if (o->info.type == CTR_OBJECT_TYPE_OTBLOCK) { return ctr_build_string("[Block]",7);}
	else if (o->info.type == CTR_OBJECT_TYPE_OTOBJECT) { return ctr_build_string("[Object]",8);}
	return ctr_build_string("[?]", 3);
}

ctr_object* ctr_internal_cast2bool( ctr_object* o ) {
	if (o->info.type == CTR_OBJECT_TYPE_OTBOOL) return o;
	if (o->info.type == CTR_OBJECT_TYPE_OTNIL
		|| (o->info.type == CTR_OBJECT_TYPE_OTNUMBER && o->value.nvalue == 0)
		|| (o->info.type == CTR_OBJECT_TYPE_OTSTRING && o->value.svalue->vlen == 0)) return ctr_build_bool(0);
	return ctr_build_bool(1);
}


void ctr_open_context() {
	cid++;
	ctr_object* context = ctr_internal_create_object(CTR_OBJECT_TYPE_OTOBJECT);
	contexts[cid] = context;
}

void ctr_close_context() {
	if (cid == 0) return;
	cid--;
}

ctr_object* ctr_find(ctr_object* key) {
	int i = cid;
	ctr_object* foundObject = NULL;
	while((i>-1 && foundObject == NULL)) {
		ctr_object* context = contexts[i];
		foundObject = ctr_internal_object_find_property(context, key, 0);
		i--;
	}
	if (foundObject == NULL) { printf("Error, key not found: [%s].\n", key->value.svalue->value); exit(1); }
	return foundObject;
}

ctr_object* ctr_find_in_my(ctr_object* key) {
	ctr_object* context = ctr_find(ctr_build_string("me",2));
	ctr_object* foundObject = ctr_internal_object_find_property(context, key, 0);
	if (foundObject == NULL) { printf("Error, property not found: %s.\n", key->value.svalue->value); exit(1); }
	return foundObject;
}

void ctr_set(ctr_object* key, ctr_object* object) {
	ctr_object* context = contexts[cid];
	ctr_internal_object_set_property(context, key, object, 0);
}


#include "base.c"
#include "system.c"
#include "collections.c"
#include "file.c"

/**
 * Populate the World of Citrine.
 */
void ctr_initialize_world() {
	srand((unsigned)time(NULL));
	
	ctr_first_object = NULL;
	
	World = ctr_internal_create_object(CTR_OBJECT_TYPE_OTOBJECT);
	contexts[0] = World;

	//Object
	Object = ctr_internal_create_object(CTR_OBJECT_TYPE_OTOBJECT);
	ctr_internal_create_func(Object, ctr_build_string("new", 3), &ctr_object_make);
	ctr_internal_create_func(Object, ctr_build_string("equals:", 7), &ctr_object_equals);
	ctr_internal_create_func(Object, ctr_build_string("on:do:", 6), &ctr_object_on_do);
	ctr_internal_create_func(Object, ctr_build_string("override:do:", 12), &ctr_object_override_does);
	ctr_internal_create_func(Object, ctr_build_string("basedOn:", 8), &ctr_object_basedOn);
	ctr_internal_create_func(Object, ctr_build_string("respondTo:", 10), &ctr_object_respond);
	ctr_internal_create_func(Object, ctr_build_string("respondTo:with:", 15), &ctr_object_respond);
	ctr_internal_create_func(Object, ctr_build_string("respondTo:with:and:", 19), &ctr_object_respond);
	ctr_internal_create_func(Object, ctr_build_string("type", 4), &ctr_object_type);
	ctr_internal_create_func(Object, ctr_build_string("new", 3), &ctr_object_make);
	ctr_internal_object_add_property(World, ctr_build_string("Object", 6), Object, 0);
	Object->link = NULL;

	//Nil
	Nil = ctr_internal_create_object(CTR_OBJECT_TYPE_OTNIL);
	ctr_internal_object_add_property(World, ctr_build_string("Nil", 3), Nil, 0);
	Nil->link = Object;

	//Boolean
	BoolX = ctr_internal_create_object(CTR_OBJECT_TYPE_OTBOOL);
	ctr_internal_create_func(BoolX, ctr_build_string("ifTrue:", 7), &ctr_bool_iftrue);
	ctr_internal_create_func(BoolX, ctr_build_string("ifFalse:", 8), &ctr_bool_ifFalse);
	ctr_internal_create_func(BoolX, ctr_build_string("else:", 5), &ctr_bool_ifFalse);
	ctr_internal_create_func(BoolX, ctr_build_string("opposite", 8), &ctr_bool_opposite);
	ctr_internal_create_func(BoolX, ctr_build_string("∧", 3), &ctr_bool_and);
	ctr_internal_create_func(BoolX, ctr_build_string("∨", 3), &ctr_bool_or);
	ctr_internal_create_func(BoolX, ctr_build_string("xor:", 4), &ctr_bool_xor);
	ctr_internal_create_func(BoolX, ctr_build_string("toNumber", 8), &ctr_bool_to_number);
	ctr_internal_create_func(BoolX, ctr_build_string("toString", 8), &ctr_bool_to_string);
	ctr_internal_object_add_property(World, ctr_build_string("Boolean", 7), BoolX, 0);
	BoolX->link = Object;

	//Number
	Number = ctr_internal_create_object(CTR_OBJECT_TYPE_OTNUMBER);
	ctr_internal_create_func(Number, ctr_build_string("+", 1), &ctr_number_add);
	ctr_internal_create_func(Number, ctr_build_string("inc:",4), &ctr_number_inc);
	ctr_internal_create_func(Number, ctr_build_string("-",1), &ctr_number_minus);
	ctr_internal_create_func(Number, ctr_build_string("dec:",4), &ctr_number_dec);
	ctr_internal_create_func(Number, ctr_build_string("*",1),&ctr_number_multiply);
	ctr_internal_create_func(Number, ctr_build_string("mul:",4),&ctr_number_mul);
	ctr_internal_create_func(Number, ctr_build_string("/",1), &ctr_number_divide);
	ctr_internal_create_func(Number, ctr_build_string("div:",4),&ctr_number_div);
	ctr_internal_create_func(Number, ctr_build_string(">",1),&ctr_number_higherThan);
	ctr_internal_create_func(Number, ctr_build_string("≥",3),&ctr_number_higherEqThan);
	ctr_internal_create_func(Number, ctr_build_string("<",1),&ctr_number_lowerThan);
	ctr_internal_create_func(Number, ctr_build_string("≤",3),&ctr_number_lowerEqThan);
	ctr_internal_create_func(Number, ctr_build_string("=",1),&ctr_number_eq);
	ctr_internal_create_func(Number, ctr_build_string("≠",3),&ctr_number_neq);
	ctr_internal_create_func(Number, ctr_build_string("%",1),&ctr_number_modulo);
	ctr_internal_create_func(Number, ctr_build_string("times:",6),&ctr_number_times);
	ctr_internal_create_func(Number, ctr_build_string("factorial",9),&ctr_number_factorial);
	ctr_internal_create_func(Number, ctr_build_string("floor",5),&ctr_number_floor);
	ctr_internal_create_func(Number, ctr_build_string("ceil",4),&ctr_number_ceil);
	ctr_internal_create_func(Number, ctr_build_string("round",5),&ctr_number_round);
	ctr_internal_create_func(Number, ctr_build_string("abs",3),&ctr_number_abs);
	ctr_internal_create_func(Number, ctr_build_string("sin",3),&ctr_number_sin);
	ctr_internal_create_func(Number, ctr_build_string("cos",3),&ctr_number_cos);
	ctr_internal_create_func(Number, ctr_build_string("exp",3),&ctr_number_exp);
	ctr_internal_create_func(Number, ctr_build_string("sqrt",4),&ctr_number_sqrt);
	ctr_internal_create_func(Number, ctr_build_string("tan",3),&ctr_number_tan);
	ctr_internal_create_func(Number, ctr_build_string("atan",4),&ctr_number_atan);
	ctr_internal_create_func(Number, ctr_build_string("log",3),&ctr_number_log);
	ctr_internal_create_func(Number, ctr_build_string("pow:",4),&ctr_number_pow);
	ctr_internal_create_func(Number, ctr_build_string("min:",4),&ctr_number_min);
	ctr_internal_create_func(Number, ctr_build_string("max:",4),&ctr_number_max);
	ctr_internal_create_func(Number, ctr_build_string("toString", 8), &ctr_number_to_string);
	ctr_internal_create_func(Number, ctr_build_string("toBoolean", 9), &ctr_number_to_boolean);
	ctr_internal_create_func(Number, ctr_build_string("between:and:",12),&ctr_number_between);
	ctr_internal_object_add_property(World, ctr_build_string("Number", 6), Number, 0);
	Number->link = Object;

	//String
	TextString = ctr_internal_create_object(CTR_OBJECT_TYPE_OTSTRING);
	ctr_internal_create_func(TextString, ctr_build_string("bytes", 5), &ctr_string_bytes);
	ctr_internal_create_func(TextString, ctr_build_string("codePoints", 10), &ctr_string_length);
	ctr_internal_create_func(TextString, ctr_build_string("from:to:", 8), &ctr_string_fromto);
	ctr_internal_create_func(TextString, ctr_build_string("from:length:", 12), &ctr_string_from_length);
	ctr_internal_create_func(TextString, ctr_build_string("+", 1), &ctr_string_concat);
	ctr_internal_create_func(TextString, ctr_build_string("=", 1), &ctr_string_eq);
	ctr_internal_create_func(TextString, ctr_build_string("≠", 3), &ctr_string_neq);
	ctr_internal_create_func(TextString, ctr_build_string("trim", 4), &ctr_string_trim);
	ctr_internal_create_func(TextString, ctr_build_string("ltrim", 5), &ctr_string_ltrim);
	ctr_internal_create_func(TextString, ctr_build_string("rtrim", 5), &ctr_string_rtrim);
	ctr_internal_create_func(TextString, ctr_build_string("htmlEscape", 10), &ctr_string_html_escape);
	ctr_internal_create_func(TextString, ctr_build_string("at:", 3), &ctr_string_at);
	ctr_internal_create_func(TextString, ctr_build_string("byteAt:", 7), &ctr_string_byte_at);
	ctr_internal_create_func(TextString, ctr_build_string("indexOf:", 8), &ctr_string_index_of);
	ctr_internal_create_func(TextString, ctr_build_string("lastIndexOf:", 12), &ctr_string_last_index_of);
	ctr_internal_create_func(TextString, ctr_build_string("replace:with:", 13), &ctr_string_replace_with);
	ctr_internal_create_func(TextString, ctr_build_string("split:", 6), &ctr_string_split);
	ctr_internal_create_func(TextString, ctr_build_string("toNumber", 8), &ctr_string_to_number);
	ctr_internal_create_func(TextString, ctr_build_string("toBoolean", 9), &ctr_string_to_boolean);
	ctr_internal_object_add_property(World, ctr_build_string("String", 6), TextString, 0);
	TextString->link = Object;

	//Block
	CBlock = ctr_internal_create_object(CTR_OBJECT_TYPE_OTBLOCK);
	ctr_internal_create_func(CBlock, ctr_build_string("run", 3), &ctr_block_run);
	ctr_internal_create_func(CBlock, ctr_build_string("error:", 6), &ctr_block_error);
	ctr_internal_create_func(CBlock, ctr_build_string("catch:", 6), &ctr_block_catch);
	ctr_internal_create_func(CBlock, ctr_build_string("whileTrue:", 10), &ctr_block_while_true);
	ctr_internal_create_func(CBlock, ctr_build_string("whileFalse:", 11), &ctr_block_while_false);
	ctr_internal_object_add_property(World, ctr_build_string("CodeBlock", 9), CBlock, 0);
	CBlock->link = Object;

	//Array
	CArray = ctr_array_new(Object);
	ctr_internal_create_func(CArray, ctr_build_string("new", 3), &ctr_array_new);
	ctr_internal_create_func(CArray, ctr_build_string("←", 3), &ctr_array_new_and_push);
	ctr_internal_create_func(CArray, ctr_build_string("push:", 5), &ctr_array_push);
	ctr_internal_create_func(CArray, ctr_build_string(";", 1), &ctr_array_push);
	ctr_internal_create_func(CArray, ctr_build_string("unshift:", 8), &ctr_array_unshift);
	ctr_internal_create_func(CArray, ctr_build_string("shift", 5), &ctr_array_shift);
	ctr_internal_create_func(CArray, ctr_build_string("count", 5), &ctr_array_count);
	ctr_internal_create_func(CArray, ctr_build_string("join:", 5), &ctr_array_join);
	ctr_internal_create_func(CArray, ctr_build_string("pop", 3), &ctr_array_pop);
	ctr_internal_create_func(CArray, ctr_build_string("at:", 3), &ctr_array_get);
	ctr_internal_create_func(CArray, ctr_build_string("sort:", 5), &ctr_array_sort);
	ctr_internal_create_func(CArray, ctr_build_string("put:at:", 7), &ctr_array_put);
	ctr_internal_create_func(CArray, ctr_build_string("from:length:", 12), &ctr_array_from_to);
	ctr_internal_create_func(CArray, ctr_build_string("+", 1), &ctr_array_add);
	ctr_internal_object_add_property(World, ctr_build_string("Array", 5), CArray, 0);
	CArray->link = Object;

	//Map
	CMap = ctr_internal_create_object(CTR_OBJECT_TYPE_OTOBJECT);
	ctr_internal_create_func(CMap, ctr_build_string("new", 3), &ctr_map_new);
	ctr_internal_create_func(CMap, ctr_build_string("put:at:", 7), &ctr_map_put);
	ctr_internal_create_func(CMap, ctr_build_string("at:", 3), &ctr_map_get);
	ctr_internal_create_func(CMap, ctr_build_string("count", 5), &ctr_map_count);
	ctr_internal_create_func(CMap, ctr_build_string("each:", 5), &ctr_map_each);
	ctr_internal_object_add_property(World, ctr_build_string("Map", 3), CMap, 0);
	CMap->link = Object;

	//Console
	Console = ctr_internal_create_object(CTR_OBJECT_TYPE_OTOBJECT);
	ctr_internal_create_func(Console, ctr_build_string("write:", 6), &ctr_console_write);
	ctr_internal_create_func(Console, ctr_build_string("brk", 3), &ctr_console_brk);
	ctr_internal_object_add_property(World, ctr_build_string("Pen", 3), Console, 0);
	Console->link = Object;
	Console->info.flagb = 1;

	//File
	CFile = ctr_internal_create_object(CTR_OBJECT_TYPE_OTOBJECT);
	ctr_internal_create_func(CFile, ctr_build_string("new:", 4), &ctr_file_new);
	ctr_internal_create_func(CFile, ctr_build_string("path", 4), &ctr_file_path);
	ctr_internal_create_func(CFile, ctr_build_string("read", 4), &ctr_file_read);
	ctr_internal_create_func(CFile, ctr_build_string("write:", 6), &ctr_file_write);
	ctr_internal_create_func(CFile, ctr_build_string("append:", 7), &ctr_file_append);
	ctr_internal_create_func(CFile, ctr_build_string("exists", 6), &ctr_file_exists);
	ctr_internal_create_func(CFile, ctr_build_string("size", 4), &ctr_file_size);
	ctr_internal_create_func(CFile, ctr_build_string("delete", 6), &ctr_file_delete);
	ctr_internal_create_func(CFile, ctr_build_string("include", 7), &ctr_file_include);
	ctr_internal_object_add_property(World, ctr_build_string("File", 4), CFile, 0);
	CFile->link = Object;

	//Command
	CCommand = ctr_internal_create_object(CTR_OBJECT_TYPE_OTOBJECT);
	ctr_internal_create_func(CCommand, ctr_build_string("argument:", 9), &ctr_command_argument);
	ctr_internal_create_func(CCommand, ctr_build_string("argCount", 8), &ctr_command_num_of_args);
	ctr_internal_object_add_property(World, ctr_build_string("Command", 7), CCommand, 0);
	CCommand->link = Object;

	//Clock
	CClock = ctr_internal_create_object(CTR_OBJECT_TYPE_OTOBJECT);
	ctr_internal_create_func(CClock, ctr_build_string("wait:", 5), &ctr_clock_wait);
	ctr_internal_create_func(CClock, ctr_build_string("time", 4), &ctr_clock_time);
	ctr_internal_object_add_property(World, ctr_build_string("Clock", 5), CClock, 0);
	CClock->link = Object;

	//Dice
	CDice = ctr_internal_create_object(CTR_OBJECT_TYPE_OTOBJECT);
	ctr_internal_create_func(CDice, ctr_build_string("roll", 4), &ctr_dice_throw);
	ctr_internal_create_func(CDice, ctr_build_string("rollWithSides:", 14), &ctr_dice_sides);
	ctr_internal_object_add_property(World, ctr_build_string("Dice", 4), CDice, 0);
	CDice->link = Object;

	//Coin
	CCoin = ctr_internal_create_object(CTR_OBJECT_TYPE_OTOBJECT);
	ctr_internal_create_func(CCoin, ctr_build_string("flip", 4), &ctr_coin_flip);
	ctr_internal_object_add_property(World, ctr_build_string("Coin", 4), CCoin, 0);
	CCoin->link = Object;

	//Shell
	CShell = ctr_internal_create_object(CTR_OBJECT_TYPE_OTOBJECT);
	ctr_internal_create_func(CShell, ctr_build_string("call:", 5), &ctr_shell_call);
	ctr_internal_object_add_property(World, ctr_build_string("Shell", 5), CShell, 0);

	//Broom
	GC = ctr_internal_create_object(CTR_OBJECT_TYPE_OTOBJECT);
	ctr_internal_create_func(GC, ctr_build_string("sweep", 5), &ctr_gc_collect);
	ctr_internal_create_func(GC, ctr_build_string("dust", 4), &ctr_gc_dust);
	ctr_internal_create_func(GC, ctr_build_string("objectCount", 11), &ctr_gc_object_count);
	ctr_internal_object_add_property(World, ctr_build_string("Broom", 5), GC, 0);
	GC->link = Object;
}


ctr_object* ctr_send_message(ctr_object* receiverObject, char* message, long vlen, ctr_argument* argumentList) {
	if (error != NULL) return NULL; //Error mode, ignore subsequent messages until resolved.
	ctr_object* methodObject = NULL;
	ctr_object* searchObject = receiverObject;
	while(!methodObject) {
		methodObject = ctr_internal_object_find_property(searchObject, ctr_build_string(message, vlen), 1);
		if (methodObject) break;
		if (!searchObject->link) break;
		searchObject = searchObject->link;
	}
	if (!methodObject) {
		ctr_argument* argCounter = argumentList;
		int argCount = 0;
		while(argCounter->next && argCount < 4) {
			argCounter = argCounter->next;
			argCount ++;
		}
		ctr_argument* mesgArgument = CTR_CREATE_ARGUMENT();
		mesgArgument->object = ctr_build_string(message, vlen);
		mesgArgument->next = argumentList;
		if (argCount == 0 || argCount > 2) {
			return ctr_send_message(receiverObject, "respondTo:", 10,  mesgArgument);
		} else if (argCount == 1) {
			return ctr_send_message(receiverObject, "respondTo:with:", 15,  mesgArgument);
		} else if (argCount == 2) {
			return ctr_send_message(receiverObject, "respondTo:with:and:", 19,  mesgArgument);
		}
	}
	ctr_object* result;
	if (methodObject->info.type == CTR_OBJECT_TYPE_OTNATFUNC) {
		ctr_object* (*funct)(ctr_object* receiverObject, ctr_argument* argumentList);
		funct = (void*) methodObject->value.block;
		result = (ctr_object*) funct(receiverObject, argumentList);
	}
	if (methodObject->info.type == CTR_OBJECT_TYPE_OTBLOCK) {
		result = ctr_block_run(methodObject, argumentList, receiverObject);
	}	
	return result;
}

ctr_object* ctr_assign_value(ctr_object* key, ctr_object* o) {
	ctr_object* object;
	key->info.sticky = 0;
	if (o->info.type == CTR_OBJECT_TYPE_OTOBJECT || o->info.type == CTR_OBJECT_TYPE_OTMISC) {
		ctr_set(key, o);
	} else {
		object = ctr_internal_create_object(o->info.type);
		object->properties = o->properties;
		object->methods = o->methods;
		object->link = o->link;
		object->info.sticky = 0;
		ctr_set(key, object);
	}
     //depending on type, copy specific value
    if (o->info.type == CTR_OBJECT_TYPE_OTBOOL) {
		object->value.bvalue = o->value.bvalue;
	 } else if (o->info.type == CTR_OBJECT_TYPE_OTNUMBER) {
		object->value.nvalue = o->value.nvalue;
	 } else if (o->info.type == CTR_OBJECT_TYPE_OTSTRING) {
		object->value.svalue = malloc(sizeof(ctr_string));
		object->value.svalue->value = malloc(sizeof(char)*o->value.svalue->vlen);
		memcpy(object->value.svalue->value, o->value.svalue->value,o->value.svalue->vlen);
		object->value.svalue->vlen = o->value.svalue->vlen;
	 } else if (o->info.type == CTR_OBJECT_TYPE_OTBLOCK) {
		object->value.block = o->value.block;
	 } else if (o->info.type == CTR_OBJECT_TYPE_OTARRAY) {
		object->value.avalue = malloc(sizeof(ctr_collection));
		object->value.avalue->elements = malloc(o->value.avalue->length*sizeof(ctr_object*));
		object->value.avalue->length = o->value.avalue->length;
		int i;
		ctr_object* putValue;
		for (i = o->value.avalue->tail; i < o->value.avalue->head; i++) {
			putValue = *( (ctr_object**) ((long)o->value.avalue->elements + (i * sizeof(ctr_object*))) );
			*( (ctr_object**) ((long)object->value.avalue->elements + (i * sizeof(ctr_object*))) ) = putValue;
		}
		object->value.avalue->head = o->value.avalue->head;
		object->value.avalue->tail = o->value.avalue->tail;
	 }
	 
	return object;
}

ctr_object* ctr_assign_value_to_my(ctr_object* key, ctr_object* o) {
	ctr_object* object;
	ctr_object* my = ctr_find(ctr_build_string("me", 2));
	key->info.sticky = 0;
	if (o->info.type == CTR_OBJECT_TYPE_OTOBJECT || o->info.type == CTR_OBJECT_TYPE_OTMISC) {
		ctr_internal_object_add_property(my, key, o, 0);
	} else {
		object = ctr_internal_create_object(o->info.type);
		object->properties = o->properties;
		object->methods = o->methods;
		object->link = o->link;
		object->info.sticky = 0;
		ctr_internal_object_add_property(my, key, object, 0);
	}
     //depending on type, copy specific value
    if (o->info.type == CTR_OBJECT_TYPE_OTBOOL) {
		object->value.bvalue = o->value.bvalue;
	 } else if (o->info.type == CTR_OBJECT_TYPE_OTNUMBER) {
		object->value.nvalue = o->value.nvalue;
	 } else if (o->info.type == CTR_OBJECT_TYPE_OTSTRING) {
		object->value.svalue = malloc(sizeof(ctr_string));
		object->value.svalue->value = malloc(sizeof(char)*o->value.svalue->vlen);
		memcpy(object->value.svalue->value, o->value.svalue->value,o->value.svalue->vlen);
		object->value.svalue->vlen = o->value.svalue->vlen;
	 } else if (o->info.type == CTR_OBJECT_TYPE_OTBLOCK) {
		object->value.block = o->value.block;
	 } else if (o->info.type == CTR_OBJECT_TYPE_OTARRAY) {
		object->value.avalue = malloc(sizeof(ctr_collection));
		object->value.avalue->elements = malloc(o->value.avalue->length*sizeof(ctr_object*));
		object->value.avalue->length = o->value.avalue->length;
		int i;
		ctr_object* putValue;
		for (i = 0; i < o->value.avalue->head; i++) {
			putValue = *( (ctr_object**) ((long)o->value.avalue->elements + (i * sizeof(ctr_object*))) );
			*( (ctr_object**) ((long)object->value.avalue->elements + (i * sizeof(ctr_object*))) ) = putValue;
		}
		object->value.avalue->head = o->value.avalue->head;
	 }
	 
	

	return object;
}

