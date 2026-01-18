#include <stdio.h>
#include <time.h>

#include "bed.h"
#include "hash.h"
#include "heap.h"
#include "queue.h"
#include "predict.h"
#include "json.h"

/* Find first free bed of required type */
Bed* find_free_bed(Bed beds[], int count, int type) {
    for (int i = 0; i < count; i++) {
        if (beds[i].type == type && beds[i].occupied == 0)
            return &beds[i];
    }
    return NULL;
}

/* Handle patient admission */
void admit_patient(
    Bed beds[],
    int bed_count,
    MaxHeap *emergency,
    Queue *waiting,
    MinHeap *predict_heap
) {
    Patient p;
    time_t now = time(NULL);

    printf("\nEnter Patient ID: ");
    scanf("%d", &p.patient_id);

    printf("Enter Severity (1–5): ");
    scanf("%d", &p.severity);

    p.arrival_time = now;
    p.allocated_bed_id = -1;

    Bed *bed = find_free_bed(beds, bed_count, p.severity);

    if (bed) {
        bed->occupied = 1;
        bed->admit_time = now;
        p.allocated_bed_id = bed->bed_id;

        long avg_stay = 60 * (6 - p.severity);
        long discharge = now + avg_stay;

        PredNode node = { bed->bed_id, discharge };
        pred_insert(predict_heap, node);

        printf(" Patient %d allocated Bed %d (Type %d)\n",
               p.patient_id, bed->bed_id, bed->type);
    } else {
        enqueue(waiting, p);
        printf(" No bed available. Patient %d added to waiting queue.\n",
               p.patient_id);
    }
}

void discharge_patient(
    Bed beds[],
    int bed_count,
    Queue *waiting,
    MinHeap *predict_heap
) {
    if (pred_empty(predict_heap)) {
        printf("No patients to discharge.\n");
        return;
    }

    // Extract earliest predicted discharge
    PredNode node = pred_extract_min(predict_heap);
    int bed_id = node.bed_id;

    // Find the bed
    Bed *bed = NULL;
    for (int i = 0; i < bed_count; i++) {
        if (beds[i].bed_id == bed_id) {
            bed = &beds[i];
            break;
        }
    }

    if (!bed) {
        printf("Error: Bed not found.\n");
        return;
    }

    // Free the bed
    bed->occupied = 0;
    bed->admit_time = 0;

    printf("✔ Bed %d discharged.\n", bed_id);

    // Allocate to waiting patient if any
    if (!queue_empty(waiting)) {
        Patient p = dequeue(waiting);

        bed->occupied = 1;
        bed->admit_time = time(NULL);
        p.allocated_bed_id = bed->bed_id;

        long avg_stay = 60 * (6 - p.severity);
        long discharge_time = time(NULL) + avg_stay;

        PredNode new_node = { bed->bed_id, discharge_time };
        pred_insert(predict_heap, new_node);

        printf("✔ Waiting patient %d allocated Bed %d.\n",
               p.patient_id, bed->bed_id);
    }
}

void display_waiting_queue(Queue *q) {
    if (queue_empty(q)) {
        printf("Waiting queue is empty.\n");
        return;
    }

    printf("\n--- Waiting Queue (FIFO Order) ---\n");

    int idx = q->front;

    for (int c = 0; c < q->count; c++) {
        Patient p = q->data[idx];
        printf("Patient ID: %d | Severity: %d\n",
               p.patient_id, p.severity);

        idx = (idx + 1) % MAX_WAIT;
    }
}



int main() {
    Bed beds[MAX_BEDS];
    int bed_count;

    MaxHeap emergency;
    Queue waiting;
    MinHeap predict_heap;

    heap_init(&emergency);
    queue_init(&waiting);
    pred_init(&predict_heap);

    init_beds(beds, &bed_count);
    hash_init();

    for (int i = 0; i < bed_count; i++)
        hash_insert(beds[i].bed_id, &beds[i]);

    printf("Hospital initialized with %d beds.\n", bed_count);

    int choice;

    while (1) {
        printf("\n===== Hospital Bed Allocation Menu =====\n");
        printf("1. Admit new patient\n");
        printf("2. Show next predicted free bed\n");
        printf("3. Export bed status to JSON\n");
        printf("4. Discharge next patient\n");
        printf("5. Show waiting queue\n");
        printf("6. Exit\n");
        printf("Enter choice: ");



        scanf("%d", &choice);

        switch (choice) {
            case 1:
                admit_patient(beds, bed_count, &emergency, &waiting, &predict_heap);
                break;

            case 2:
                if (!pred_empty(&predict_heap)) {
                    PredNode next = pred_peek(&predict_heap);
                    printf("Next expected free bed: Bed %d\n", next.bed_id);
                } else {
                    printf("No discharge predictions available.\n");
                }
                break;

            case 3:
                export_beds_to_json(beds, bed_count, "data/beds.json");
                break;

            case 4:
                discharge_patient(beds, bed_count, &waiting, &predict_heap);
                break;

            case 5:
                display_waiting_queue(&waiting);
                break;

            case 6:
                printf("Exiting system.\n");
                return 0;



            default:
                printf("Invalid choice. Try again.\n");
        }
    }
}
