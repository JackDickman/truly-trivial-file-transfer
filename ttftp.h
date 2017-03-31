#define DATA 512
#define FILENAME 256
#define ERRMSG 256

#define RRQOP 1
#define DATAOP 3
#define ERROP 5

#define ERRCODE 1

typedef enum {
  false,
  true
} boolean;

typedef struct {
  short opcode;
  char filename[FILENAME];
  char nullterm;
} RRQStruct;

typedef struct {
  short opcode;
  short blocknum;
  char data[DATA];
} DataStruct;

typedef struct {
  short opcode;
  short blocknum;
} AckStruct;

typedef struct {
  short opcode;
  short errcode;
  char errmsg[ERRMSG];
  char nullterm;
} ErrorStruct;

int client(char * host, int lport, char * file, int verbose);
int server(int lport, int loop, int verbose);
