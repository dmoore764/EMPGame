enum token_type
{
	TOKEN_TYPE_WORD,
	TOKEN_TYPE_STRING,
	TOKEN_TYPE_NUMBER,
	TOKEN_TYPE_COLON,
	TOKEN_TYPE_COMMA,
	TOKEN_TYPE_OPEN_BRACKET,
	TOKEN_TYPE_CLOSE_BRACKET,
	TOKEN_TYPE_OPEN_BRACE,
	TOKEN_TYPE_CLOSE_BRACE,
	TOKEN_TYPE_EOF,
};

struct tokenizer
{
	const char *file_name;
	char *src;
	char *at;
	int row;
	int col;
};

struct token
{
	token_type type;
	char *text;
	int length;

	tokenizer tokenizer_copy;

	token *next;
};

void EatWhiteSpace(tokenizer *t);
token GetToken(tokenizer *t);
token *Tokenize(char *text, const char *file_name, memory_arena *arena);
bool TokenMatches(token *t, const char *match);
void PrintTokens(token *head);
void PrintErrorAtToken(tokenizer t, const char *error_message, ...);
void EatToken(token **t, const char *match);
void EatToken(token **t, token_type type);
void EatToken(token **t);
void MoveToToken(token **t, const char *match);
void MoveToToken(token **t, token_type type);

enum json_type
{
	JSON_BOOL,
	JSON_NUMBER,
	JSON_STRING,
	JSON_ARRAY,
	JSON_HASH,
};

enum json_number_type
{
	JSON_NUMBER_INT,
	JSON_NUMBER_FLOAT,
	JSON_NUMBER_HEX,
};

struct json_hash;
struct json_array;

struct json_value
{
	token *start;
	json_type type;
	union
	{
		json_hash *hash;
		json_array *array;
		struct 
		{
			json_number_type type;
			union
			{
				int64_t i;
				double f;
				uint64_t h;
			};
		} number;
		char *string;
		bool boolean;
	};
	json_value *next;
};

struct json_hash_element
{
	token *keyword;
	char *key;
	json_value *value;
	json_hash_element *next;
};

struct json_hash
{
	token *start;
	json_hash_element *first;
	json_value *GetByKey(const char *key);
};

struct json_array
{
	token *start;
	int num_elements;
	json_value *first;

	json_value *GetByIndex(int index);
};

json_value *ParseJSONValue(token **start);
json_hash_element *ParseJSONHashElement(token **start);
json_hash *ParseJSONHash(token **start);
json_array *ParseJSONArray(token **start);

float GetJSONValAsFloat(json_value *val);
uint8_t GetJSONValAsUint8(json_value *val);
uint32_t GetJSONValAsUint32(json_value *val);
int GetJSONValAsInt(json_value *val);
bool GetJSONValAsBool(json_value *val);
void JsonErrorUnlessVec2(json_value *val);
