#ifndef BP_DATANODE_H
#define BP_DATANODE_H
/* Στο αντίστοιχο αρχείο .h μπορείτε να δηλώσετε τις συναρτήσεις
 * και τις δομές δεδομένων που σχετίζονται με τους Κόμβους Δεδομένων.*/

#endif

// include/bplus_datanode.h
#ifndef BP_DATANODE_H
#define BP_DATANODE_H

#include "bf.h"
#include "record.h"
#include "bplus_file_structs.h"

// Δομή Κόμβου Δεδομένων (Data Node / Leaf)
typedef struct {
    int count;                  // Τρέχον πλήθος εγγραφών στον κόμβο
    int next_block_id;          // Block ID του επόμενου φύλλου (-1 αν είναι το τελευταίο)
    char is_leaf;               // 1 (πάντα) για Data Node
    char padding[3];            // Padding για ευθυγράμμιση
    Record records[1];          // Το πρώτο στοιχείο του πίνακα εγγραφών (το μέγεθος καθορίζεται δυναμικά)
} DataNode;

// Δηλώσεις βοηθητικών συναρτήσεων
void datanode_init(DataNode* node, int next_block_id, int max_records);
int datanode_insert_record(DataNode* node, BPlusMeta* meta, const Record* record);
// ... δηλώσεις για split, find_record, κ.λπ.

#endif