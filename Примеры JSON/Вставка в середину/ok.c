#include <stdlib.h>
typedef struct Node { struct Node *left, *right; } Node;

int main() {
    Node *root = malloc(sizeof(Node));
    root->left = malloc(sizeof(Node)); // первый левый узел

    free(root->left); root->left = NULL; // освобождаем перед заменой
    root->left = malloc(sizeof(Node));   // вставляем новый

    free(root->left); root->left = NULL;
    free(root);       root       = NULL;
    return 0;
}
