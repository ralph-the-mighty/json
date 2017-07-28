//
//				TODO				
//				1) bounds checking on objects and arrays
//				2) check empty string handling
//				3) handle escape sequences
//				4) handle all legal numeric expressions (ints, floats, scientific notation)
//				5) recursive freeing of nodes
//				6) implement object as a bone fide hash table







#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define MAX_OBJECT_SIZE 256
#define MAX_ARRAY_SIZE 256

typedef enum {
	JSON_NULL = 0,
	JSON_INTEGER,
	JSON_STRING,
	JSON_BOOL,
	JSON_ARRAY,
	JSON_OBJECT,
} json_type;


struct json_pair;

struct json_value {
	json_type type;
	char* string;
	int boolean;
	int integer;

	int num_pairs;
	json_pair** pairs;
	

	int num_values;
	json_value** values;
	
};


struct json_pair {
	char* name;
	json_value* value;
};







void error(const char* msg) {
	MessageBoxA(0, msg, "Error", MB_OK);
	//__debugbreak();
	exit(1);
}



void errorf(const char* fmt, ...) {
	char buf[1024];
	va_list argp;
	va_start(argp, fmt);
	vsprintf_s(buf, sizeof(buf), fmt, argp);
	va_end(argp);

	error(buf);
}


typedef enum {
	TOKEN_NUM = 128,
	TOKEN_STRING,
	TOKEN_TRUE,
	TOKEN_FALSE,
	TOKEN_NULL
} Token_t;


Token_t token;
int token_number;
char* token_string;
char* text;


#define MAX_STRINGS 1024
size_t string_table_size = 0;
char* string_table[MAX_STRINGS] = { 0 };


void consume_token();

void expect_token(Token_t t) {
	if (token != t) {
		errorf("expected token %c", t);
	}
	consume_token();
}


//intern from start up to but not including end
void intern_string(char* start, char* end) {

	if (string_table_size == MAX_STRINGS) {
		error("Maximum number of unique strings exceeded!");
	}

	for (unsigned int i = 0; i < string_table_size; i++) {
		char* cmp_table = string_table[i];
		char* cmp_string = start;

		while (cmp_string != end && cmp_table && *cmp_string == *cmp_table) {
			cmp_string++;
			cmp_table++;
		}

		if (cmp_string == end) {
			return;
		}
	}

	string_table[string_table_size++] = start;
	token_string = start;
	*end = 0;
}

//returns next non-whitespace char
char get_next_char() {
	char* c = text;
	while (isspace(*c)) {
		c++;
	}
	return *c;
}


void consume_token() {
	//whitespace
	while (isspace(*text)) {
		text++;
	}

	//strings
	if (*text == '"') {
		token = TOKEN_STRING;
		text++;
		char *string_start = text;
		char *string_end = text;
		while (*string_end != '"') {
			string_end++;
		}
		intern_string(string_start, string_end);
		text = string_end + 1;
	} else if (*text == 'n'){
		token = TOKEN_NULL;
		text += 4;
	} else if (*text == 't') {
		token = TOKEN_TRUE;
		text += 4;
	} else if (*text == 'f') {
		token = TOKEN_FALSE;
		text += 5;
	} else if (isdigit(*text)) {
		token = TOKEN_NUM;
		token_number = 0;
		while (isdigit(*text)) {
			token_number *= 10;
			token_number += *text - '0';
			text++;
		}
	} else {
		switch (*text) {
		case '{':
		case '}':
		case '[':
		case ']':
		case ':':
		case ',':
			token = (Token_t)*text;
			text++;
			break;
		case 0:
			break;
		default:
			errorf("unknown token %c", *text);
			break;
		}
	}
}



json_value* initialize_value(json_type type) {
	json_value* value = (json_value*)malloc(sizeof(json_value));
	memset(value, 0, sizeof(json_value));
	value->type = type;
	return value;
}

void free_value(json_value* value) {
	free(value);
}


json_value* parse_object();
json_value* parse_array();


json_value* parse_value() {

	json_value* value = initialize_value(JSON_NULL);

	switch (token) {
		case (Token_t) '{':
			free_value(value);
			return parse_object();
			break;
		case (Token_t) '[':
			free_value(value);
			return parse_array();
			break;
		case TOKEN_NUM:
			value->type = JSON_INTEGER;
			value->integer = token_number;
			consume_token();
			return value;
			break;
		case TOKEN_STRING:
			value->type = JSON_STRING;
			value->string = token_string;
			consume_token();
			return value;
			break;
		case TOKEN_TRUE:
			value->type = JSON_BOOL;
			value->boolean = 1;
			consume_token();
			return value;
			break;
		case TOKEN_FALSE:
			value->type = JSON_BOOL;
			value->boolean = 0;
			consume_token();
			return value;
			break;
		case TOKEN_NULL:
			consume_token();
			return value;
			break;
		default:
			error("parse_value(): unrecognized token type");
			break;
	}
}

json_pair* parse_pair() {
	json_pair* pair = (json_pair*)malloc(sizeof(json_pair));
	expect_token(TOKEN_STRING);
	pair->name = token_string;
	expect_token((Token_t) ':');
	pair->value = parse_value();
	return pair;
}


json_value* parse_object() {

	json_value* object = initialize_value(JSON_OBJECT);


	consume_token();
	if (get_next_char() == '}') {		
		return object;
	} else {

		// initilize pairs array
		// just do the dumb thing for now and 
		// allocate a fixed size array every time.
		object->pairs = (json_pair**)malloc(sizeof(json_pair) * MAX_OBJECT_SIZE);
		memset(object->pairs, 0, sizeof(json_pair) * MAX_OBJECT_SIZE);

		object->pairs[object->num_pairs++] = parse_pair();

		while (token == (Token_t) ',') {
			//TODO: bounds check
			consume_token();
			object->pairs[object->num_pairs++] = parse_pair();
		}

		expect_token((Token_t)'}');
	}

	return object;	
}

json_value* parse_array() {

	json_value* array = initialize_value(JSON_ARRAY);

	consume_token();
	if (get_next_char() == ']') {
		return array;
	} else {


		// initilize values array
		// just do the dumb thing for now and 
		// allocate a fixed size array every time.
		array->values = (json_value**)malloc(sizeof(json_value) * MAX_ARRAY_SIZE);
		memset(array->values, 0, sizeof(json_value) * MAX_ARRAY_SIZE);

		array->values[array->num_values++] = parse_value();

		while (token == (Token_t)',') {
			//TODO: bounds check
			consume_token();
			array->values[array->num_values++] = parse_value();
		}
		expect_token((Token_t) ']');
	}
	
	return array;
}



json_value* object_get(json_value* object, char* name) {
	//TODO: check to make sure object is actually a non-empty object
	for (unsigned int i = 0; i < object->num_pairs; i++) {
		char* obj_string = object->pairs[i]->name;
		char* cmp_string = name;
		while (*cmp_string && *obj_string && *cmp_string == *obj_string) {
			cmp_string++;
			obj_string++;
		}

		if (*cmp_string == 0 && *obj_string == 0) {
			return object->pairs[i]->value;
		}
	}
	errorf("no entry for name: %s", name);
}


json_value* array_get(json_value* array, int index) {
	//TODO:
	//assert index > 0
	//assert type of array is array

	if (index >= array->num_values) {
		error("out of bounds access to array");
	} else {
		return array->values[index];
	}

}



json_value* parse(char* string) {
	text = string;
	consume_token();
	return parse_object();
}




int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {	
	
	OFSTRUCT file_struct;
	HFILE file_handle;
	size_t file_size;

	file_handle = OpenFile("C:\\Users\\Joshua\\Desktop\\json\\json\\test.json", &file_struct, OF_READ);
	if (!file_handle) {
		error("error opening json file");
		CloseHandle((HANDLE)file_handle);
		exit(1);
	}

	file_size = (size_t)GetFileSize((HANDLE)file_handle, 0);
	char* json_string = (char*)malloc(file_size + 1);
	ReadFile((HANDLE)file_handle, json_string, file_size, 0, 0);
	CloseHandle((HANDLE)file_handle);
	json_string[file_size] = 0;


	json_value* root = parse(json_string);
	json_value* server = object_get(root, "server");
	json_value* that = object_get(server, "that");
	json_value* those = object_get(server, "those");
	json_value* entry = array_get(those, 2);
}