#include "postgres.h"
#include <string.h>
#include "stdio.h"
#include <malloc.h>
#include <memory.h>
#include "stringBuffer.h"

#define DEFAULT_STR_LENGTH_PADDING 10
#define DEFAULT_PERCENT_SIZE_INCREASE 0.25

//NOTE: buffer->length does NOT include the null!!!




//constructor for the string buffer
STRBUFF * new_strBUFF(int size) {
	STRBUFF * buffer;
	buffer = (STRBUFF *)palloc(sizeof(STRBUFF));
	buffer->string = (char * )palloc(size);
	buffer->size = size;
	buffer->length = 0;
	return buffer;
}

//destructor
void delete_StrBUFF(STRBUFF* buffer) {
	pfree(buffer->string);
	pfree(buffer);
}

//returns a CString contained in the buffer
char* to_CString(STRBUFF* buffer) {
	char* resultStr;
	if(buffer->length == 0) {
		return NULL;
	}

	resultStr = (char * )palloc(buffer->length+1);
	memcpy(resultStr, buffer->string, buffer->length+1);
	return resultStr;
}

//add a string to the buffer- calls catenate
void add_str_simple(STRBUFF* buffer, char* str) {
	if(str == NULL) {
		return;
	}
	catenate(buffer, str, strlen(str));
}

//adds the new string to the existing string in the buffer
//allocates
void catenate(STRBUFF *buffer, char* str, int strLength)
{
	if  (buffer->size <= (buffer->length+strLength)  ) // not big enough to hold this + null termination
	{
		//need to re-allocate the buffer so its bigger
		char *old_buffer = buffer->string;
		int  new_size    = getSize(buffer->size, buffer->length, strLength);  //new size (always big enough)

		buffer->string = palloc(new_size);
		buffer->size = new_size;
		memcpy(buffer->string, old_buffer, buffer->length+1); // copy old string (+1 = null term)

			// buff is exactly the same as it was, except it has a bigger string buffer
		pfree(old_buffer);
	}

		//add new info
	memcpy(buffer->string + buffer->length, str, strLength);
	buffer->length += strLength;
	buffer->string[buffer->length] = 0;// force null-terminated
}


// get new buffer size
//  new size =    <big enough to put in the requested info> +
//                        10 +                                 //just a little constant
//                        25% * <current buffer size>          //exponential growth
int getSize(int buffer_size, int buffer_length, int strLength)
{
	int needed_extra = strLength - (buffer_size-buffer_length); // extra space required in buffer

	return buffer_size + needed_extra + DEFAULT_STR_LENGTH_PADDING + buffer_size*DEFAULT_PERCENT_SIZE_INCREASE;
}

