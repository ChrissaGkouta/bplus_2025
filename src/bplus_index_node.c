
#include "bplus_index_node.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Υλοποίηση βοηθητικών συναρτήσεων για Index Node
void indexnode_init(IndexNode* node) {
    node->count = 0;
    node->is_leaf = 0;

    // Προαιρετικά: Μηδενισμός padding για ασφάλεια/καθαριότητα
    memset(node->padding, 0, sizeof(node->padding));
}

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

@brief Εισάγει ένα νέο κλειδί και τον δεξιό δείκτη του σε έναν κόμβο ευρετηρίου.
 * * Όταν ένας παιδί-κόμβος σπάει, ανεβάζει ένα κλειδί (key) και έναν δείκτη (right_child).
 * Η συνάρτηση αυτή τα τοποθετεί στη σωστή ταξινομημένη θέση.
 * * @param node Ο κόμβος ευρετηρίου.
 * @param key Το κλειδί που ανεβαίνει από το παιδί.
 * @param right_child Ο δείκτης (Block ID) προς το νέο κόμβο που δημιουργήθηκε από το split του παιδιού.
 * @return 0 σε επιτυχία, -1 αν ο κόμβος είναι γεμάτος (χρειάζεται split).
 */
int indexnode_insert_entry(IndexNode* node, int max_keys, int key, int right_child) {
    // 1. Έλεγχος χωρητικότητας
    if (node->count >= max_keys) {
        return -1; // Ο κόμβος είναι γεμάτος, χρειάζεται Index Node Split
    }

    // 2. Εύρεση θέσης εισαγωγής
    // Ψάχνουμε την πρώτη θέση όπου keys[i] > key.
    int i = 0;
    while (i < node->count && node->keys[i] < key) {
        i++;
    }
    
    // Η θέση 'i' είναι εκεί που πρέπει να μπει το νέο κλειδί.
    // Ο δείκτης right_child θα μπει στη θέση 'i+1' των pointers.
    // (Θυμηθείτε: Ένας Index Node έχει Count κλειδιά και Count+1 δείκτες).

    // 3. Μετατόπιση στοιχείων προς τα δεξιά για να ανοίξει χώρος
    
    // Μετακίνηση δεικτών (ξεκινάμε από τον τελευταίο δείκτη στη θέση count+1)
    // Προσοχή: Τα pointers είναι count+1, άρα ο τελευταίος είναι στο index 'node->count'.
    // Θέλουμε να μετακινήσουμε τα pointers από τη θέση i+1 και μετά.
    for (int j = node->count + 1; j > i + 1; j--) {
        node->pointers[j] = node->pointers[j - 1];
    }

    // Μετακίνηση κλειδιών
    for (int j = node->count; j > i; j--) {
        node->keys[j] = node->keys[j - 1];
    }

    // 4. Εισαγωγή νέων τιμών
    node->keys[i] = key;
    node->pointers[i + 1] = right_child;
    
    // 5. Ενημέρωση μετρητή
    node->count++;

    return 0;
}

/**
 * @brief Διασπά έναν γεμάτο κόμβο ευρετηρίου.
 * * Διαφέρει από το Data Split: Το μεσαίο κλειδί ανεβαίνει (Push Up) και αφαιρείται από τους κόμβους.
 * * @param old_node Ο γεμάτος κόμβος.
 * @param new_node Ο νέος (άδειος) κόμβος.
 * @param new_key Το νέο κλειδί που προκάλεσε την υπερχείλιση (ανεβαίνει από παιδί).
 * @param new_pointer Ο δείκτης δεξιά του νέου κλειδιού.
 * @param max_keys Μέγιστο πλήθος κλειδιών.
 * @param pivot_key Δείκτης για να επιστραφεί το κλειδί που θα ανέβει στον πατέρα.
 */
int indexnode_split(IndexNode* old_node, IndexNode* new_node, int new_key, int new_pointer,
                    int max_keys, int* pivot_key) {
    
    // 1. Προσωρινοί πίνακες για ταξινόμηση (Max + 1 κλειδιά, Max + 2 δείκτες)
    // Χρησιμοποιούμε σταθερό μέγεθος για απλότητα, υποθέτοντας ότι χωράνε στο stack.
    // Σε production κώδικα θα θέλαμε malloc αν το Max ήταν μεγάλο.
    int temp_keys[max_keys + 1];
    int temp_pointers[max_keys + 2];
    int total_keys = 0;

    // 2. Αντιγραφή και Ταξινόμηση (Merge Logic)
    int i = 0;
    int inserted = 0;

    // Αντιγραφή του πρώτου δείκτη
    temp_pointers[0] = old_node->pointers[0];

    for (i = 0; i < old_node->count; i++) {
        int curr_key = old_node->keys[i];
        
        if (!inserted && new_key < curr_key) {
            // Εισαγωγή νέου κλειδιού/δείκτη
            temp_keys[total_keys] = new_key;
            temp_pointers[total_keys + 1] = new_pointer;
            total_keys++;
            inserted = 1;
        }

        // Αντιγραφή υπάρχοντος
        temp_keys[total_keys] = old_node->keys[i];
        temp_pointers[total_keys + 1] = old_node->pointers[i+1];
        total_keys++;
    }

    // Αν είναι το μεγαλύτερο, μπαίνει στο τέλος
    if (!inserted) {
        temp_keys[total_keys] = new_key;
        temp_pointers[total_keys + 1] = new_pointer;
        total_keys++;
    }

    // 3. Εύρεση του Pivot (Μεσαίο στοιχείο)
    int split_point = total_keys / 2;
    *pivot_key = temp_keys[split_point]; // Αυτό το κλειδί ανεβαίνει και ΦΕΥΓΕΙ

    // 4. Επαναφορά κόμβων
    old_node->count = 0;
    indexnode_init(new_node);

    // 5. Μοίρασμα (Split)
    
    // Αριστερός Κόμβος (Old Node): Παίρνει τα κλειδιά ΠΡΙΝ το split point
    // και τους αντίστοιχους δείκτες.
    old_node->pointers[0] = temp_pointers[0];
    for (i = 0; i < split_point; i++) {
        old_node->keys[i] = temp_keys[i];
        old_node->pointers[i+1] = temp_pointers[i+1];
        old_node->count++;
    }

    // Δεξιός Κόμβος (New Node): Παίρνει τα κλειδιά ΜΕΤΑ το split point.
    // Ο πρώτος δείκτης του νέου κόμβου είναι ο δείκτης δεξιά του Pivot (temp_pointers[split_point + 1]).
    new_node->pointers[0] = temp_pointers[split_point + 1];
    for (i = split_point + 1; i < total_keys; i++) {
        // Υπολογισμός offset για τον νέο πίνακα (ξεκινάει από 0)
        int new_idx = i - (split_point + 1);
        new_node->keys[new_idx] = temp_keys[i];
        new_node->pointers[new_idx + 1] = temp_pointers[i+1];
        new_node->count++;
    }

    return 0;
}