


/* 删除两边的空格*/
char *trim(char *out, const char *input)
{
	char *p = NULL;
	assert(out != NULL);
	assert(input != NULL);

	while((*input != '\0' && isspace(*input)))
	{
		input++;
	}
	
	while((*input != '\0' && (!isspace(*input))))
	{
		*output = *input;
		output++;
		input++;
	}
	output = '\0';
}
