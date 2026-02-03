#include <stdio.h>
#include <time.h>

#include "bed.h"
#include "hash.h"
#include "heap.h"
#include "queue.h"
#include "predict.h"
#include "json.h"
#include "decision.h"

/* Find first free bed of required type */
Bed* find_free_bed(Bed beds[], int count, int type) {
    for (int i = 0; i < count; i++) {
        if (beds[i].type == type && beds[i].state == BED_FREE)
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

    printf("Enter Severity (1-5): ");
    scanf("%d", &p.severity);

    p.arrival_time = now;
    p.allocated_bed_id = -1;

    // DECISION SUPPORT FOR EMERGENCY
    if (p.severity >= 4) { // ICU or Ventilator
        AdmissionDecision dec = evaluate_emergency(p, beds, bed_count);
        
        if (dec == DECISION_REJECT) {
            printf("\n[DECISION]: REJECT Patient %d. No capacity avilable, no discharge imminent.\n", p.patient_id);
            return;
        }
        if (dec == DECISION_RECOMMEND_TRANSFER) {
            printf("\n[DECISION]: RECOMMEND TRANSFER for Patient %d. No free beds, but discharge approved beds exist.\n", p.patient_id);
            return;
        }
        // If DECISION_ADMIT, proceed to standard allocation
    }

    Bed *bed = find_free_bed(beds, bed_count, p.severity);

    if (bed) {
        bed->state = BED_OCCUPIED;
        bed->admit_time = now;
        bed->current_patient_id = p.patient_id;
        p.allocated_bed_id = bed->bed_id;

        long avg_stay = 60 * (6 - p.severity);
        long discharge = now + avg_stay;

        PredNode node = { bed->bed_id, discharge };
        pred_insert(predict_heap, node);

        printf(" Patient %d allocated Bed %d (Type %d)\n",
               p.patient_id, bed->bed_id, bed->type);
    } else {
        // Only enqueue NON-EMERGENCY patients
        if (p.severity < 4) {
            enqueue(waiting, p);
            printf(" No bed available. Patient %d added to waiting queue.\n",
                   p.patient_id);
        } else {
            // Should not be reached if evaluate_emergency works, but safety net
            printf(" [ERROR] Unexpected state for emergency patient. Rejecting.\n");
        }
    }
}

void discharge_patient(
    Bed beds[],
    int bed_count,
    Queue *waiting,
    MinHeap *predict_heap
) {
    int bed_id;
    printf("Enter Bed ID to manage: ");
    scanf("%d", &bed_id);

    Bed *bed = NULL;
    for (int i = 0; i < bed_count; i++) {
        if (beds[i].bed_id == bed_id) {
            bed = &beds[i];
            break;
        }
    }

    if (!bed) {
        printf("Error: Bed %d not found.\n", bed_id);
        return;
    }

    printf("Current State: ");
    if (bed->state == BED_FREE) printf("FREE\n");
    else if (bed->state == BED_OCCUPIED) printf("OCCUPIED (Patient %d)\n", bed->current_patient_id);
    else if (bed->state == BED_DISCHARGE_APPROVED) printf("DISCHARGE APPROVED (Patient %d)\n", bed->current_patient_id);

    if (bed->state == BED_FREE) {
        printf("Bed is already free.\n");
        return;
    }

    if (bed->state == BED_OCCUPIED) {
        int confirm;
        printf("Action: Approve Discharge? (1=Yes, 0=Cancel): ");
        scanf("%d", &confirm);
        if (confirm == 1) {
            bed->state = BED_DISCHARGE_APPROVED;
            printf("Bed %d marked as DISCHARGE APPROVED.\n", bed_id);
        }
    } 
    else if (bed->state == BED_DISCHARGE_APPROVED) {
        int confirm;
        printf("Action: Finalize Discharge & Clean? (1=Yes, 0=Cancel): ");
        scanf("%d", &confirm);
        if (confirm == 1) {
            printf("Patient %d discharged from Bed %d.\n", bed->current_patient_id, bed_id);
            bed->state = BED_FREE;
            bed->current_patient_id = 0;
            bed->admit_time = 0;

            // Check Waiting Queue
            // Only non-emergency are in queue.
            // Check if anyone in queue needs this bed type.
            // Simple FIFO logic: traverse queue, find first match?
            // Existing logic was simplified dequeue. 
            // Queue only stores `Patient`. Does not index by type.
            // But strict FIFO means we look at HEAD.
            // If HEAD needs this bed type -> Allocate.
            // If HEAD needs different type -> Can we skip? 
            // Standard queue doesn't skip. So usually we only check head.
            // If head cannot fit, we leave bed free? Or do we search queue?
            // "FIFO waiting queue for non-emergency patients".
            // If I stick to strict FIFO, I check queue.data[front].
            
            if (!queue_empty(waiting)) {
                Patient p = waiting->data[waiting->front]; // Peek
                if (p.severity == bed->type) {
                     // Match! Dequeue and allocate.
                     p = dequeue(waiting);
                     bed->state = BED_OCCUPIED;
                     bed->current_patient_id = p.patient_id;
                     bed->admit_time = time(NULL);
                     
                     long avg_stay = 60 * (6 - p.severity);
                     long discharge_time = time(NULL) + avg_stay;
                     PredNode new_node = { bed->bed_id, discharge_time };
                     pred_insert(predict_heap, new_node);

                     printf("✔ Waiting patient %d allocated Bed %d automatically.\n", p.patient_id, bed->bed_id);
                } else {
                    printf("Note: Head of queue (Patient %d, Type %d) does not match Bed Type %d. Bed remains Free.\n", p.patient_id, p.severity, bed->type);
                }
            }
        }
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

    heap_init(&emergency); // Not extensively used in new logic but kept for struct compatibility if needed
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
        printf("2. Show next predicted free bed (INFO)\n");
        printf("3. Export bed status to JSON\n");
        printf("4. Manage Bed State (Discharge Support)\n");
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
                    long remaining = next.discharge_time - time(NULL);
                    if (remaining < 0) remaining = 0;
                    printf("Next predicted free bed: Bed %d in ~%ld minutes.\n", next.bed_id, remaining/60);
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
