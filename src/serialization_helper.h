#pragma once

bool DeserializeCharStar(json_value *member, char **value)
{
	//Set string
	//string options: direct set, random selection
	if (member->type == JSON_HASH) //random selection
	{
		json_value *selection = member->hash->GetByKey("random selection");
		if (!selection)
		{
			PrintErrorAtToken(selection->start->tokenizer_copy, "Strings can be specified as 'random selection' or as a simple string");
			return false;
		}
		if (selection->type != JSON_ARRAY)
		{
			PrintErrorAtToken(selection->start->tokenizer_copy, "type of 'random selection' must be an array of choices");
			return false;
		}
		json_value *string = selection->array->GetByIndex(rand() % selection->array->num_elements);
		if (string->type != JSON_STRING)
		{
			PrintErrorAtToken(selection->start->tokenizer_copy, "This must be a string");
			return false;
		}
		*value = string->string;
	}
	else if (member->type == JSON_STRING)
	{
		*value = member->string;
	}
	else
	{
		PrintErrorAtToken(member->start->tokenizer_copy, "Set as string or random selection of strings");
		return false;
	}

	return true;
}

bool DeserializeCharArray(json_value *member, char *value, int count)
{
	//Set string
	//string options: direct set, random selection
	if (member->type == JSON_HASH) //random selection
	{
		json_value *selection = member->hash->GetByKey("random selection");
		if (!selection)
		{
			PrintErrorAtToken(selection->start->tokenizer_copy, "Strings can be specified as 'random selection' or as a simple string");
			return false;
		}
		if (selection->type != JSON_ARRAY)
		{
			PrintErrorAtToken(selection->start->tokenizer_copy, "type of 'random selection' must be an array of choices");
			return false;
		}
		json_value *string = selection->array->GetByIndex(rand() % selection->array->num_elements);
		if (string->type != JSON_STRING)
		{
			PrintErrorAtToken(selection->start->tokenizer_copy, "This must be a string");
			return false;
		}
		if (strlen(string->string) + 1 >= count)
		{
			PrintErrorAtToken(selection->start->tokenizer_copy, "String too long to fit into char[] array");
			return false;
		}

		strcpy(value, string->string);
	}
	else if (member->type == JSON_STRING)
	{
		if (strlen(member->string) + 1 >= count)
		{
			PrintErrorAtToken(member->start->tokenizer_copy, "String too long to fit into char[] array");
			return false;
		}
		strcpy(value, member->string);
	}
	else
	{
		PrintErrorAtToken(member->start->tokenizer_copy, "Set as string or random selection of strings");
		return false;
	}

	return true;
}

bool DeserializeFloat(json_value *member, float *value)
{
	//float options: direct set, random selection, random range
	if (member->type == JSON_HASH) //random selection
	{
		json_value *selection = member->hash->GetByKey("random selection");
		if (selection)
		{
			if (selection->type != JSON_ARRAY)
			{
				PrintErrorAtToken(selection->start->tokenizer_copy, "type of 'random selection' must be an array of choices");
				return false;
			}

			json_value *val = selection->array->GetByIndex(rand() % selection->array->num_elements);
			if (val->type != JSON_NUMBER)
			{
				PrintErrorAtToken(selection->start->tokenizer_copy, "This must be a number");
				return false;
			}
			*value = GetJSONValAsFloat(val);
		}
		else
		{
			json_value *range = member->hash->GetByKey("random range");
			if (range)
			{
				if (range->type != JSON_ARRAY || range->array->num_elements != 2)
				{
					PrintErrorAtToken(selection->start->tokenizer_copy, "'range' must be specified as [Min, Max]");
					return false;
				}
				float min = GetJSONValAsFloat(range->array->GetByIndex(0));
				float max = GetJSONValAsFloat(range->array->GetByIndex(1));
				*value = glm::linearRand(min, max);
			}
		}
	}
	else if (member->type == JSON_NUMBER)
	{
		*value = GetJSONValAsFloat(member);
	}
	else
	{
		PrintErrorAtToken(member->start->tokenizer_copy, "Floats are set defined with the following: 0.0 or { \"random selection\" : [0, 2, 3, 5] } or { \"random range\" : [-10.0, 30.0] }");
		return false;
	}
	return true;
}


bool DeserializeInt(json_value *member, int *value)
{
	//int options: direct set, random selection, random range
	if (member->type == JSON_HASH) //random selection
	{
		json_value *selection = member->hash->GetByKey("random selection");
		if (selection)
		{
			if (selection->type != JSON_ARRAY)
			{
				PrintErrorAtToken(selection->start->tokenizer_copy, "type of 'random selection' must be an array of choices");
				return false;
			}

			json_value *val = selection->array->GetByIndex(rand() % selection->array->num_elements);
			if (val->type != JSON_NUMBER)
			{
				PrintErrorAtToken(selection->start->tokenizer_copy, "This must be a number");
				return false;
			}
			*value = GetJSONValAsInt(val);
		}
		else
		{
			json_value *range = member->hash->GetByKey("random range");
			if (range)
			{
				if (range->type != JSON_ARRAY || range->array->num_elements != 2)
				{
					PrintErrorAtToken(selection->start->tokenizer_copy, "'range' must be specified as [Min, Max]");
					return false;
				}
				int min = GetJSONValAsInt(range->array->GetByIndex(0));
				int max = GetJSONValAsInt(range->array->GetByIndex(1));
				int val = (rand() % (max - min)) + min;
				*value = val;
			}
		}
	}
	else if (member->type == JSON_NUMBER)
	{
		*value = GetJSONValAsInt(member);
	}
	else
	{
		PrintErrorAtToken(member->start->tokenizer_copy, "Ints are set defined with the following: 123 or { \"random selection\" : [0, 2, 3, 5] } or { \"random range\" : [-10, 30] }");
		return false;
	}
	return true;
}


bool DeserializeBool(json_value *member, bool *value)
{
	if (member->type == JSON_BOOL)
	{
		*value = member->boolean;
	}
	else
	{
		PrintErrorAtToken(member->start->tokenizer_copy, "Bools must be set directly as true or false");
		return false;
	}
	return true;
}

bool DeserializeUint16(json_value *member, uint16_t *value)
{
	//uint32_t options: direct set, random selection, random range
	if (member->type == JSON_HASH) //random selection
	{
		json_value *selection = member->hash->GetByKey("random selection");
		if (selection)
		{
			if (selection->type != JSON_ARRAY)
			{
				PrintErrorAtToken(selection->start->tokenizer_copy, "type of 'random selection' must be an array of choices");
				return false;
			}

			json_value *val = selection->array->GetByIndex(rand() % selection->array->num_elements);
			if (val->type != JSON_NUMBER)
			{
				PrintErrorAtToken(selection->start->tokenizer_copy, "This must be a number");
				return false;
			}
			*value = (uint16_t)GetJSONValAsUint32(val);
		}
		else
		{
			json_value *range = member->hash->GetByKey("random range");
			if (range)
			{
				if (range->type != JSON_ARRAY || range->array->num_elements != 2)
				{
					PrintErrorAtToken(selection->start->tokenizer_copy, "'range' must be specified as [Min, Max]");
					return false;
				}
				int min = GetJSONValAsUint32(range->array->GetByIndex(0));
				int max = GetJSONValAsUint32(range->array->GetByIndex(1));
				int val = (rand() % (max - min)) + min;
				*value = (uint16_t)val;
			}
		}
	}
	else if (member->type == JSON_NUMBER)
	{
		*value = (uint16_t)GetJSONValAsUint32(member);
	}
	else
	{
		PrintErrorAtToken(member->start->tokenizer_copy, "Ints are set defined with the following: 123 or { \"random selection\" : [0, 2, 3, 5] } or { \"random range\" : [-10, 30] }");
		return false;
	}
	return true;
}




bool DeserializeUint32(json_value *member, uint32_t *value)
{
	//uint32_t options: direct set, random selection, random range
	if (member->type == JSON_HASH) //random selection
	{
		json_value *selection = member->hash->GetByKey("random selection");
		if (selection)
		{
			if (selection->type != JSON_ARRAY)
			{
				PrintErrorAtToken(selection->start->tokenizer_copy, "type of 'random selection' must be an array of choices");
				return false;
			}

			json_value *val = selection->array->GetByIndex(rand() % selection->array->num_elements);
			if (val->type != JSON_NUMBER)
			{
				PrintErrorAtToken(selection->start->tokenizer_copy, "This must be a number");
				return false;
			}
			*value = GetJSONValAsUint32(val);
		}
		else
		{
			json_value *range = member->hash->GetByKey("random range");
			if (range)
			{
				if (range->type != JSON_ARRAY || range->array->num_elements != 2)
				{
					PrintErrorAtToken(selection->start->tokenizer_copy, "'range' must be specified as [Min, Max]");
					return false;
				}
				int min = GetJSONValAsUint32(range->array->GetByIndex(0));
				int max = GetJSONValAsUint32(range->array->GetByIndex(1));
				int val = (rand() % (max - min)) + min;
				*value = val;
			}
		}
	}
	else if (member->type == JSON_NUMBER)
	{
		*value = GetJSONValAsUint32(member);
	}
	else
	{
		PrintErrorAtToken(member->start->tokenizer_copy, "Ints are set defined with the following: 123 or { \"random selection\" : [0, 2, 3, 5] } or { \"random range\" : [-10, 30] }");
		return false;
	}
	return true;
}


bool DeserializeVec2(json_value *member, vec2 *v)
{
	if (member->type == JSON_ARRAY && member->array->num_elements == 2)
	{
		if (!DeserializeFloat(member->array->GetByIndex(0), &v->x) || !DeserializeFloat(member->array->GetByIndex(1), &v->y))
		{
			return false;
		}
	}
	else if (member->type == JSON_HASH)
	{
		json_value *circrand = member->hash->GetByKey("circular rand");
		if (!circrand)
		{
			PrintErrorAtToken(circrand->start->tokenizer_copy, "vec2s must be either [23.4, 34.5] or { 'circular rand' : 30.0 }");
			return false;
		}
		float val = GetJSONValAsFloat(circrand);
		*v = glm::circularRand(val);
	}
	else
	{
		PrintErrorAtToken(member->start->tokenizer_copy, "vec2's are specified as 2 member arrays [float, float] or hash with circular rand");
		return false;
	}
	return true;
}


bool DeserializeUvec2(json_value *member, uvec2 *v)
{
	if (member->type == JSON_ARRAY && member->array->num_elements == 2)
	{
		if (!DeserializeUint32(member->array->GetByIndex(0), &v->x) || !DeserializeUint32(member->array->GetByIndex(1), &v->y))
		{
			return false;
		}
	}
	else
	{
		PrintErrorAtToken(member->start->tokenizer_copy, "vec2's are specified as 2 member arrays [float, float]");
		return false;
	}
	return true;
}


bool DeserializeIvec2(json_value *member, ivec2 *v)
{
	if (member->type == JSON_ARRAY && member->array->num_elements == 2)
	{
		if (!DeserializeInt(member->array->GetByIndex(0), &v->x) || !DeserializeInt(member->array->GetByIndex(1), &v->y))
		{
			return false;
		}
	}
	else
	{
		PrintErrorAtToken(member->start->tokenizer_copy, "vec2's are specified as 2 member arrays [float, float]");
		return false;
	}
	return true;
}


bool DeserializeSizeT(json_value *member, size_t *value)
{
	uint32_t val;
	bool result = DeserializeUint32(member, &val);
	return result;
}
