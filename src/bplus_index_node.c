// Μπορείτε να προσθέσετε εδώ βοηθητικές συναρτήσεις για την επεξεργασία Κόμβων Δεδομένων.


// src/bplus_index_node.c
#include "bplus_index_node.h"
#include <string.h>
#include <stdlib.h>

// Υλοποίηση βοηθητικών συναρτήσεων για Index Node
void indexnode_init(IndexNode* node) {
    node->count = 0;
    node->is_leaf = 0;

    // Προαιρετικά: Μηδενισμός padding για ασφάλεια/καθαριότητα
    memset(node->padding, 0, sizeof(node->padding));
}

// ... άλλες συναρτήσεις (π.χ. indexnode_find_child, indexnode_insert_entry, indexnode_split) ...
// ... (indexnode_init, κ.λπ. πρέπει να υλοποιηθούν) ...



/**
 * @brief Βρίσκει το επόμενο Block ID (παιδί) στο οποίο πρέπει να συνεχιστεί η αναζήτηση.
 * * Λογική:
 * - Αν key < Keys[0] -> Pointers[0]
 * - Αν Keys[i] <= key < Keys[i+1] -> Pointers[i+1]
 * - Αν key >= Keys[τελευταίο] -> Pointers[τελευταίο]
 * * @param node Ο δείκτης στον κόμβο ευρετηρίου.
 * @param key Το κλειδί που ψάχνουμε (ή που θέλουμε να εισάγουμε).
 * @return int Το Block ID του παιδιού.
 */
int indexnode_find_child(const IndexNode* node, int key) {
    // Σαρώνουμε τα κλειδιά που υπάρχουν στον κόμβο
    for (int i = 0; i < node->count; i++) {
        // Αν το κλειδί αναζήτησης είναι μικρότερο από το τρέχον κλειδί του κόμβου,
        // τότε πρέπει να ακολουθήσουμε τον αριστερό δείκτη (i).
        if (key < node->keys[i]) {
            return node->pointers[i];
        }
    }
    
    // Αν φτάσαμε εδώ, σημαίνει ότι το key είναι μεγαλύτερο ή ίσο από ΟΛΑ τα κλειδιά του κόμβου.
    // Επομένως, ακολουθούμε τον τελευταίο δείκτη (τον δεξιότερο).
    return node->pointers[node->count];
}

// ΝΕΑ ΥΛΟΠΟΙΗΣΗ: Εκτύπωση κόμβου ευρετηρίου
void indexnode_print(const IndexNode* node) {
    printf("--- Index Node ---\n");
    printf("Count: %d\n", node->count);
    printf("Pointers/Keys: (P0: %d) ", node->pointers[0]);
    for (int i = 0; i < node->count; i++) {
        printf("[Key: %d, P%d: %d] ", node->keys[i], i + 1, node->pointers[i+1]);
    }
    printf("\n------------------\n");
}