#include <stdlib.h>
typedef struct Node { struct Node *left, *right; } Node;

int main() {
    Node *root  = malloc(sizeof(Node)); // корень дерева
    root->left  = malloc(sizeof(Node)); // левый лист
    root->right = malloc(sizeof(Node)); // правый лист

    free(root->left);  root->left  = NULL; // чистим листья
    free(root->right); root->right = NULL;
    free(root);        root        = NULL; // и сам корень
    return 0;
}
