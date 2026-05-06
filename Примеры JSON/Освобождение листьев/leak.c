#include <stdlib.h>
typedef struct Node { struct Node *left, *right; } Node;

int main() {
    Node *root  = malloc(sizeof(Node));
    root->left  = malloc(sizeof(Node));
    root->right = malloc(sizeof(Node));

    free(root->left); root->left = NULL;
    // правый лист забыли — обнуляем без free
    root->right = NULL;
    free(root); root = NULL;
    return 0;
}
