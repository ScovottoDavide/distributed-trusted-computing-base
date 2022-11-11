#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <openssl/rand.h>
#include "/home/privateadm/WAM/WAM.h"

#define NONCE_LEN 32

void menu(void *in);
void PoC_heartbeat(void *nodes_number_p);
bool legal_int(const char *str);

volatile int heartBeat_status = 0; // 0 -> do not stop; 1 --> stop the process
pthread_mutex_t menuLock;

int main(int argc, char *argv[]) {
    int nodes_number;
    pthread_t th_heartbeat, th_menu;
    if(argc != 2){
        fprintf(stdout, "Usage: ./WAM_heartbeat (number of verifier nodes)\n");
        return -1;
    }
    if(atoi(argv[1]) <= 0 || !legal_int(argv[1])){
        fprintf(stdout, "Entered parameter is NaN or it has to be greater than 0\n");
        return -1;
    }
    nodes_number = atoi(argv[1]);
    pthread_create(&th_heartbeat, NULL, (void *)&PoC_heartbeat, &nodes_number);
    pthread_create(&th_menu, NULL, (void *)&menu, NULL);

    pthread_join(th_heartbeat, NULL);
    pthread_join(th_menu, NULL);
    //PoC_heartbeat(nodes_number);
    return 0;
}

bool legal_int(const char *str) {
    while (*str)
        if (!isdigit(*str++))
            return false;
    return true;
}

void menu(void *in) {
    int input;
    fprintf(stdout, "Press [1] --> Stop Heartbeat\n");
    scanf("%d", &input);
    if(input == 1){
        pthread_mutex_lock(&menuLock); // Lock a mutex for heartBeat_Status
        heartBeat_status = 1;
        pthread_mutex_unlock(&menuLock); // Unlock a mutex for heartBeat_Status
    }
}

void PoC_heartbeat(void *nodes_number_p) {
    int nodes_number = *((int *)nodes_number_p);
	uint8_t mykey[]="supersecretkeyforencryptionalby", nonce[NONCE_LEN], expected_response_messages[DATA_SIZE];
    uint8_t last[4] = "done", **read_response_messages;
	uint32_t expected_response_size = DATA_SIZE, offset[nodes_number], previous_msg_num[nodes_number];
	WAM_AuthCtx a; a.type = AUTHS_NONE;
	WAM_Key k; k.data = mykey; k.data_len = (uint16_t) strlen((char*)mykey);
	
    FILE *index_file;
    int len_file, i, new_nonce_send = 1, received_responses = 0, *responses_map;
    char *data = NULL, prefix_str_index[12]="read_index_", prefix_str_pubK[9]="pub_key_", buf_index_str[100] = {0};

    IOTA_Index file_index, *read_response_indexes;
    WAM_channel ch_send, *ch_read_responses;

    IOTA_Endpoint privatenet = {.hostname = "130.192.86.15",
							 .port = 14265,
							 .tls = false};

    // read the pre-allocated indexes from the file
    index_file = fopen("heartbeat_write.json", "r");
        //get len of file
    fseek(index_file, 0, SEEK_END);
    len_file = ftell(index_file);
    fseek(index_file, 0, SEEK_SET);
        // read the data from the file 
    data = (char*) malloc(len_file + 1 * sizeof(char));
    fread(data, 1, len_file, index_file);
    data[len_file] = '\0';
    fclose(index_file);

    cJSON *json = cJSON_Parse(data);

    read_response_indexes = malloc(nodes_number * sizeof(IOTA_Index));

    hex_2_bin(cJSON_GetObjectItemCaseSensitive(json, "index")->valuestring, INDEX_HEX_SIZE, file_index.index, INDEX_SIZE);
    hex_2_bin(cJSON_GetObjectItemCaseSensitive(json, "pub_key")->valuestring, (ED_PUBLIC_KEY_BYTES * 2) + 1, file_index.keys.pub, ED_PUBLIC_KEY_BYTES);
    hex_2_bin(cJSON_GetObjectItemCaseSensitive(json, "priv_key")->valuestring, (ED_PRIVATE_KEY_BYTES * 2) + 1, file_index.keys.priv, ED_PRIVATE_KEY_BYTES);

    /*
    * char prefix[12]="read_index_";
    * char buf_str[152] = {0};
    * ...
    * snprintf(buf_str, 152, "%s%d", prefix, i+1);
    */

    for(i = 0; i<nodes_number; i++){
        //prefix_str_index[11] = (i + 1) + '0';
        //prefix_str_pubK[8] = (i + 1) + '0';
        snprintf(buf_index_str, 100, "%s%d", prefix_str_index, i+1);
        fprintf(stdout, "%s\n", buf_index_str);
        hex_2_bin(cJSON_GetObjectItemCaseSensitive(json, buf_index_str)->valuestring, INDEX_HEX_SIZE, read_response_indexes[i].index, INDEX_SIZE);
        hex_2_bin(cJSON_GetObjectItemCaseSensitive(json, buf_index_str)->valuestring, (ED_PUBLIC_KEY_BYTES * 2) + 1, read_response_indexes[i].keys.pub, ED_PUBLIC_KEY_BYTES);
    }   

    // Set write index read from the file
    WAM_init_channel(&ch_send, 1, &privatenet, &k, &a);
    set_channel_index_write(&ch_send, file_index);

    ch_read_responses = malloc(nodes_number * sizeof(WAM_channel));
    // Set read indexes read from the file
    for(i = 0; i < nodes_number; i++){
        WAM_init_channel(&ch_read_responses[i], i, &privatenet, &k, &a);
        set_channel_index_read(&ch_read_responses[i], read_response_indexes[i].index);
    }

    read_response_messages = (uint8_t**) malloc(nodes_number * sizeof(uint8_t *));
    for(i = 0; i<nodes_number; i++)
        read_response_messages[i] = malloc(DATA_SIZE * 2 * sizeof(uint8_t));
    responses_map = calloc(nodes_number, sizeof(int));

    while(1){
        if(new_nonce_send){
            if (!RAND_bytes(nonce, NONCE_LEN)) {
                return;
            } else {
                nonce[NONCE_LEN] = '\0';
                printf("NONCE: ");
                for (i = 0; i < NONCE_LEN; i++)
                    printf("%02x", nonce[i]);
                printf("\n");
                WAM_write(&ch_send, nonce, NONCE_LEN, false);   
                fprintf(stdout, "[CH-id=%d] Messages sent: %d (%d bytes)\n", ch_send.id, ch_send.sent_msg, ch_send.sent_bytes);
                new_nonce_send = 0;
            }
            for(i = 0; i < nodes_number; i++){
                ch_read_responses[i].recv_bytes = 0;
                ch_read_responses[i].recv_msg = 0;
                offset[i] = 0;
                previous_msg_num[i] = 0;
            }
        }
        i = 0;
        while(received_responses < nodes_number && !new_nonce_send){
            if(responses_map[i] == 0){
                if(!WAM_read(&ch_read_responses[i], expected_response_messages, &expected_response_size)){
                    if(ch_read_responses[i].recv_msg != previous_msg_num[i]){
                        memcpy((read_response_messages[i] + offset[i]), expected_response_messages, DATA_SIZE);
                        offset[i] += DATA_SIZE;
                        previous_msg_num[i] += 1;
                    }
                    else if(memcmp(last, read_response_messages[i] + ch_read_responses[i].recv_bytes - sizeof last, sizeof last) == 0) {
                        fprintf(stdout, "New response arrived of bytes [%d]\n", ch_read_responses[i].recv_bytes);
                        received_responses+=1;
                        responses_map[i] = 1;
                    }
                }
                if(received_responses == nodes_number){
                    fprintf(stdout, "All responses arrived! Start new cicle.\n");
                    new_nonce_send = 1;
                    received_responses = 0;
                    for(int j = 0; j < nodes_number; j++)
                        responses_map[j] = 0;
                    if(ch_send.sent_bytes >= 32)
                        sleep(10);
                }
            }
            if(i + 1 == nodes_number) i = 0;
                else i+=1;
            pthread_mutex_lock(&menuLock); // Lock a mutex for heartBeat_Status
            if(heartBeat_status == 1){ // stop
                fprintf(stdout, "Stopping...\n");
                pthread_mutex_unlock(&menuLock); // Unlock a mutex for heartBeat_Status
                goto end;
            }
            pthread_mutex_unlock(&menuLock); // Unlock a mutex for heartBeat_Status
        }  
    }
end:
    /*for(i = 0; i < nodes_number; i++){
        if(read_response_messages[i] != NULL)
            free(read_response_messages[i]);
    }
    if(read_response_messages!=NULL) free(read_response_messages);*/
    //if(responses_map!=NULL) free(responses_map);
    //if(data != NULL) free(data);
    //if(ch_read_responses != NULL) free(ch_read_responses);
    //if(read_response_indexes!= NULL) free(read_response_indexes);
    //if(json != NULL) cJSON_Delete(json);
    return ;
}