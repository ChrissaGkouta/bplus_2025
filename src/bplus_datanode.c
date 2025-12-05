// Μπορείτε να προσθέσετε εδώ βοηθητικές συναρτήσεις για την επεξεργασία Κόμβων toy Ευρετηρίου.

#include "bplus_datanode.h"
#include "bf.h"
#include "record.h"
#include "bplus_file_structs.h"
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
            return -2; // Απόρριψη διπλοτύπου
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

/**
 * @brief Διασπά έναν γεμάτο κόμβο δεδομένων σε δύο, μοιράζοντας τις εγγραφές.
 * * @param old_node Ο δείκτης στον γεμάτο (υπάρχοντα) κόμβο.
 * @param new_node Ο δείκτης στον νέο (άδειο) κόμβο που μόλις δεσμεύτηκε.
 * @param new_block_id Το Block ID του νέου κόμβου (για τη σύνδεση της λίστας).
 * @param new_record Η νέα εγγραφή που προκάλεσε την υπερχείλιση.
 * @param max_records Το μέγιστο πλήθος εγγραφών ανά κόμβο.
 * @param schema Το σχήμα του πίνακα (για ανάγνωση κλειδιών).
 * @param pivot_key Δείκτης για να επιστραφεί το κλειδί-διαχωριστής που θα ανέβει στον γονέα.
 * @return 0 σε επιτυχία, -1 αν υπάρχει διπλότυπο κλειδί.
 */
int datanode_split(DataNode* old_node, DataNode* new_node, int new_block_id, 
                   const Record* new_record, int max_records, 
                   const TableSchema* schema, int* pivot_key) {
    
    // 1. Δημιουργία προσωρινού πίνακα που χωράει ΟΛΕΣ τις εγγραφές (παλιές + νέα)
    // Χρειαζόμαστε χώρο για max_records + 1
    // Προσοχή: Αν οι εγγραφές είναι πολύ μεγάλες, ίσως χρειαστεί malloc αντί για stack.
    // Εδώ υποθέτουμε ότι χωράνε στη στοίβα (stack) επειδή το block είναι μικρό (512b).
    Record temp_records[max_records + 1];
    int total_records = 0;
    int inserted_new = 0; // Flag για το αν μπήκε η νέα εγγραφή

    int new_key = record_get_key(schema, new_record);

    // 2. Αντιγραφή και Ταξινόμηση (Merge Sort λογική)
    // Διαβάζουμε από τον old_node και τοποθετούμε στο temp_records μαζί με το new_record
    int old_idx = 0;
    while (old_idx < old_node->count) {
        int curr_key = record_get_key(schema, &old_node->records[old_idx]);

        if (!inserted_new && new_key < curr_key) {
            // Εδώ μπαίνει η νέα εγγραφή
            temp_records[total_records++] = *new_record;
            inserted_new = 1;
        } else if (new_key == curr_key) {
            printf("Error: Duplicate key %d during split.\n", new_key);
            return -1;
        }

        // Αντιγραφή υπάρχουσας εγγραφής
        temp_records[total_records++] = old_node->records[old_idx++];
    }

    // Αν η νέα εγγραφή είναι η μεγαλύτερη από όλες, μπαίνει στο τέλος
    if (!inserted_new) {
        temp_records[total_records++] = *new_record;
    }

    // 3. Υπολογισμός σημείου διαχωρισμού (Split Point)
    // Συνήθως κρατάμε ceil((N+1)/2) στο αριστερό και τα υπόλοιπα στο δεξί
    int split_point = (total_records + 1) / 2;

    // 4. Επαναφορά των μετρητών στους κόμβους
    old_node->count = 0;
    datanode_init(new_node, old_node->next_block_id); // Ο νέος δείχνει εκεί που έδειχνε ο παλιός
    
    // 5. Μοίρασμα εγγραφών
    // Γέμισμα Παλιού Κόμβου (Αριστερό μέρος)
    for (int i = 0; i < split_point; i++) {
        old_node->records[i] = temp_records[i];
        old_node->count++;
    }

    // Γέμισμα Νέου Κόμβου (Δεξί μέρος)
    for (int i = split_point; i < total_records; i++) {
        // Στον νέο κόμβο οι δείκτες ξεκινάνε από το 0
        new_node->records[i - split_point] = temp_records[i];
        new_node->count++;
    }

    // 6. Ενημέρωση της Λίστας (Linked List Update)
    // Ο παλιός κόμβος τώρα δείχνει στον νέο
    old_node->next_block_id = new_block_id;

    // 7. Επιστροφή Pivot Key
    // Στα B+ Trees (στα φύλλα), το κλειδί που ανεβαίνει στον γονέα είναι
    // το μικρότερο κλειδί του ΝΕΟΥ (δεξιού) κόμβου.
    *pivot_key = record_get_key(schema, &new_node->records[0]);

    return 0;
}