#include <stdlib.h>
typedef struct Node { struct Node *left, *right; } Node;

// выделяет память через указатель на указатель
void alloc_node(Node **p) {
    *p = malloc(sizeof(Node));
}

int main() {
    Node *root = malloc(sizeof(Node));
    alloc_node(&root->left); // выделяем через функцию

    free(root->left); root->left = NULL;
    free(root);       root       = NULL;
    return 0;
}
