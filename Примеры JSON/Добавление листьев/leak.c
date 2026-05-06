#include <stdlib.h>
typedef struct Node { struct Node *left, *right; } Node;

int main() {
    Node *root  = malloc(sizeof(Node));
    root->left  = malloc(sizeof(Node));
    root->right = malloc(sizeof(Node));

    // освободили только корень, листья потеряны
    free(root); root = NULL;
    return 0;
}
