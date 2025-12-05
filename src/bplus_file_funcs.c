
#include "bplus_file_funcs.h"
#include "bplus_file_structs.h"
#include "bplus_datanode.h"
#include "bplus_index_node.h"
#include "bf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Μέγιστο βάθος δέντρου (για τη στοίβα path). 20 είναι υπεραρκετό για B+ Trees.
#define MAX_PATH_HEIGHT 20

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return bplus_ERROR;     \
    }                         \
  }


// --- Βοηθητικές Συναρτήσεις για Υπολογισμό Μεγεθών ---
// Πρέπει να ταιριάζουν με αυτές που ορίσαμε στα headers,
// ή να χρησιμοποιήσουμε τα macros αν υπάρχουν. Εδώ τα ξαναγράφουμε για σιγουριά.

int get_max_index_keys() {
    // IndexNode: Header + pointers + keys
    // Header ~ 8 bytes
    // Διαθέσιμο ~ 504 bytes
    // (N)*4 + (N+1)*4 <= 504 => 8N + 4 <= 504 => 8N <= 500 => N = 62
    return (BF_BLOCK_SIZE - sizeof(IndexNode)) / (2 * sizeof(int)); 
}

int get_max_data_records(int record_size) {
    // DataNode: Header + records
    // Header ~ 12 bytes
    return (BF_BLOCK_SIZE - sizeof(DataNode)) / record_size;
}


int bplus_create_file(const TableSchema *schema, const char *fileName)
{
    int file_desc;
    BF_Block *block;
    BPlusMeta *meta_data;

    // 1. Δημιουργία Αρχείου
    if (BF_CreateFile(fileName) != BF_OK) {
        return -1;
    }

    CALL_BF(BF_OpenFile(fileName, &file_desc));

    // 2. Δέσμευση Block 0 (Metadata)
    BF_Block_Init(&block);
    CALL_BF(BF_AllocateBlock(file_desc, block));

    meta_data = (BPlusMeta *)BF_Block_GetData(block);
    
    // Αρχικοποίηση Metadata
    meta_data->magic_number = BPLUS_MAGIC_NUMBER;
    meta_data->schema = *schema;
    meta_data->root_block_id = 1; // Η ρίζα θα είναι το Block 1
    meta_data->height = 1;        // Αρχικό ύψος
    meta_data->next_block_id = 2; // Επόμενο διαθέσιμο block

    // Υπολογισμός χωρητικοτήτων
    // (Χρησιμοποιούμε τις συναρτήσεις από τα datanode.h / index_node.h αν υπάρχουν ή τοπικές)
    // Εδώ χρησιμοποιούμε την calculate_max_data_records που φτιάξαμε στο datanode.c
    meta_data->max_records_data = calculate_max_data_records(schema->record_size);
    
    // Για το index node, κάνουμε έναν ασφαλή υπολογισμό εδώ ή καλούμε αντίστοιχη συνάρτηση
    int index_header_size = sizeof(int) + sizeof(char) + 3; // count + is_leaf + padding
    int space_for_entries = BF_BLOCK_SIZE - index_header_size;
    // 4 bytes key + 4 bytes pointer = 8 bytes per entry (περίπου)
    meta_data->max_keys_index = (space_for_entries - sizeof(int)) / (2 * sizeof(int));

    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));

    // 3. Δέσμευση Block 1 (Πρώτη Ρίζα - Data Node)
    CALL_BF(BF_AllocateBlock(file_desc, block));
    DataNode* root = (DataNode*)BF_Block_GetData(block);
    
    datanode_init(root, -1); // next_block_id = -1 (τέλος λίστας)

    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);

    CALL_BF(BF_CloseFile(file_desc));
  return 0;
}


int bplus_open_file(const char *fileName, int *file_desc, BPlusMeta **metadata)
{
  BF_Block *block;
    
    CALL_BF(BF_OpenFile(fileName, file_desc));
    
    BF_Block_Init(&block);
    CALL_BF(BF_GetBlock(*file_desc, 0, block)); // Block 0

    *metadata = (BPlusMeta *)malloc(sizeof(BPlusMeta));
    if (*metadata == NULL) {
        return -1;
    }

    memcpy(*metadata, BF_Block_GetData(block), sizeof(BPlusMeta));

    // Έλεγχος Magic Number
    if ((*metadata)->magic_number != BPLUS_MAGIC_NUMBER) {
        printf("Error: Invalid B+ Tree file.\n");
        free(*metadata);
        *metadata = NULL;
        BF_UnpinBlock(block); // Δεν είναι dirty
        BF_Block_Destroy(&block);
        return -1;
    }

    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);
  return 0;
}

int bplus_close_file(const int file_desc, BPlusMeta* metadata)
{
  if (metadata != NULL) {
        BF_Block *block;
        BF_Block_Init(&block);
        
        // Ενημέρωση του Block 0 πριν κλείσουμε
        if (BF_GetBlock(file_desc, 0, block) == BF_OK) {
            memcpy(BF_Block_GetData(block), metadata, sizeof(BPlusMeta));
            BF_Block_SetDirty(block);
            BF_UnpinBlock(block);
        }
        BF_Block_Destroy(&block);
        free(metadata);
    }
    
    CALL_BF(BF_CloseFile(file_desc));
  return -1;
}

int bplus_record_insert(const int file_desc, BPlusMeta *metadata, const Record *record)
{
// 1. Έλεγχος αν υπάρχει ήδη η εγγραφή (Απαίτηση εκφώνησης)
    Record* temp_rec;
    if (bplus_record_find(file_desc, metadata, record_get_key(&metadata->schema, record), &temp_rec) == 0) {
        printf("Error: Key already exists. Insertion rejected.\n");
        free(temp_rec);
        return -1;
    }

    BF_Block *block;
    BF_Block_Init(&block);
    
    // Στοίβα για να κρατάμε το μονοπάτι (Path) προς τη ρίζα
    int path[MAX_PATH_HEIGHT];
    int path_depth = 0;

    int curr_block_id = metadata->root_block_id;

    // 2. Κατάβαση στο δέντρο για εύρεση του φύλλου
    int is_leaf = 0;
    while (!is_leaf) {
        path[path_depth++] = curr_block_id; // Αποθήκευση στο path
        
        CALL_BF(BF_GetBlock(file_desc, curr_block_id, block));
        IndexNode* inode = (IndexNode*)BF_Block_GetData(block);

        if (inode->is_leaf) {
            is_leaf = 1;
            // Κρατάμε το block δεσμευμένο (pinned) γιατί θα γράψουμε σε αυτό
        } else {
            int next_block = indexnode_find_child(inode, record_get_key(&metadata->schema, record));
            CALL_BF(BF_UnpinBlock(block));
            curr_block_id = next_block;
        }
    }

    // 3. Προσπάθεια εισαγωγής στο φύλλο
    DataNode* leaf = (DataNode*)BF_Block_GetData(block);
    int status = datanode_insert_record(leaf, metadata->max_records_data, record, &metadata->schema);

    if (status == 0) {
        // Επιτυχία - Χώρεσε χωρίς split
        BF_Block_SetDirty(block);
        CALL_BF(BF_UnpinBlock(block));
        BF_Block_Destroy(&block);
        return 0; 
    } else if (status == -2) {
        // Διπλότυπο (αν δεν το έπιασε το find παραπάνω)
        CALL_BF(BF_UnpinBlock(block));
        BF_Block_Destroy(&block);
        return -1;
    }

    // 4. Υπερχείλιση Φύλλου -> SPLIT
    // Χρειαζόμαστε νέο block
    BF_Block *new_block;
    BF_Block_Init(&new_block);
    CALL_BF(BF_AllocateBlock(file_desc, new_block));
    
    int new_block_id;
    BF_GetBlockCounter(file_desc, &new_block_id);
    new_block_id--; // Το AllocateBlock δεσμεύει στο τέλος, άρα count-1

    DataNode* new_leaf_node = (DataNode*)BF_Block_GetData(new_block);
    int pivot_key;

    // Εκτέλεση Split
    if (datanode_split(leaf, new_leaf_node, new_block_id, record, 
                       metadata->max_records_data, &metadata->schema, &pivot_key) != 0) {
        // Error handling
        BF_UnpinBlock(block); BF_UnpinBlock(new_block);
        BF_Block_Destroy(&block); BF_Block_Destroy(&new_block);
        return -1;
    }

    // Τα δύο blocks είναι έτοιμα και dirty
    BF_Block_SetDirty(block);
    BF_Block_SetDirty(new_block);
    CALL_BF(BF_UnpinBlock(block));      // Ξε-καρφιτσώνουμε το παλιό φύλλο
    CALL_BF(BF_UnpinBlock(new_block));  // Ξε-καρφιτσώνουμε το νέο φύλλο
    
    // Απελευθέρωση μνήμης των struct pointers (όχι των blocks στον δίσκο)
    BF_Block_Destroy(&block);
    BF_Block_Destroy(&new_block);

    // 5. Προπαγάνδιση προς τα πάνω (Propagate Up)
    // Πρέπει να βάλουμε το (pivot_key, new_block_id) στον πατέρα
    int key_to_insert = pivot_key;
    int right_child_id = new_block_id;

    // Loop προς τα πίσω στο path (από τον πατέρα του φύλλου προς τη ρίζα)
    // path[path_depth - 1] είναι το φύλλο. Ο πατέρας είναι path[path_depth - 2].
    
    for (int i = path_depth - 2; i >= 0; i--) {
        int parent_id = path[i];
        
        BF_Block *parent_block;
        BF_Block_Init(&parent_block);
        CALL_BF(BF_GetBlock(file_desc, parent_id, parent_block));
        IndexNode* parent_node = (IndexNode*)BF_Block_GetData(parent_block);

        // Προσπάθεια εισαγωγής στον Index Node
        if (indexnode_insert_entry(parent_node, metadata->max_keys_index, key_to_insert, right_child_id) == 0) {
            // Χώρεσε! Τέλος αναδρομής.
            BF_Block_SetDirty(parent_block);
            CALL_BF(BF_UnpinBlock(parent_block));
            BF_Block_Destroy(&parent_block);
            return 0;
        }

        // Ο Index Node γέμισε -> Split Index Node
        BF_Block *new_idx_block;
        BF_Block_Init(&new_idx_block);
        CALL_BF(BF_AllocateBlock(file_desc, new_idx_block));
        
        int new_idx_id;
        BF_GetBlockCounter(file_desc, &new_idx_id);
        new_idx_id--;

        IndexNode* new_idx_node = (IndexNode*)BF_Block_GetData(new_idx_block);
        int up_key;

        indexnode_split(parent_node, new_idx_node, key_to_insert, right_child_id, 
                        metadata->max_keys_index, &up_key);

        // Ενημέρωση μεταβλητών για τον επόμενο γύρο (προς τα πάνω)
        key_to_insert = up_key;
        right_child_id = new_idx_id;

        BF_Block_SetDirty(parent_block);
        BF_Block_SetDirty(new_idx_block);
        CALL_BF(BF_UnpinBlock(parent_block));
        CALL_BF(BF_UnpinBlock(new_idx_block));
        
        BF_Block_Destroy(&parent_block);
        BF_Block_Destroy(&new_idx_block);
    }

    // 6. Split Ρίζας (Αν φτάσαμε εδώ, σημαίνει ότι και η παλιά ρίζα διασπάστηκε)
    // Πρέπει να φτιάξουμε ΝΕΑ ρίζα που θα δείχνει στην παλιά (path[0]) και στη νέα (right_child_id)
    
    BF_Block *new_root_block;
    BF_Block_Init(&new_root_block);
    CALL_BF(BF_AllocateBlock(file_desc, new_root_block));
    
    int new_root_id;
    BF_GetBlockCounter(file_desc, &new_root_id);
    new_root_id--;

    IndexNode* new_root = (IndexNode*)BF_Block_GetData(new_root_block);
    indexnode_init(new_root);

    // Η νέα ρίζα έχει 1 κλειδί και 2 δείκτες
    new_root->keys[0] = key_to_insert;
    new_root->pointers[0] = metadata->root_block_id; // Αριστερό παιδί (παλιά ρίζα)
    new_root->pointers[1] = right_child_id;          // Δεξί παιδί (το νέο block από το split)
    new_root->count = 1;

    // Ενημέρωση Metadata
    metadata->root_block_id = new_root_id;
    metadata->height++;

    BF_Block_SetDirty(new_root_block);
    CALL_BF(BF_UnpinBlock(new_root_block));
    BF_Block_Destroy(&new_root_block);

    return 0; // Επιτυχία
}

int bplus_record_find(const int file_desc, const BPlusMeta *metadata, const int key, Record** out_record)
{  
  *out_record=NULL;
  int curr_block_id = metadata->root_block_id;
    BF_Block *block;
    BF_Block_Init(&block);

    // 1. Διάσχιση από τη Ρίζα μέχρι το Φύλλο
    // Χρησιμοποιούμε το height για να ξέρουμε πόσα επίπεδα θα κατέβουμε
    // ή ελέγχουμε το is_leaf flag του κόμβου.
    
    int is_leaf = 0;
    while (!is_leaf) {
        CALL_BF(BF_GetBlock(file_desc, curr_block_id, block));
        char* data = BF_Block_GetData(block);
        
        // Ελέγχουμε αν είναι φύλλο (κοινή δομή στην αρχή: count, is_leaf)
        // Υποθέτουμε ότι το is_leaf είναι στη σωστή θέση και στα δύο structs.
        // Ασφαλής τρόπος: check height loop ή cast σε IndexNode και έλεγχος is_leaf.
        IndexNode* inode = (IndexNode*)data;
        
        if (inode->is_leaf) {
            is_leaf = 1;
            // Μην κάνετε unpin εδώ, χρειαζόμαστε το block για την αναζήτηση δεδομένων
        } else {
            // Είναι Index Node -> Βρες το επόμενο παιδί
            int next_block = indexnode_find_child(inode, key);
            CALL_BF(BF_UnpinBlock(block)); // Τελειώσαμε με αυτόν τον κόμβο
            curr_block_id = next_block;
        }
    }

    // 2. Αναζήτηση στο Φύλλο (το block περιέχει το DataNode)
    DataNode* dnode = (DataNode*)BF_Block_GetData(block);
    int result = datanode_find_record(dnode, key, &metadata->schema, out_record);

    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);

    return result; // 0 (Found) or -1 (Not Found)
}

