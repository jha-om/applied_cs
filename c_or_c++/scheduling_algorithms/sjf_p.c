#include <stdio.h>

struct Process
{
    int id, at, bt, ct, rt, tat, wt;
};

int main() {
    int n, current_time = 0, complete = 0;
    printf("Enter the number of process: ");
    scanf("%d", &n);

    struct Process p[n];

    for (int i = 0; i < n;i++) {
        p[i].id = i + 1;
        printf("Enter arrival time and burst time for P[%d]: ", p[i].id);
        scanf("%d %d", &p[i].at, &p[i].bt);
        p[i].rt = p[i].bt;
    }
    while(complete < n) {
        int ind = -1;
        for (int i = 0; i < n;i++) {
            if(p[i].at <= current_time && p[i].rt > 0 && (ind == -1 || p[i].rt < p[ind].rt)) {
                ind = i;
            }
        }
        if(ind != -1) {
            p[ind].rt--;
            current_time++;
            if(p[ind].rt == 0) {
                // process is complete and is in now for termination state;
                p[ind].ct = current_time;
                p[ind].tat = current_time - p[ind].at;
                p[ind].wt = p[ind].tat - p[ind].bt;
                complete++;
            }
        } else {
            current_time++;
        }
    }

    int swt = 0, stat = 0;
    printf("\nProcess\tAT\tBT\tCT\tTAT\tWT\n");
    for (int i = 0; i < n;i++) {
        swt += p[i].wt;
        stat += p[i].tat;
        printf("P%d\t%d\t%d\t%d\t%d\t%d\n", p[i].id, p[i].at, p[i].bt, p[i].ct, p[i].tat, p[i].wt);
    }

    printf("\n Average Waiting time: %.2f", swt * 1.0 / n);
    printf("\n Average Turn Around time: %.2f", stat * 1.0 / n);

    return 0;
}
