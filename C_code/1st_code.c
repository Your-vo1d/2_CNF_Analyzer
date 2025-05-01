#include <iostream>
#include <queue> // для обхода в ширину (BFS)

// Структура узла дерева
struct TreeNode {
    int data;
    TreeNode* left;
    TreeNode* right;
};

// Рекурсивный поиск элемента в дереве
bool search(TreeNode* node, int value) {
    if (!node) return false;
    if (node->data == value) return true;
    return search(node->left, value) || search(node->right, value);
}

// Подсчёт количества узлов (итеративный, BFS)
int countNodes(TreeNode* root) {
    if (!root) return 0;
    std::queue<TreeNode*> q;
    q.push(root);
    int count = 0;
    while (!q.empty()) {
        TreeNode* current = q.front();
        q.pop();
        count++;
        if (current->left) q.push(current->left);
        if (current->right) q.push(current->right);
    }
    return count;
}

// Проверка, является ли дерево BST (бинарным деревом поиска)
bool isBSTHelper(TreeNode* node, int min, int max) {
    if (!node) return true;
    if (node->data <= min || node->data >= max) return false;
    return isBSTHelper(node->left, min, node->data) && 
           isBSTHelper(node->right, node->data, max);
}

bool isBST(TreeNode* root) {
    return isBSTHelper(root, INT_MIN, INT_MAX);
}

// Освобождение памяти (рекурсивное удаление узлов)
void freeTree(TreeNode* node) {
    if (!node) return;
    freeTree(node->left);
    freeTree(node->right);
    free(node);
}

int main() {
    // Создание корня дерева
    TreeNode* root = NULL;
    root = (TreeNode*)malloc(sizeof(TreeNode));
    root->data = 10;
    root->left = NULL;
    root->right = NULL;

    // Добавляем левого потомка
    root->left = (TreeNode*)malloc(sizeof(TreeNode));
    root->left->data = 5;
    root->left->left = NULL;
    root->left->right = NULL;

    // Добавляем правого потомка
    root->right = (TreeNode*)malloc(sizeof(TreeNode));
    root->right->data = 15;
    root->right->left = root->left;
    root->right->right = root-> right;

    // === Примеры работы с деревом ===

    // Условие: если дерево BST, выводим сообщение
    if (root) {
        std::cout << "Дерево существует\n";
    } else {
        std::cout << "Дерево не существует\n";
    }

    // Цикл: обход дерева в ширину (BFS) и вывод значений
    
    std::cout << "Самая левая ветка: ";
    TreeNode* current = root;
    while (current) {
        std::cout << current->data << " ";
        // Если нет левого, но есть правый - идем в правый
        current = current->left;
    }
    std::cout << "\n";

    // Подсчёт узлов
    std::cout << "Количество узлов в дереве: " << countNodes(root) << "\n";

    // Освобождаем память (можно раскомментировать)
    // freeTree(root);

    return 0;
}