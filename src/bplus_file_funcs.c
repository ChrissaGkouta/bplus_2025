#include "bplus_file_funcs.h"
#include <stdio.h>

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return bplus_ERROR;     \
    }                         \
  }


int bplus_create_file(const TableSchema *schema, const char *fileName)
{
  return -1;
}


int bplus_open_file(const char *fileName, int *file_desc, BPlusMeta **metadata)
{
  return -1;
}

int bplus_close_file(const int file_desc, BPlusMeta* metadata)
{
  return -1;
}

int bplus_record_insert(const int file_desc, BPlusMeta *metadata, const Record *record)
{

    // Μέσα στην bplus_record_insert... αφού βρεις το φύλλο (leaf_node)

int status = datanode_insert_record(leaf_node, meta->max_records_data, record, &meta->schema);

if (status == 0) {
    // ΕΠΙΤΥΧΙΑ: Η εγγραφή μπήκε, όλα καλά.
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    return 0; // Ή το ID του block όπως ζητάει η εκφώνηση
} 
else if (status == -2) {
    // ΔΙΠΛΟΤΥΠΟ: Απορρίπτεται και "το πετάμε".
    printf("Error: Record with key %d already exists.\n", record_get_key(&meta->schema, record));
    
    // Απλά κάνουμε Unpin χωρίς SetDirty (δεν αλλάξαμε τίποτα)
    BF_UnpinBlock(block);
    return -1; // Επιστρέφουμε γενικό σφάλμα
} 
else if (status == -1) {
    // ΓΕΜΑΤΟ: Εδώ και ΜΟΝΟ ΕΔΩ καλούμε τη διαδικασία Split.
    // ... κώδικας για Allocate νέου block ...
    // ... κώδικας για datanode_split ...
}
  return -1;
}

int bplus_record_find(const int file_desc, const BPlusMeta *metadata, const int key, Record** out_record)
{  
  *out_record=NULL;
  return -1;
}

