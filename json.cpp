#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>



void error(const char* msg) {
	MessageBoxA(0, msg, "Error", MB_OK);
	__debugbreak();
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
			error("unknown token!");
			break;
		}
	}
}


void parse_object();
void parse_array();


void parse_value() {
	switch (token) {
		case (Token_t) '{':
			parse_object();
			break;
		case (Token_t) '[':
			parse_array();
			break;
		case TOKEN_NUM:
		case TOKEN_STRING:
		case TOKEN_TRUE:
		case TOKEN_FALSE:
		case TOKEN_NULL:
			consume_token();
			break;
		default:
			break;
	}
}

void parse_pair() {
	expect_token(TOKEN_STRING);
	expect_token((Token_t) ':');
	parse_value();
}


void parse_object() {
	consume_token();
	parse_pair();
	while (token == (Token_t) ',') {
		consume_token();
		parse_pair();
	}
	expect_token((Token_t)'}');
}

void parse_array() {
	consume_token();
	parse_value();
	while (token == Token_t(',')) {
		consume_token();
		parse_value();
	}
	expect_token((Token_t) ']');
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
	text = (char*)malloc(file_size + 1);
	ReadFile((HANDLE)file_handle, text, file_size, 0, 0);
	CloseHandle((HANDLE)file_handle);
	text[file_size] = 0;


	consume_token();
	parse_object();
}