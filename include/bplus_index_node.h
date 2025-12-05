
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

/**
 * @brief Αρχικοποιεί έναν κόμβο ευρετηρίου.
 * @param node Δείκτης στον κόμβο.
 */
void indexnode_init(IndexNode* node);

/**
 * @brief Βρίσκει το επόμενο Block ID (παιδί) για αναζήτηση.
 * @param node Ο κόμβος ευρετηρίου.
 * @param key Το κλειδί που ψάχνουμε.
 * @return Το Block ID του παιδιού.
 */
int indexnode_find_child(const IndexNode* node, int key);

/**
 * @brief Εισάγει ένα entry (Κλειδί, Δείκτης) στον κόμβο.
 * @param node Ο κόμβος ευρετηρίου.
 * @param max_keys Μέγιστο πλήθος κλειδιών.
 * @param key Το κλειδί που ανεβαίνει από το παιδί.
 * @param right_child Ο δείκτης δεξιά του κλειδιού.
 * @return 0 σε επιτυχία, -1 αν χρειάζεται split.
 */
int indexnode_insert_entry(IndexNode* node, int max_keys, int key, int right_child);

/**
 * @brief Διασπά έναν γεμάτο κόμβο ευρετηρίου.
 * @param old_node Ο παλιός κόμβος.
 * @param new_node Ο νέος κόμβος.
 * @param new_key Το κλειδί προς εισαγωγή.
 * @param new_pointer Ο δείκτης προς εισαγωγή.
 * @param max_keys Μέγιστη χωρητικότητα.
 * @param pivot_key Δείκτης για επιστροφή του Pivot κλειδιού (Push Up).
 * @return 0 σε επιτυχία.
 */
int indexnode_split(IndexNode* old_node, IndexNode* new_node, int new_key, int new_pointer,
                    int max_keys, int* pivot_key);

/**
 * @brief Εκτυπώνει τον κόμβο ευρετηρίου.
 */
void indexnode_print(const IndexNode* node);

#endif