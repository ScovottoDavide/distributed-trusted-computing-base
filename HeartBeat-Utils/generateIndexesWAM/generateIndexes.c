#include <string.h>
#include "WAM/WAM.h"

void generateIndexFiles(IOTA_Index *idx_TPA, IOTA_Index *idx_RA, IOTA_Index *idx_AK_Whitelist, IOTA_Index heartBeat_index, int number_of_indexes);

int main(int argc, char *argv[]) {
    IOTA_Index *indexes_TPA, *indexes_RA, *indexes_AK_Whitelist, heartBeat_index;
    FILE *heartbeatWriteIndex_file;
    int number_of_indexes = 0, i;

    if(argc != 2){
        fprintf(stdout, "Usage: ./generateIndexes (number of nodes)\n");
        return -1;
    }
    if(argv[1] != NULL){
        number_of_indexes = atoi(argv[1]);
    }
    if(number_of_indexes <= 0) {
        fprintf(stdout, "Number of indexes must be greater than 0!\n");
        return -1;
    }

    fprintf(stdout, "Generetaing heartbeat index ...\n");
    generate_iota_index(&(heartBeat_index));
    fprintf(stdout, "DONE\n");

    
    indexes_TPA = (IOTA_Index *) malloc(sizeof(IOTA_Index) * number_of_indexes); // write indexes for TpaData  
    if(indexes_TPA == NULL){
        fprintf(stdout, "OOM\n");
        return -1;
    }
    indexes_AK_Whitelist = (IOTA_Index *) malloc(sizeof(IOTA_Index) * number_of_indexes); // write indexes for TPA's AkPub  
    if(indexes_AK_Whitelist == NULL){
        fprintf(stdout, "OOM\n");
        return -1;
    }
    indexes_RA = (IOTA_Index *) malloc(sizeof(IOTA_Index) * number_of_indexes);
    if(indexes_RA == NULL){
        fprintf(stdout, "OOM\n");
        return -1;
    }
    // TPA write indexes --> write quote and log
    for(i = 0; i < number_of_indexes; i++) {
        generate_iota_index(&(indexes_TPA[i]));
    }
    // TPA write indexes --> write AkPub
    for(i = 0; i < number_of_indexes; i++) {
        generate_iota_index(&(indexes_AK_Whitelist[i]));
    }
    // RA write indexes --> write response of verification
    for(i = 0; i < number_of_indexes; i++) {
        generate_iota_index(&(indexes_RA[i]));
    }

    fprintf(stdout, "Generating index files...\n");
    generateIndexFiles(indexes_TPA, indexes_RA, indexes_AK_Whitelist, heartBeat_index, number_of_indexes);
    fprintf(stdout, "DONE\n");

    free(indexes_TPA);
    free(indexes_RA);
    return 0;
}

void generateIndexFiles(IOTA_Index *idx_TPA, IOTA_Index *idx_RA, IOTA_Index *idx_AK_Whitelist, IOTA_Index heartBeat_index, int number_of_indexes) {
    FILE **index_files_TPA, **index_files_RA, *heartbeatWriteIndex_file;
    int i, j, k = 0;
    cJSON *iota_index_json_TPA, *iota_index_json_RA, *heartbeat_json;
    char *out, priv_hex[ED_PRIVATE_KEY_BYTES*2 + 1], pub_hex[ED_PUBLIC_KEY_BYTES*2 +1], index_hex[INDEX_HEX_SIZE];
    char base_index_str[20] = "read_index_"; 
    char base_pub_str[20] = "pub_key_";
    char base_index_str_akpub[20] = "AK_White_read_"; 
    char base_pub_str_akpub[30] = "AK_White_pubkey_";
    char base_index_str_status[30] = "status_read_"; 
    char base_pub_str_status[40] = "status_read_pubkey_";

    heartbeatWriteIndex_file = fopen("heartbeat_write.json", "w");
    if(heartbeatWriteIndex_file == NULL){
        fprintf(stdout, "Cannot open hearbeat index write file!\n");
        return;
    }

    heartbeat_json = cJSON_CreateObject();

    bin_2_hex(heartBeat_index.keys.priv, ED_PRIVATE_KEY_BYTES, priv_hex, (ED_PRIVATE_KEY_BYTES*2 +1));
    bin_2_hex(heartBeat_index.keys.pub, ED_PUBLIC_KEY_BYTES, pub_hex, (ED_PUBLIC_KEY_BYTES*2 +1));
    bin_2_hex(heartBeat_index.index, INDEX_SIZE, index_hex, INDEX_HEX_SIZE);
    cJSON_AddItemToObject(heartbeat_json, "priv_key", cJSON_CreateString(priv_hex));
    cJSON_AddItemToObject(heartbeat_json, "pub_key", cJSON_CreateString(pub_hex));
    cJSON_AddItemToObject(heartbeat_json, "index", cJSON_CreateString(index_hex));

    index_files_TPA = malloc(sizeof(FILE *) * number_of_indexes);
    if(index_files_TPA == NULL){
        fprintf(stdout, "OOM\n");
        return ;
    }
    index_files_RA = malloc(sizeof(FILE *) * number_of_indexes);
    if(index_files_RA == NULL){
        fprintf(stdout, "OOM\n");
        return ;
    }

    for(i = 0; i < number_of_indexes; i++) {
        iota_index_json_TPA = cJSON_CreateObject();
        iota_index_json_RA = cJSON_CreateObject();

        char base_filename_TPA[30] = "TPA_index_node";
        base_filename_TPA[14] = (i + 1) + '0';
        strcat(base_filename_TPA, ".json");
        index_files_TPA[i] = fopen(base_filename_TPA, "w");
        if(index_files_TPA[i] == NULL){
            fprintf(stdout, "Cannot open/create file %s\n", base_filename_TPA);
            return ;
        }

        char base_filename_RA[30] = "RA_index_node";
        base_filename_RA[13] = (i + 1) + '0';
        strcat(base_filename_RA, ".json");
        index_files_RA[i] = fopen(base_filename_RA, "w");
        if(index_files_RA[i] == NULL){
            fprintf(stdout, "Cannot open/create file %s\n", base_filename_RA);
            return ;
        }

        for(j = 0; j < number_of_indexes; j++) {
            if(j == i) { // write index (for TPA [1 for TpaData + 1 for AkPub_and_whitelist] and RA)
                bin_2_hex(idx_TPA[i].keys.priv, ED_PRIVATE_KEY_BYTES, priv_hex, (ED_PRIVATE_KEY_BYTES*2 +1));
                bin_2_hex(idx_TPA[i].keys.pub, ED_PUBLIC_KEY_BYTES, pub_hex, (ED_PUBLIC_KEY_BYTES*2 +1));
                bin_2_hex(idx_TPA[i].index, INDEX_SIZE, index_hex, INDEX_HEX_SIZE);
                cJSON_AddItemToObject(iota_index_json_TPA, "priv_key", cJSON_CreateString(priv_hex));
                cJSON_AddItemToObject(iota_index_json_TPA, "pub_key", cJSON_CreateString(pub_hex));
                cJSON_AddItemToObject(iota_index_json_TPA, "index", cJSON_CreateString(index_hex));

                bin_2_hex(idx_AK_Whitelist[i].keys.priv, ED_PRIVATE_KEY_BYTES, priv_hex, (ED_PRIVATE_KEY_BYTES*2 +1));
                bin_2_hex(idx_AK_Whitelist[i].keys.pub, ED_PUBLIC_KEY_BYTES, pub_hex, (ED_PUBLIC_KEY_BYTES*2 +1));
                bin_2_hex(idx_AK_Whitelist[i].index, INDEX_SIZE, index_hex, INDEX_HEX_SIZE);
                cJSON_AddItemToObject(iota_index_json_TPA, "AK_White_priv_key", cJSON_CreateString(priv_hex));
                cJSON_AddItemToObject(iota_index_json_TPA, "AK_White_pub_key", cJSON_CreateString(pub_hex));
                cJSON_AddItemToObject(iota_index_json_TPA, "AK_White_index", cJSON_CreateString(index_hex));

                bin_2_hex(idx_RA[i].keys.priv, ED_PRIVATE_KEY_BYTES, priv_hex, (ED_PRIVATE_KEY_BYTES*2 +1));
                bin_2_hex(idx_RA[i].keys.pub, ED_PUBLIC_KEY_BYTES, pub_hex, (ED_PUBLIC_KEY_BYTES*2 +1));
                bin_2_hex(idx_RA[i].index, INDEX_SIZE, index_hex, INDEX_HEX_SIZE);
                cJSON_AddItemToObject(iota_index_json_RA, "priv_key", cJSON_CreateString(priv_hex));
                cJSON_AddItemToObject(iota_index_json_RA, "pub_key", cJSON_CreateString(pub_hex));
                cJSON_AddItemToObject(iota_index_json_RA, "index", cJSON_CreateString(index_hex));

                // reading info for heartbeat
                base_index_str[11] = (i + 1) + '0';
                base_pub_str[8] = (i + 1) + '0';
                cJSON_AddItemToObject(heartbeat_json, base_pub_str, cJSON_CreateString(pub_hex));
                cJSON_AddItemToObject(heartbeat_json, base_index_str, cJSON_CreateString(index_hex));
            } else {
                // read indexes for RA to read quote
                bin_2_hex(idx_TPA[j].index, INDEX_SIZE, index_hex, INDEX_HEX_SIZE);
                bin_2_hex(idx_TPA[j].keys.pub, ED_PUBLIC_KEY_BYTES, pub_hex, (ED_PUBLIC_KEY_BYTES*2 +1));
                base_index_str[11] = (k + 1) + '0';
                base_pub_str[8] = (k + 1) + '0';
                cJSON_AddItemToObject(iota_index_json_RA, base_index_str, cJSON_CreateString(index_hex));
                cJSON_AddItemToObject(iota_index_json_RA, base_pub_str, cJSON_CreateString(pub_hex));

                //read indexes for RA to read AkPubs_and_Whitelists
                bin_2_hex(idx_AK_Whitelist[j].index, INDEX_SIZE, index_hex, INDEX_HEX_SIZE);
                bin_2_hex(idx_AK_Whitelist[j].keys.pub, ED_PUBLIC_KEY_BYTES, pub_hex, (ED_PUBLIC_KEY_BYTES*2 +1));
                base_index_str_akpub[14] = (k + 1) + '0';
                base_pub_str_akpub[16] = (k + 1) + '0';
                cJSON_AddItemToObject(iota_index_json_RA, base_index_str_akpub, cJSON_CreateString(index_hex));
                cJSON_AddItemToObject(iota_index_json_RA, base_pub_str_akpub, cJSON_CreateString(pub_hex));

                // read indexes for RA to read others local status
                bin_2_hex(idx_RA[j].index, INDEX_SIZE, index_hex, INDEX_HEX_SIZE);
                bin_2_hex(idx_RA[j].keys.pub, ED_PUBLIC_KEY_BYTES, pub_hex, (ED_PUBLIC_KEY_BYTES*2 +1));
                base_index_str_status[12] = (k + 1) + '0';
                base_pub_str_status[19] = (k + 1) + '0';
                cJSON_AddItemToObject(iota_index_json_RA, base_index_str_status, cJSON_CreateString(index_hex));
                cJSON_AddItemToObject(iota_index_json_RA, base_pub_str_status, cJSON_CreateString(pub_hex));
                k+=1;
            }
        }
        k=0;
        bin_2_hex(heartBeat_index.index, INDEX_SIZE, index_hex, INDEX_HEX_SIZE);
        bin_2_hex(heartBeat_index.keys.pub, ED_PUBLIC_KEY_BYTES, pub_hex, (ED_PUBLIC_KEY_BYTES*2 +1));
        cJSON_AddItemToObject(iota_index_json_TPA, "heartbeat", cJSON_CreateString(index_hex));
        cJSON_AddItemToObject(iota_index_json_TPA, "heartBeat_pub_key", cJSON_CreateString(pub_hex));
        cJSON_AddItemToObject(iota_index_json_RA, "heartbeat", cJSON_CreateString(index_hex));
        cJSON_AddItemToObject(iota_index_json_RA, "heartBeat_pub_key", cJSON_CreateString(pub_hex));
        
        out = cJSON_Print(iota_index_json_TPA);
        fprintf(index_files_TPA[i], "%s", out);
        free(out);
        cJSON_Delete(iota_index_json_TPA);

        out = cJSON_Print(iota_index_json_RA);
        fprintf(index_files_RA[i], "%s", out);
        free(out);
        cJSON_Delete(iota_index_json_RA);

        fclose(index_files_RA[i]);
        fclose(index_files_TPA[i]);
    }

    out = cJSON_Print(heartbeat_json);
    fprintf(heartbeatWriteIndex_file, "%s", out);
    free(out);
    cJSON_Delete(heartbeat_json);

    fclose(heartbeatWriteIndex_file);
}