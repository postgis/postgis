
typedef struct {
	char * string;
	int length; /* length of string, EXCLUDING null termination */
	int size;   /* size of buffer -can be longer than */
} STRBUFF;


extern STRBUFF * new_strBUFF(int size);
extern void delete_StrBUFF(STRBUFF* buff);
extern char* to_CString(STRBUFF * buffer);
extern void add_str_simple(STRBUFF* buffer, char* str);
extern void catenate(STRBUFF *buffer, char* str, int length);
extern int getSize(int buffer_size, int buffer_length, int strLength);
