//
// Created by theofilos on 11/4/25.
//

#ifndef BPLUS_BPLUS_FILE_STRUCTS_H
#define BPLUS_BPLUS_FILE_STRUCTS_H
#include "bf.h"
#include "bplus_datanode.h"
#include "bplus_index_node.h"
#include "record.h"
#include "bplus_file_structs.h"

typedef struct {
    // 1. Αναγνώριση αρχείου
    int magic_number;       

    // 2. Πληροφορίες Δομής Δέντρου
    int root_block_id;      // Το Block ID της ρίζας
    int height;             // Το ύψος του δέντρου (1 = μόνο ρίζα-φύλλο)
    int next_block_id;      // Ο επόμενος διαθέσιμος αριθμός block (για απλή διαχείριση χώρου)

    // 3. Προ-υπολογισμένες τιμές χωρητικότητας (Απαντά στα ερωτήματα μεγέθους)
    int max_keys_index;     // Μέγιστο πλήθος κλειδιών σε κόμβο ευρετηρίου
    int max_records_data;   // Μέγιστο πλήθος εγγραφών σε κόμβο δεδομένων

    // 4. Σχήμα Πίνακα (Απαντά στο ερώτημα μεγέθους εγγραφής)
    TableSchema schema;
} BPlusMeta;

#endif //BPLUS_BPLUS_FILE_STRUCTS_H

