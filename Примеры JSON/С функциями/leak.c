#include <stdlib.h>
typedef struct Node { struct Node *left, *right; } Node;

void alloc_node(Node **p) {
    *p = malloc(sizeof(Node));
}

int main() {
    Node *root = malloc(sizeof(Node));
    alloc_node(&root->left);

    // root->left не освобождён перед удалением корня
    free(root); root = NULL;
    return 0;
}
