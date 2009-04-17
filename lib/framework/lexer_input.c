#include "frame.h"
#include "lexer_input.h"

int lexer_input(lexerinput_t* input, char* buf, size_t max_size, int nullvalue)
{
	switch (input->type)
	{
		case LEXINPUT_STDIO:
			if (feof(input->input.stdiofile))
			{
				buf[0] = EOF;
				return nullvalue;
			}
			else
			{
				return fread(buf, 1, max_size, input->input.stdiofile);
			}
			break;

		case LEXINPUT_PHYSFS:
			if (PHYSFS_eof(input->input.physfsfile))
			{
				buf[0] = EOF;
				return nullvalue;
			}
			else
			{
				int result = PHYSFS_read(input->input.physfsfile, buf, 1, max_size);
				if (result == -1)
				{
					buf[0] = EOF;
					return nullvalue;
				}
				return result;
			}
			break;

		case LEXINPUT_BUFFER:
			if (input->input.buffer.begin != input->input.buffer.end)
			{
				buf[0] = *input->input.buffer.begin++;
				return 1;
			}
			else
			{
				buf[0] = EOF;
				return nullvalue;
			}
			break;
	}

	ASSERT(!"Invalid input type!", "Invalid input type used for lexer (numeric value: %u)", (unsigned int)input->type);
	return nullvalue;
}
