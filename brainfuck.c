
/* Visual Studio can keep quiet */
#ifdef _MSC_VER
#	define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h> /* malloc, free */ 
#include <stdio.h> /* putchar, getchar, fprintf, FILE functions */
#include <string.h> /* memset */

/* Seems more canonical */
typedef char byte;
/* From the brainfuck specification */
#define MAX_BRAINFUCK 30000 /* !!! */

/**
 * Contains information about "the pointer" and specifies how to handle
 * input and output
 */
struct brainfuck_t
{
	/**
	 * A pointer to "the pointer". A pointer to the pointer is kept so
	 * that this object may be passed around by value while still allowing
	 * to increment both "the pointer" and "the pointer"'s value,
	 */
	byte** the_pointer;

	/**
	 * A function pointer that should specify how to handle output.
	 * i.e. the '.' instruction.
	 */
	int(*output)(int);

	/**
	 * A function pointer that should specify how to handle input.
	 * i.e. the ',' instruction.
	 */
	int(*input)(void);
};

/**
 * Execute the brainfuck code with default memory allocation,
 * input handlers, and output handlers.
 *
 * @param code a NUL-terminated string
 * @param n    number of characters to read and execute
 *
 * @return an error code. Non-zero means something went wrong.
 */
static int do_brainfuck(const char* code, size_t n);

/**
 * Execute a block of brainfuck code.
 *
 * @param code      pointer to the first character in the block of code
 * @param n         number of characters to read and execute
 * @param brainfuck context info
 *
 * @return an error code. Non-zero means something went wrong.
 */
static int do_brainfuck_block(const char* code, size_t n, struct brainfuck_t* brainfuck);


/* Entry point */
int main(int argc, char* argv[])
{
	/* Expects one and only one file name */
	if (argc != 2)
	{
		fprintf(stderr, "Expected a single path as a command line argument. Found %d arguments.\n", argc - 1);
		return -1;
	}

	/* Execute the brainfuck file */
	{
		FILE* f;
		long fsize;
		int ret;
		char* codes;

		/* Open file and determine size */
		f = fopen(argv[1], "r");
		if (f == NULL)
		{
			fprintf(stderr, "Failed to open file '%s'.\n", argv[1]);
		}
		fseek(f, 0, SEEK_END);
		fsize = ftell(f);
		rewind(f);

		/*
		 * Allocate buffer big enough for data and NUL character.
		 * Then fill the buffer and make sure it is NUL-terminated.
		 */
		codes = (char*)malloc(fsize + 1);
		fread(codes, sizeof(char), fsize, f);
		codes[fsize] = '\0';

		/* Do the brainfuck */
		ret = do_brainfuck(codes, fsize);

		/* Close file and free buffer */
		fclose(f);
		free(codes);
		
		return ret;
	}
}




static int do_brainfuck_block(const char* code, size_t n, struct brainfuck_t* brainfuck)
{
	size_t cnt = 0;
	/*
	 * Pointer to the byte that is immediately after n bytes.
	 * i.e. we only read blocks that are less than end.
	 */
	const char* end = code + n;
	/* Just because I'm lazy */
	byte** p = brainfuck->the_pointer;
	
	/* Only read max of n characters */
	while (cnt < n)
	{
		/* The current character */
		char cur = code[cnt];

		/* Instructions */
		switch (cur)
		{
		case '>':
			++(*p);
			break;
		case '<':
			--(*p);
			break;
		case '+':
			++(**p);
			break;
		case '-':
			--(**p);
			break;
		case '.':
			brainfuck->output(**p);
			break;
		case ',':
			**p = (byte)brainfuck->input();
			break;
		/* Begin loop */
		case '[':
		{
			/* Number of begin loops found */
			size_t open_cnt = 1;
			/* Pointer to where the loop begins */
			const char* loop_begin = code + cnt;
			/* Pointer to where the loop ends (begin here though) */
			const char* loop_end = loop_begin + 1;
			/* Number of characters between begin and end */
			size_t loop_len;

			/* Make sure we don't overshoot */
			while (loop_end != end)
			{
				/* There may be nested loops */
				switch (*loop_end)
				{
				case '[':
					++open_cnt;
					break;
				case ']':
					--open_cnt;
					break;
				}
				/* We have found the end of the outermost loop */
				if (open_cnt == 0) break;
				++loop_end;
			}
			/* If we reached the end without finding the end of the loop there is an unmatched '[' */
			if (open_cnt != 0)
			{
				fprintf(stderr, "Unmatched '[' in code.\n");
				return -2;
			}

			loop_len = (size_t)(loop_end - loop_begin);
			/* Execute the loop */
			while (**p)
			{
				int ret = do_brainfuck_block(loop_begin + 1, loop_len - 1, brainfuck);
				if (ret != 0) return ret;
			}
			cnt += loop_len + 1;
			continue;
		}
		case ']':
			/* We don't expect any ']'s to appear. They should all be handled up there. */
			fprintf(stderr, "Unmatched ']' in code.\n");
			return -3;
			break;
		default:
			/* Not a known instruction */
			switch (cur)
			{
			case '\r':
			case '\n':
			case ' ':
			case '\t':
			case '\v':
			case '\f':
				/* These characters are okay to ignore */
				break;
			default:
				/* Illegal characters are not okay */
				fprintf(stderr, "Illegal character: '%c' (int: '%d')\n", cur, (int)cur);
				return -4;
			}
			break;
		}
		/* Next character */
		++cnt;
	}
	return 0;
}

static int do_brainfuck(const char* code, size_t n)
{
	/* Allocate and zero-initialize */
	byte mem[MAX_BRAINFUCK];
	memset(mem, 0, sizeof(mem));

	/* Set up the context info */
	char* the_pointer = mem;
	struct brainfuck_t brainfuck;
	brainfuck.the_pointer = &the_pointer;
	brainfuck.input = &getchar;
	brainfuck.output = &putchar;

	/* Execute the entire code */
	return do_brainfuck_block(code, n, &brainfuck);
}