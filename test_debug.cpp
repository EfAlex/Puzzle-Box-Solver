#include <cstdio>
#include <cstdint>
#include <cstring>

static const int BOX_W = 4, BOX_H = 3, BOX_D = 2;
static const uint8_t directionSwapZ[16] = {
    0, 1, 4, 5, 2, 3, 13, 12, 11, 10, 9, 8, 7, 6
};

struct BitBox {
    uint8_t mask[12];
    BitBox() { std::memset(mask, 0, sizeof(mask)); }
    
    inline uint8_t cellState(int x, int y, int z) const {
        int i = x + BOX_W*y + BOX_W*BOX_H*z;
        return (mask[i>>1] >> ((i&1)<<2)) & 0xf;
    }
    inline void setCellState(int x, int y, int z, uint8_t s) {
        int i = x + BOX_W*y + BOX_W*BOX_H*z;
        mask[i>>1] &= (uint8_t)~(0xf << ((i&1)<<2));
        mask[i>>1] |= (uint8_t)(s << ((i&1)<<2));
    }
    
    void rotateZ_bm() {
        printf("  rotateZ_bm: BOX_H=%d, BOX_W=%d\n", BOX_H, BOX_W);
        printf("  Iteration bounds: y < %d, x < %d\n", (BOX_H + 1) / 2, (BOX_W + 1) / 2);
        for (int z = 0; z < BOX_D; z++)
            for (int y = 0; y < (BOX_H + 1) / 2; y++)
                for (int x = 0; x < (BOX_W + 1) / 2; x++) {
                    int x2 = BOX_W - 1 - x;
                    int y2 = BOX_H - 1 - y;
                    uint8_t code1 = cellState(x, y, z);
                    uint8_t code2 = cellState(x2, y2, z);
                    printf("  pair (%d,%d,%d)->(%d,%d,%d): code1=%d code2=%d\n",
                           x, y, z, x2, y2, z, code1, code2);
                    if (code1 >= 2 && code1 <= 13) {
                        printf("    -> writing directionSwapZ[%d]=%d to (%d,%d,%d)\n",
                               code1, directionSwapZ[code1], x2, y2, z);
                        setCellState(x2, y2, z, directionSwapZ[code1]);
                    } else {
                        printf("    -> writing code1=%d to (%d,%d,%d)\n", code1, x2, y2, z);
                        setCellState(x2, y2, z, code1);
                    }
                    if (code2 >= 2 && code2 <= 13) {
                        printf("    -> writing directionSwapZ[%d]=%d to (%d,%d,%d)\n",
                               code2, directionSwapZ[code2], x, y, z);
                        setCellState(x, y, z, directionSwapZ[code2]);
                    } else {
                        printf("    -> writing code2=%d to (%d,%d,%d)\n", code2, x, y, z);
                        setCellState(x, y, z, code2);
                    }
                }
    }
    
    void dump() const {
        for (int z = 0; z < BOX_D; z++) {
            printf("    z=%d:", z);
            for (int y = 0; y < BOX_H; y++)
                for (int x = 0; x < BOX_W; x++) {
                    uint8_t c = cellState(x,y,z);
                    if(c==0) printf(" .");
                    else if(c==1) printf(" X");
                    else printf(" +%d%d%d", 
                        (c>>1)&1, (c>>3)&1, c&1);
                }
            printf("\n");
        }
    }
};

int main() {
    printf("=== Testing rotateZ_bm with half-cell at (0,1,0) code 2 ===\n");
    BitBox bb;
    bb.setCellState(0, 1, 0, 2);  // code 2 = (+1,0,+1)
    printf("  Before:\n");
    bb.dump();
    bb.rotateZ_bm();
    printf("  After:\n");
    bb.dump();
    printf("  cellState(3,1,0) = %d (expected 4)\n", bb.cellState(3,1,0));
    printf("  cellState(0,1,0) = %d (expected 0)\n", bb.cellState(0,1,0));
    return 0;
}
