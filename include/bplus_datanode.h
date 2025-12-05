
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
/**
 * @brief Υπολογίζει το μέγιστο πλήθος εγγραφών που χωράνε σε ένα Data Node.
 * @param record_size Το μέγεθος της εγγραφής (από το schema).
 * @return Ο μέγιστος αριθμός εγγραφών.
 */
int calculate_max_data_records(int record_size);

/**
 * @brief Αρχικοποιεί έναν κόμβο δεδομένων.
 * @param node Δείκτης στον κόμβο.
 * @param next_block_id Το ID του επόμενου μπλοκ στη λίστα (-1 αν δεν υπάρχει).
 */
void datanode_init(DataNode* node, int next_block_id);

/**
 * @brief Εισάγει μια εγγραφή στον κόμβο διατηρώντας την ταξινόμηση.
 * @param node Δείκτης στον κόμβο.
 * @param max_records Το μέγιστο πλήθος εγγραφών που χωράει ο κόμβος.
 * @param record Η εγγραφή προς εισαγωγή.
 * @param schema Το σχήμα του πίνακα.
 * @return 0 σε επιτυχία, -1 αν ο κόμβος είναι γεμάτος, -2 αν υπάρχει διπλότυπο.
 */
int datanode_insert_record(DataNode* node, int max_records, const Record* record, const TableSchema* schema);

/**
 * @brief Διασπά έναν γεμάτο κόμβο δεδομένων σε δύο.
 * @param old_node Ο γεμάτος κόμβος.
 * @param new_node Ο νέος (άδειος) κόμβος.
 * @param new_block_id Το Block ID του νέου κόμβου.
 * @param new_record Η εγγραφή που προκάλεσε τη διάσπαση.
 * @param max_records Το μέγιστο πλήθος εγγραφών.
 * @param schema Το σχήμα του πίνακα.
 * @param pivot_key Δείκτης για επιστροφή του κλειδιού-διαχωριστή (για τον γονέα).
 * @return 0 σε επιτυχία, -1 σε σφάλμα.
 */
int datanode_split(DataNode* old_node, DataNode* new_node, int new_block_id, 
                   const Record* new_record, int max_records, 
                   const TableSchema* schema, int* pivot_key);


/**
 * @brief Αναζητά μια εγγραφή με βάση το κλειδί.
 * @param node Δείκτης στον κόμβο.
 * @param key Το κλειδί αναζήτησης.
 * @param schema Το σχήμα του πίνακα.
 * @param out_record Δείκτης για να επιστραφεί η εγγραφή (αν βρεθεί).
 * @return 0 αν βρέθηκε, -1 αν δεν βρέθηκε.
 */
int datanode_find_record(const DataNode* node, int key, const TableSchema* schema, Record** out_record);

/**
 * @brief Εκτυπώνει τα περιεχόμενα του κόμβου δεδομένων (για debugging).
 * @param node Δείκτης στον κόμβο.
 * @param schema Το σχήμα του πίνακα.
 */
void datanode_print(const DataNode* node, const TableSchema* schema);

#endif