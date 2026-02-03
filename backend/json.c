#include <stdio.h>
#include "json.h"

void export_beds_to_json(Bed beds[], int count, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: Cannot create JSON file.\n");
        return;
    }

    fprintf(fp, "{\n  \"beds\": [\n");

    for (int i = 0; i < count; i++) {
        fprintf(fp,
            "    { \"id\": %d, \"type\": %d, \"state\": %d, \"patient_id\": %d }%s\n",
            beds[i].bed_id,
            beds[i].type,
            beds[i].state, // 0=Free, 1=Occupied, 2=DischargeApproved
            beds[i].current_patient_id,
            (i < count - 1) ? "," : ""
        );
    }

    fprintf(fp, "  ]\n}\n");
    fclose(fp);

    printf("\nJSON exported successfully to %s\n", filename);
}
