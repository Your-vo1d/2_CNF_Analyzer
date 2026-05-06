#include <stdlib.h>
typedef struct Node { struct Node *left, *right; } Node;

int main() {
    Node *root  = malloc(sizeof(Node));
    root->left  = malloc(sizeof(Node));
    root->right = malloc(sizeof(Node));

    free(root->left);  root->left  = NULL;
    free(root->right); root->right = NULL;
    free(root);        root        = NULL;
    return 0;
}
