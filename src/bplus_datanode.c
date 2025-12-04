// Μπορείτε να προσθέσετε εδώ βοηθητικές συναρτήσεις για την επεξεργασία Κόμβων toy Ευρετηρίου.

#include "bplus_datanode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h> // Για τη χρήση της offsetof

/**
 * @brief Υπολογίζει το μέγιστο πλήθος εγγραφών που χωράνε σε ένα Data Node.
 * Χρησιμοποιεί την offsetof για να λάβει υπόψη το padding της δομής.
 */
int calculate_max_data_records(int record_size) {
    // Χρησιμοποιούμε το offsetof για να βρούμε ακριβώς πού ξεκινούν τα records στη μνήμη.
    // Αυτό είναι πιο ασφαλές από το να αθροίζουμε τα sizeof των πεδίων χειροκίνητα.
    size_t header_size = offsetof(DataNode, records);
    
    size_t available_space = BF_BLOCK_SIZE - header_size;
    
    // ΣΗΜΕΙΩΣΗ: Εφόσον η δομή DataNode χρησιμοποιεί 'Record records[1]', 
    // ο compiler δεσμεύει χώρο με βάση το sizeof(Record). 
    // Επομένως, διαιρούμε με το sizeof(Record) ανεξαρτήτως του logical record_size του schema,
    // σύμφωνα με την παραδοχή της εργασίας για σταθερό μέγεθος εγγραφών.
    return (int)(available_space / sizeof(Record));
}

/**
 * @brief Αρχικοποιεί έναν κόμβο δεδομένων.
 */
void datanode_init(DataNode* node, int next_block_id) {
    node->count = 0;
    node->next_block_id = next_block_id;
    node->is_leaf = 1; // Πάντα 1 για Data Nodes
    // Προαιρετικά μηδενίζουμε το padding για καθαριότητα
    memset(node->padding, 0, sizeof(node->padding));
}

/**
 * @brief Εισάγει μια εγγραφή στον κόμβο διατηρώντας την ταξινόμηση.
 * @return 0 σε επιτυχία, -1 σε αποτυχία (π.χ. αν ο κόμβος είναι γεμάτος ή υπάρχει διπλότυπο).
 */
int datanode_insert_record(DataNode* node, int max_records, const Record* record, const TableSchema* schema) {
    // 1. Έλεγχος χωρητικότητας
    if (node->count >= max_records) {
        // Ο κόμβος είναι γεμάτος, η εισαγωγή δεν μπορεί να γίνει εδώ (χρειάζεται split)
        return -1;
    }

    int new_key = record_get_key(schema, record);
    int insert_pos = 0;

    // 2. Εύρεση της θέσης εισαγωγής (Ταξινομημένη λίστα)
    while (insert_pos < node->count) {
        int curr_key = record_get_key(schema, &node->records[insert_pos]);
        
        if (curr_key == new_key) {
            printf("Error: Duplicate key %d found in Data Node. Insertion rejected.\n", new_key);
            return -1; // Απόρριψη διπλοτύπου
        }
        
        if (curr_key > new_key) {
            break; // Βρέθηκε η θέση, όλα τα επόμενα είναι μεγαλύτερα
        }
        insert_pos++;
    }

    // 3. Μετατόπιση εγγραφών προς τα δεξιά για να δημιουργηθεί χώρος
    // Ξεκινάμε από το τέλος και πάμε προς τη θέση insert_pos
    for (int i = node->count; i > insert_pos; i--) {
        node->records[i] = node->records[i - 1];
    }

    // 4. Εισαγωγή της νέας εγγραφής
    node->records[insert_pos] = *record;
    node->count++;

    return 0;
}

/**
 * @brief Αναζητά μια εγγραφή με βάση το κλειδί.
 * @return 0 αν βρέθηκε, -1 αν δεν βρέθηκε.
 */
int datanode_find_record(const DataNode* node, int key, const TableSchema* schema, Record** out_record) {
    for (int i = 0; i < node->count; i++) {
        int curr_key = record_get_key(schema, &node->records[i]);
        
        if (curr_key == key) {
            // Βρέθηκε! Δέσμευση μνήμης και αντιγραφή
            *out_record = (Record*)malloc(sizeof(Record));
            if (*out_record == NULL) {
                return -1; // Αποτυχία malloc
            }
            // Αντιγραφή περιεχομένου
            memcpy(*out_record, &node->records[i], sizeof(Record));
            return 0; // Επιτυχία
        }
        
        // Βελτιστοποίηση: Αν βρούμε κλειδί μεγαλύτερο από αυτό που ψάχνουμε,
        // σταματάμε (αφού ο πίνακας είναι ταξινομημένος).
        if (curr_key > key) {
            return -1; // Δεν βρέθηκε
        }
    }
    return -1; // Δεν βρέθηκε
}

/**
 * @brief Εκτυπώνει τα περιεχόμενα του κόμβου δεδομένων.
 */
void datanode_print(const DataNode* node, const TableSchema* schema) {
    printf("--- Data Node ---\n");
    printf("Type: Leaf Node\n");
    printf("Count: %d\n", node->count);
    printf("Next Block ID: %d\n", node->next_block_id);
    printf("Records:\n");
    
    for (int i = 0; i < node->count; i++) {
        printf("  [%d]: ", i);
        record_print(schema, &node->records[i]);
    }
    printf("-----------------\n");
}