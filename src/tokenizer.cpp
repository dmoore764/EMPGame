void EatToken(token **t, const char *match)
{
	if (!TokenMatches(*t, match))
		assert(false);
	(*t) = (*t)->next;
}

void EatToken(token **t, token_type type)
{
	if (!((*t)->type == type))
		assert(false);
	(*t) = (*t)->next;
}

void EatToken(token **t)
{
	(*t) = (*t)->next;
}

void MoveToToken(token **t, const char *match)
{
	(*t) = (*t)->next; //don't match first one
	while (*t && !TokenMatches(*t, match))
		(*t) = (*t)->next;
}

void MoveToToken(token **t, token_type type)
{
	(*t) = (*t)->next; //don't match first one
	while (*t && !((*t)->type == type))
		(*t) = (*t)->next;
}

char *_AdvanceTokenizer(tokenizer *t)
{
	if (t->at[0] == '\n')
	{
		t->row ++;
		t->col = 1;
	}
	else
	{
		t->col ++;
	}
	return t->at++;
}

void EatWhiteSpace(tokenizer *t)
{
	bool eating_whitespace = true;
	while (eating_whitespace)
	{
		if (t->at[0] == ' ' || t->at[0] == '\t' || t->at[0] == '\n' || t->at[0] == '\r')
			_AdvanceTokenizer(t);
		else if (t->at[0] == '/' && t->at[1] == '/')
		{
			while (t->at[0] && t->at[0] != '\n')
			{
				_AdvanceTokenizer(t);
			}
		}
		else
			eating_whitespace = false;
	}
}

token GetToken(tokenizer *t)
{
	EatWhiteSpace(t);

	token result = {};
	char c = t->at[0];
	result.tokenizer_copy = *t;
	result.text = _AdvanceTokenizer(t);
	result.length = 1;

	switch (c)
	{
		case ':': { result.type = TOKEN_TYPE_COLON; } break;
		case ',': { result.type = TOKEN_TYPE_COMMA; } break;
		case '[': { result.type = TOKEN_TYPE_OPEN_BRACKET; } break;
		case ']': { result.type = TOKEN_TYPE_CLOSE_BRACKET; } break;
		case '{': { result.type = TOKEN_TYPE_OPEN_BRACE; } break;
		case '}': { result.type = TOKEN_TYPE_CLOSE_BRACE; } break;
		case '\0': { result.type = TOKEN_TYPE_EOF; } break;
		case '"':
		{
			result.tokenizer_copy = *t;
			result.text = t->at;
			result.length = 0;
			while (t->at[0])
			{
				if (t->at[0] == '"')
				{
					if (t->at[-1] != '\\')
						break;
				}
				_AdvanceTokenizer(t);
			}

			result.length = (int)(t->at - result.text);
			result.type = TOKEN_TYPE_STRING;
			
			//advance past the end quote
			if (t->at)
				_AdvanceTokenizer(t);
		} break;

		default:
		{
			if ((c >= '0' && c <= '9') || c == '-')
			{
				bool encountered_period = false;
				bool allow_hex = false;
				if (c== '0' && (t->at[0] == 'x' || t->at[0] == 'X'))
				{
					_AdvanceTokenizer(t);
					encountered_period = true; //not really, but we don't accept any '.' at this point
					allow_hex = true;
				}

				result.type = TOKEN_TYPE_NUMBER;
				while ((t->at[0] >= '0' && t->at[0] <= '9') || (t->at[0] == '.' && !encountered_period) || (allow_hex && ((t->at[0] >= 'a' && t->at[0] <= 'f') || (t->at[0] >= 'A' && t->at[0] <= 'F'))))
				{
					if (t->at[0] == '.')
						encountered_period = true;
					_AdvanceTokenizer(t);
				}
				result.length = (int)(t->at - result.text);
			}
			else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
			{
				result.type = TOKEN_TYPE_WORD;
				c = t->at[0];
				while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
				{
					_AdvanceTokenizer(t);
					c = t->at[0];
				}
				result.length = (int)(t->at - result.text);
			}
			else
			{
				PrintErrorAtToken(result.tokenizer_copy, "Error tokenizing, unexpected token '%.*s'", result.length, result.text);
				assert(false);
			}
		} break;
	}

	return result;
}

void PrintErrorAtToken(tokenizer t, const char *error_message, ...)
{
	char message[KILOBYTES(1)];
	va_list args;
	va_start(args, error_message);
	vsprintf(message, error_message, args);
	va_end(args);

	ERR("Error in file '%s'", t.file_name);
	ERR("Line %d col %d", t.row, t.col);
	ERR("%s", message);
	char *line_start = t.at;
	int line_moves = 0;
	while (line_start > t.src && line_moves < 4)
	{
		if (line_start[0] == '\n')
			line_moves++;
		line_start--;
	}
	if (line_start[0] == '\r' && line_start[1] == '\n')
	{
		line_start += 2;
		line_moves--;
	}

	int num_lines = 0;
	char *line_ends[7];
	char *current_pos = line_start;
	while (num_lines < 7)
	{
		line_ends[num_lines] = strchr(current_pos, '\n');
		if (line_ends[num_lines])
		{
			current_pos = line_ends[num_lines] + 1;
			num_lines++;
		}
		else
		{
			break;
		}
	}
	current_pos = line_start;
	for (int i = 0; i < num_lines; i++)
	{
		ERR("%*d:%.*s", 4, t.row - line_moves + i, (int)(line_ends[i] - current_pos), current_pos);
		if (t.row - line_moves + i == t.row)
		{
			//count how many tabs in the row
			int num_tabs = 0;
			for (int j = 0; j < t.col; j++)
			{
				if (current_pos[j] == '\t')
					num_tabs++;
			}
			ERR("     %.*s%.*s^-----here", num_tabs, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t", t.col - num_tabs - 1, "                                                                                               ");
		}
		current_pos = line_ends[i] + 1;
	}
}

token *Tokenize(char *text, const char *file_name, memory_arena *arena)
{
	if (!text)
		return NULL;

	token *result = NULL;
	tokenizer t;
	t.file_name = file_name;
	t.src = text;
	t.at = text;
	t.row = 0;
	t.col = 1;

	token *current = NULL;
	bool tokenizing = true;
	while (tokenizing)
	{
		token next = GetToken(&t);
		switch (next.type)
		{
			case TOKEN_TYPE_EOF:
			{
				tokenizing = false;
			} break;
			default:
			{
				if (result == NULL)
				{
					result = PUSH_ITEM(arena, token);
					*result = next;
					current = result;
				}
				else
				{
					current->next = PUSH_ITEM(arena, token);
					current = current->next;
					*current = next;
				}
			} break;
		}
	}

	return result;
}

bool TokenMatches(token *t, const char *match)
{
	for (int i = 0; i < t->length; i++)
	{
		if (!match[i])
			return false;
		if (match[i] != t->text[i])
			return false;
	}
	if (match[t->length] != '\0')
		return false;

	return true;
}


void PrintTokens(token *head)
{
	int current = 0;
	while (head)
	{
		
		const char* tokenString = 0;
		switch (head->type)
		{
			case TOKEN_TYPE_WORD: { tokenString = "Word"; } break;
			case TOKEN_TYPE_STRING: { tokenString = "String"; } break;
			case TOKEN_TYPE_NUMBER: { tokenString = "Number"; } break;
			case TOKEN_TYPE_COLON: { tokenString = "Colon"; } break;
			case TOKEN_TYPE_COMMA: { tokenString = "Comma"; } break;
			case TOKEN_TYPE_OPEN_BRACKET: { tokenString = "Open Bracket"; } break;
			case TOKEN_TYPE_CLOSE_BRACKET: { tokenString = "Close Bracket"; } break;
			case TOKEN_TYPE_OPEN_BRACE: { tokenString = "Open Brace"; } break;
			case TOKEN_TYPE_CLOSE_BRACE: { tokenString = "Close Brace"; } break;
			case TOKEN_TYPE_EOF: { tokenString = "EOF"; } break;
		}
		DBG("[%d] (%s) %.*s", current++, tokenString, head->length, head->text);
		head = head->next;
	}
}


json_value *json_hash::GetByKey(const char *key)
{
	json_value *result = NULL;
	json_hash_element *current = this->first;
	while (current)
	{
		if (strcmp(current->key, key) == 0)
		{
			result = current->value;
			break;
		}
		current = current->next;
	}

	return result;
}

json_value *json_array::GetByIndex(int index)
{
	assert (index < this->num_elements);
	json_value *result = this->first;

	for (int i = 0; i < index; i++)
		result = result->next;

	return result;
}

json_value *ParseJSONValue(token **start)
{
	json_value *val = PUSH_ITEM(&Arena, json_value);
	val->next = NULL;
	val->start  = *start;

	token *current = *start;
	if (!current)
	{
		ERR("Unexpected end of JSON");
		assert(false);
	}

	switch (current->type)
	{
		case TOKEN_TYPE_OPEN_BRACE:
		{
			val->type = JSON_HASH;
			val->hash = ParseJSONHash(&current);
		} break;

		case TOKEN_TYPE_OPEN_BRACKET:
		{
			val->type = JSON_ARRAY;
			val->array = ParseJSONArray(&current);
		} break;

		case TOKEN_TYPE_STRING:
		{
			val->type = JSON_STRING;
			val->string = PUSH_ARRAY(&Arena, char, current->length + 1);
			sprintf(val->string, "%.*s", current->length, current->text);
			current = current->next;
		} break;

		case TOKEN_TYPE_NUMBER:
		{
			val->type = JSON_NUMBER;
			val->number.type = JSON_NUMBER_INT;
			for (int i = 0; i < current->length; i++)
			{
				if (current->text[i] == '.')
				{
					val->number.type = JSON_NUMBER_FLOAT;
					break;
				}
			}
			if (val->number.type == JSON_NUMBER_FLOAT)
				val->number.f = atof(current->text);
			else
			{
				if (current->length > 2 && current->text[0] == '0' && (current->text[1] == 'x' || current->text[1] == 'X'))
				{
					val->number.type = JSON_NUMBER_HEX;
					val->number.h = (uint64_t)strtol(current->text, NULL, 16);
				}
				else
				{
					val->number.i = atoi(current->text);
				}
			}

			current = current->next;
		} break;

		case TOKEN_TYPE_WORD:
		{
			if (TokenMatches(current, "true"))
			{
				val->type = JSON_BOOL;
				val->boolean = true;
			}
			else if (TokenMatches(current, "false"))
			{
				val->type = JSON_BOOL;
				val->boolean = false;
			}
			else
			{
				val->type = JSON_STRING;
				val->string = PUSH_ARRAY(&Arena, char, current->length + 1);
				sprintf(val->string, "%.*s", current->length, current->text);
			}
			current = current->next;
		} break;

		default:
		{
			PrintErrorAtToken(val->start->tokenizer_copy, "Unexpected token while parsing JSON (expected value)");
			return NULL;
		} break;
	}

	*start = current;

	return val;
}

json_hash_element *ParseJSONHashElement(token **start)
{
	json_hash_element *el = PUSH_ITEM(&Arena, json_hash_element);
	el->keyword = *start;
	el->key = PUSH_ARRAY(&Arena, char, el->keyword->length + 1);
	sprintf(el->key, "%.*s", el->keyword->length, el->keyword->text);
	el->next = NULL;
	
	token *current = *start;
	current = current->next;
	if (current && current->type != TOKEN_TYPE_COLON)
	{
		PrintErrorAtToken(current->tokenizer_copy, "Unexpected token while parsing JSON (expected ':')");
		assert(false);
	}
	current = current->next;
	el->value = ParseJSONValue(&current);
	*start = current;

	return el;
}

json_hash *ParseJSONHash(token **start)
{
	json_hash *hash = PUSH_ITEM(&Arena, json_hash);
	hash->start = *start;
	hash->first = NULL;
	json_hash_element **cur_el_pointer = &hash->first;

	token *current = *start;
	if (!current || current->type != TOKEN_TYPE_OPEN_BRACE)
	{
		PrintErrorAtToken(current->tokenizer_copy, "Unexpected token while parsing JSON (expected '{')");
		assert(false);
	}
	current = current->next;

	//allow keys to be enclosed by quotes or not
	while (current && (current->type == TOKEN_TYPE_STRING || current->type == TOKEN_TYPE_WORD))
	{
		json_hash_element *el = ParseJSONHashElement(&current);
		*cur_el_pointer = el;
		cur_el_pointer = &el->next;

		switch (current->type)
		{
			case TOKEN_TYPE_COMMA:
			{
				current = current->next;
			} break;
			case TOKEN_TYPE_CLOSE_BRACE:
			{
				break;
			} break;
			default:
			{
				PrintErrorAtToken(current->tokenizer_copy, "Unexpected token while parsing JSON (expected '}' or ',')");
				assert(false);
			} break;
		}
	}
	if (!current || current->type != TOKEN_TYPE_CLOSE_BRACE)
	{
		PrintErrorAtToken(current->tokenizer_copy, "Unmatched '{' in JSON file");
		assert (false);
	}

	current = current->next;
	*start = current;
	
	return hash;
}

json_array *ParseJSONArray(token **start)
{
	json_array *array = PUSH_ITEM(&Arena, json_array);
	array->start = *start;
	array->num_elements = 0;
	array->first = NULL;
	json_value **cur_val_pointer = &array->first;

	token *current = *start;
	if (!current || current->type != TOKEN_TYPE_OPEN_BRACKET)
	{
		PrintErrorAtToken(current->tokenizer_copy, "Unexpected token while parsing JSON (expected '[')");
		assert(false);
	}
	current = current->next;

	while (current && current->type != TOKEN_TYPE_CLOSE_BRACKET)
	{
		array->num_elements++;
		json_value *val = ParseJSONValue(&current);
		*cur_val_pointer = val;
		cur_val_pointer = &val->next;

		switch (current->type)
		{
			case TOKEN_TYPE_COMMA:
			{
				current = current->next;
			} break;
			case TOKEN_TYPE_CLOSE_BRACKET:
			{
				break;
			} break;
			default:
			{
				PrintErrorAtToken(current->tokenizer_copy, "Unexpected token while parsing JSON (expected ']' or ',')");
				assert(false);
			} break;
		}
	}
	if (!current || current->type != TOKEN_TYPE_CLOSE_BRACKET)
	{
		PrintErrorAtToken(current->tokenizer_copy, "Unmatched '[' in JSON file");
		assert (false);
	}
	current = current->next;
	*start = current;

	return array;
}

float GetJSONValAsFloat(json_value *val)
{
	if (val->type != JSON_NUMBER)
	{
		PrintErrorAtToken(val->start->tokenizer_copy, "Expecting a number");
		assert(false);
	}
	float result = 0;
	if (val->number.type == JSON_NUMBER_INT)
		result = (float)val->number.i;
	else
		result = val->number.f;
	return result;
}

uint8_t GetJSONValAsUint8(json_value *val)
{
	if (val->type != JSON_NUMBER)
	{
		PrintErrorAtToken(val->start->tokenizer_copy, "Expecting a number");
		assert(false);
	}
	int64_t result = 0;
	if (val->number.type == JSON_NUMBER_INT)
		result = val->number.i;
	else if (val->number.type == JSON_NUMBER_HEX)
		result = val->number.h;
	assert(val->number.type == JSON_NUMBER_INT);

	assert(result == (uint8_t)result); //negative or higher than 255?
	return (uint8_t)result;
}

uint32_t GetJSONValAsUint32(json_value *val)
{
	if (val->type != JSON_NUMBER)
	{
		PrintErrorAtToken(val->start->tokenizer_copy, "Expecting a number");
		assert(false);
	}
	int64_t result = 0;
	if (val->number.type == JSON_NUMBER_INT)
		result = val->number.i;
	else if (val->number.type == JSON_NUMBER_HEX)
		result = val->number.h;

	assert(result == (uint32_t)result); //negative number?
	return (uint32_t)result;
}

int GetJSONValAsInt(json_value *val)
{
	if (val->type != JSON_NUMBER)
	{
		PrintErrorAtToken(val->start->tokenizer_copy, "Expecting a number");
		assert(false);
	}
	int64_t result = 0;
	assert(val->number.type == JSON_NUMBER_INT);
	result = val->number.i;
	assert(result == (int)result);
	return (int)result;
}

bool GetJSONValAsBool(json_value *val)
{
	if (val->type != JSON_BOOL)
	{
		PrintErrorAtToken(val->start->tokenizer_copy, "Expecting a true or false");
		assert(false);
	}
	bool result = val->boolean;
	return result;
}

void JsonErrorUnlessVec2(json_value *val)
{
	if (val->type != JSON_ARRAY || val->array->num_elements != 2 || val->array->GetByIndex(0)->type != JSON_NUMBER || val->array->GetByIndex(1)->type != JSON_NUMBER)
	{
		PrintErrorAtToken(val->start->tokenizer_copy, "vec2 values specified as two member arrays [0,0]");
		assert(false);
	}
}



