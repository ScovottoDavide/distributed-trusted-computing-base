#if !defined( READ_AKPUB_H )
#define READ_AKPUB_H

#include "common.h"

#define FILENAME_LEN 10
#define FILE_PEM_LEN 8 // .pub.pem

void cleanUpFolder(char *path);
char* rand_str(size_t length);
int read_AKs_Whitelists(WAM_channel *ch_read, AK_FILE_TABLE *ak_table, WHITELIST_TABLE *whitelist_table, FILE *ak_file, int node_number, volatile int *verifier_status, pthread_mutex_t mutex);

#endif