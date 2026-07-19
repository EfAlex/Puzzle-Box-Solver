#include <cstdio>
#include <cstdint>

static const int BOX_W = 4, BOX_H = 3, BOX_D = 2;

int main() {
    printf("=== rotateX pairs with z < (BOX_D+1)/2 = %d ===\n", (BOX_D+1)/2);
    for (int z = 0; z < (BOX_D+1)/2; z++)
        for (int y = 0; y < (BOX_H+1)/2; y++)
            printf("  pair: (%d,%d,%d) <-> (%d,%d,%d)\n",
                   0, y, z, 0, BOX_H-1-y, BOX_D-1-z);
    
    printf("\n=== rotateX pairs with z < BOX_D/2 = %d ===\n", BOX_D/2);
    for (int z = 0; z < BOX_D/2; z++)
        for (int y = 0; y < (BOX_H+1)/2; y++)
            printf("  pair: (%d,%d,%d) <-> (%d,%d,%d)\n",
                   0, y, z, 0, BOX_H-1-y, BOX_D-1-z);
    
    printf("\n=== All unique pairs for rotateX ===\n");
    // For rotateX: (y,z) <-> (BOX_H-1-y, BOX_D-1-z)
    // Unique pairs: iterate all y in [0,BOX_H), all z in [0,BOX_D)
    // but only process if (y,z) is the "first" in the pair
    for (int y = 0; y < BOX_H; y++)
        for (int z = 0; z < BOX_D; z++) {
            int y2 = BOX_H - 1 - y;
            int z2 = BOX_D - 1 - z;
            // Process only if this is the "first" in the pair
            if (y < y2 || (y == y2 && z < z2)) {
                printf("  pair: (%d,%d,%d) <-> (%d,%d,%d)\n",
                       0, y, z, 0, y2, z2);
            }
        }
    
    return 0;
}
