#include <stdlib.h>
typedef struct Node { struct Node *left, *right; } Node;

int main() {
    Node *root = malloc(sizeof(Node));
    free(root); root = NULL;
    return 0;
}
