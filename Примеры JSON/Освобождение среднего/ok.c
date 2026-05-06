#include <stdlib.h>
typedef struct Node { struct Node *left, *right; } Node;

int main() {
    Node *root = malloc(sizeof(Node));
    root->left = malloc(sizeof(Node));       // средний
    root->left->left = malloc(sizeof(Node)); // лист

    // освобождаем снизу вверх
    free(root->left->left); root->left->left = NULL;
    free(root->left);       root->left       = NULL;
    free(root);             root             = NULL;
    return 0;
}
