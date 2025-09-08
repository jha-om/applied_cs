#include <stdio.h>

int main() {

    int time = 0, sum_bt = 0, smallest, n;
    int swt = 0, stat = 0;
    printf("Enter the number of processes: ");
    scanf("%d", &n);
    int bt[n+1], at[n+1];

    for (int i = 0; i < n;++i) {
        printf("the arrival time for process P%d : ", i + 1);
        scanf("%d", &at[i]);
        
        printf("the burst time for process P%d : ", i + 1);
        scanf("%d", &bt[i]);

        sum_bt += bt[i];
    }

    bt[n+1] = 999;
    for (int t = 0; t < sum_bt;) {
        smallest = n+1;
        for (int i = 0; i < n;i++) {
            if(at[i] <= t && bt[i] > 0 && bt[i] < bt[smallest]) {
                smallest = i;
            }
        }
        printf("\n\nP[%d]\t|\t%d\t|\t%d\n", smallest + 1, t + bt[smallest] - at[smallest], t - at[smallest]);
        stat += t + bt[smallest] - at[smallest];
        printf("\n\n turnaround time for process : P[%d] = %d", smallest+1, stat);
        swt += t - at[smallest];
        t += bt[smallest];
        bt[smallest] = 0;
    }
    printf("\n\n waiting time = %d", swt);
    printf("\n\n turnaround time = %d", stat);
    printf("\n\n average waiting time = %f", swt * 1.0 / n);
    printf("\n\n average turnaround time = %f", stat * 1.0 / n);
    
    return 0;
}