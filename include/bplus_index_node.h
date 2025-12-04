#ifndef BP_INDEX_NODE_H
#define BP_INDEX_NODE_H
/* Στο αντίστοιχο αρχείο .h μπορείτε να δηλώσετε τις συναρτήσεις
 * και τις δομές δεδομένων που σχετίζονται με τους Κόμβους Δεδομένων.*/

#endif

// include/bplus_index_node.h
#ifndef BP_INDEX_NODE_H
#define BP_INDEX_NODE_H

#include "bf.h"
#include "bplus_file_structs.h"

// Δομή Κόμβου Ευρετηρίου (Index Node / Internal Node)
typedef struct {
    int count;                  // Τρέχον πλήθος κλειδιών στον κόμβο
    char is_leaf;               // 0 (πάντα) για Index Node
    char padding[3];            // Padding
    int pointers[1];            // Το πρώτο στοιχείο του πίνακα δεικτών (Block IDs)
    int keys[1];                // Το πρώτο στοιχείο του πίνακα κλειδιών
} IndexNode;

// Δηλώσεις βοηθητικών συναρτήσεων
void indexnode_init(IndexNode* node);
// ... δηλώσεις για find_child, insert_entry, split, κ.λπ.

#endif

// include/bplus_index_node.h

// ... (Οι προηγούμενοι ορισμοί IndexNode, indexnode_init παραμένουν) ...

// ΠΡΟΣΘΗΚΗ: Συνάρτηση εκτύπωσης Index Node
void indexnode_print(const IndexNode* node);

// ΠΡΟΣΘΗΚΗ: Συνάρτηση εύρεσης επόμενου block ID για αναζήτηση
int indexnode_find_child(const IndexNode* node, int key);

#endif