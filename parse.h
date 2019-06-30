#define BUFF_SIZE (10*1024)
int nextToken(char * buf, FILE * file);
int nextIntToken(char * token, FILE * file, int * result);
int nextStringToken(char * token, FILE * file, char * result);
int nextFloatToken(char * token, FILE * file, float * result);
int from_zander_file(FILE * file, FILE * out);
int convert_old_cnf(FILE * file, FILE * out);

