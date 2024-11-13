#include <stdio.h>
#include <stdlib.h>  // for abs function

int find_platform(int n, int *heights, int *rope_lengths) {
    for (int i = n - 1; i >= 0; i--) {
        if (rope_lengths[n - 1 - i] >= abs(heights[i])) {
            //return i + 2;
            return n - 1;
        }
    }
    return 0;
}

int main() {
    int n, m;
    printf("Please enter the number of platforms and the number of ropes carried ：\n");
    scanf("%d %d", &n, &m);
    
    int heights[n];
    printf("Please enter the height of each platform：\n");
    for (int i = 0; i < n; i++) {
        scanf("%d", &heights[i]);
        printf("heights[i] = %d\n",heights[i]);
    }

    int rope_lengths[m];
    printf("Please enter the length of ropes you are carrying separated by spaces：\n");
    for (int i = 0; i < m; i++) {
        scanf("%d", &rope_lengths[i]);
        printf("rope_lengths[i] = %d\n",rope_lengths[i]);
    }
    
    int result = find_platform(n, heights, rope_lengths);
    printf("The highest platform that can be reached is the %d platform。\n", result);
    return 0;
}
