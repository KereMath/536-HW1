/**
 * Red-Black Tree Implementation in C
 *
 * This implementation adheres to the following constraints:
 * 1. Nodes are stored in a global array `GLB`.
 * 2. "Pointers" (parent, left, right) are integer indices into `GLB`.
 * 3. A special index `NIL` (defined as -1) represents a null pointer.
 * 4. Node data is accessed via `GLB[index].key` and `GLB[index].color`.
 *
 * This implementation includes `search`, `insert`, and `delete`,
 * and maintains a global `minimum_key` index.
 *
 * Author: Gemini
 */

#include <stdio.h>
#include <stdlib.h> // For exit()
#include <math.h>   // For fabs()
#include <float.h>  // For DBL_EPSILON

// --- Constants ---

// Define our "null pointer" index
#define NIL -1

// Define color constants
#define RED 0
#define BLACK 1

// Define the maximum number of nodes the global array can hold.
#define MAX_NODES 1024

// --- Data Structures ---

/**
 * @brief Defines the structure of a node in the tree.
 * This structure is stored in the `GLB` array.
 */
struct Node {
    double key;   // The key value for the node
    int color;    // RED or BLACK
    int parent;   // Index of the parent node (NIL for root)
    int left;     // Index of the left child (NIL for empty)
    int right;    // Index of the right child (NIL for empty)
};

// --- Global Array & Variables ---

/**
 * @brief The global array for storing all tree nodes.
 */
struct Node GLB[MAX_NODES];

/**
 * @brief Global index of the node with the minimum key in the tree.
 * Initialized to NIL (-1).
 */
int minimum_key = NIL;

// --- Helper Functions ---

/**
 * @brief Safely compares two double-precision floating-point numbers.
 * @param a The first double.
 * @param b The second double.
 * @return 1 if they are "equal" within a small epsilon, 0 otherwise.
 */
int double_equals(double a, double b) {
    return fabs(a - b) < DBL_EPSILON;
}

/**
 * @brief Finds the node with the minimum key in the subtree rooted at `x`.
 * @param x The index of the subtree root.
 * @return The index of the minimum node.
 */
int tree_minimum(int x) {
    if (x == NIL) return NIL;
    while (GLB[x].left != NIL) {
        x = GLB[x].left;
    }
    return x;
}

/**
 * @brief Performs a left rotation on the node at index `x`.
 * Updates the main `root` pointer if the root itself is rotated.
 *
 * @param root_ptr A pointer to the main root index of the tree.
 * @param x The index of the node to rotate around.
 */
void left_rotate(int* root_ptr, int x) {
    int y = GLB[x].right; // set y
    if (y == NIL) return; // Cannot rotate

    // Turn y's left subtree into x's right subtree
    GLB[x].right = GLB[y].left;
    if (GLB[y].left != NIL) {
        GLB[GLB[y].left].parent = x;
    }

    // Link x's parent to y
    GLB[y].parent = GLB[x].parent;
    if (GLB[x].parent == NIL) {
        *root_ptr = y; // y is the new root
    } else if (x == GLB[GLB[x].parent].left) {
        GLB[GLB[x].parent].left = y;
    } else {
        GLB[GLB[x].parent].right = y;
    }

    // Put x on y's left
    GLB[y].left = x;
    GLB[x].parent = y;
}

/**
 * @brief Performs a right rotation on the node at index `y`.
 * Updates the main `root` pointer if the root itself is rotated.
 *
 * @param root_ptr A pointer to the main root index of the tree.
 * @param y The index of the node to rotate around.
 */
void right_rotate(int* root_ptr, int y) {
    int x = GLB[y].left; // set x
    if (x == NIL) return; // Cannot rotate

    // Turn x's right subtree into y's left subtree
    GLB[y].left = GLB[x].right;
    if (GLB[x].right != NIL) {
        GLB[GLB[x].right].parent = y;
    }

    // Link y's parent to x
    GLB[x].parent = GLB[y].parent;
    if (GLB[y].parent == NIL) {
        *root_ptr = x; // x is the new root
    } else if (y == GLB[GLB[y].parent].right) {
        GLB[GLB[y].parent].right = x;
    } else {
        GLB[GLB[y].parent].left = x;
    }

    // Put y on x's right
    GLB[x].right = y;
    GLB[y].parent = x;
}

/**
 * @brief Replaces the subtree rooted at `u` with the subtree rooted at `v`.
 * This is a helper for the delete operation.
 *
 * @param root_ptr A pointer to the main root index.
 * @param u The index of the node to be replaced.
 * @param v The index of the node to replace `u` with (can be NIL).
 */
void transplant(int* root_ptr, int u, int v) {
    if (GLB[u].parent == NIL) {
        *root_ptr = v;
    } else if (u == GLB[GLB[u].parent].left) {
        GLB[GLB[u].parent].left = v;
    } else {
        GLB[GLB[u].parent].right = v;
    }
    if (v != NIL) {
        GLB[v].parent = GLB[u].parent;
    }
}

/**
 * @brief Fixes Red-Black Tree violations after a standard insertion.
 *
 * @param root_ptr A pointer to the main root index of the tree.
 * @param z The index of the newly inserted node.
 */
void insert_fixup(int* root_ptr, int z) {
    int y; // Uncle node

    // Loop as long as the parent is RED (violating property 4)
    // Note: If parent is NIL, parent->color is BLACK (implicitly), so loop
    // correctly requires parent to be non-NIL.
    while (GLB[z].parent != NIL && GLB[GLB[z].parent].color == RED) {
        
        // Case A: Parent is the LEFT child of the grandparent
        if (GLB[z].parent == GLB[GLB[GLB[z].parent].parent].left) {
            y = GLB[GLB[GLB[z].parent].parent].right; // Uncle

            // Case 1: Uncle is RED
            if (y != NIL && GLB[y].color == RED) {
                GLB[GLB[z].parent].color = BLACK;
                GLB[y].color = BLACK;
                GLB[GLB[GLB[z].parent].parent].color = RED;
                z = GLB[GLB[z].parent].parent; // Move z up to grandparent
            } else {
                // Case 2: Uncle is BLACK, and z is a RIGHT child ("triangle")
                if (z == GLB[GLB[z].parent].right) {
                    z = GLB[z].parent;
                    left_rotate(root_ptr, z);
                }
                
                // Case 3: Uncle is BLACK, and z is a LEFT child ("line")
                GLB[GLB[z].parent].color = BLACK;
                GLB[GLB[GLB[z].parent].parent].color = RED;
                right_rotate(root_ptr, GLB[GLB[z].parent].parent);
            }
        } 
        // Case B: Parent is the RIGHT child of the grandparent (symmetric)
        else {
            y = GLB[GLB[GLB[z].parent].parent].left; // Uncle

            // Case 1: Uncle is RED
            if (y != NIL && GLB[y].color == RED) {
                GLB[GLB[z].parent].color = BLACK;
                GLB[y].color = BLACK;
                GLB[GLB[GLB[z].parent].parent].color = RED;
                z = GLB[GLB[z].parent].parent;
            } else {
                // Case 2: Uncle is BLACK, and z is a LEFT child ("triangle")
                if (z == GLB[GLB[z].parent].left) {
                    z = GLB[z].parent;
                    right_rotate(root_ptr, z);
                }
                
                // Case 3: Uncle is BLACK, and z is a RIGHT child ("line")
                GLB[GLB[z].parent].color = BLACK;
                GLB[GLB[GLB[z].parent].parent].color = RED;
                left_rotate(root_ptr, GLB[GLB[z].parent].parent);
            }
        }
    }

    // Property 2: The root must be BLACK.
    GLB[*root_ptr].color = BLACK;
}

/**
 * @brief Fixes Red-Black Tree violations after a deletion.
 *
 * @param root_ptr A pointer to the main root index.
 * @param x The node that has the "extra black" (can be NIL).
 * @param x_parent The parent of `x` (needed if `x` is NIL).
 */
void delete_fixup(int* root_ptr, int x, int x_parent) {
    int w; // Sibling node

    while (x != *root_ptr && (x == NIL || GLB[x].color == BLACK)) {
        // Determine parent
        int current_parent = (x == NIL) ? x_parent : GLB[x].parent;

        // Case A: x is a LEFT child
        if (x == GLB[current_parent].left) {
            w = GLB[current_parent].right; // Sibling

            // Case 1: Sibling w is RED
            if (w != NIL && GLB[w].color == RED) {
                GLB[w].color = BLACK;
                GLB[current_parent].color = RED;
                left_rotate(root_ptr, current_parent);
                w = GLB[current_parent].right; // New sibling (must be BLACK)
            }

            // w is now guaranteed to be BLACK (or NIL)
            // Case 2: Sibling w is BLACK, and both of w's children are BLACK
            if ((w == NIL) ||
                ((GLB[w].left == NIL || GLB[GLB[w].left].color == BLACK) &&
                 (GLB[w].right == NIL || GLB[GLB[w].right].color == BLACK)))
            {
                if (w != NIL) GLB[w].color = RED;
                x = current_parent; // Move "extra black" up the tree
                // Update x_parent for next iteration (if x becomes NIL)
                x_parent = (x == NIL) ? NIL : GLB[x].parent;
            } else {
                // Case 3: Sibling w is BLACK, w's left child is RED, right is BLACK
                if (w != NIL && (GLB[w].right == NIL || GLB[GLB[w].right].color == BLACK)) {
                    if (GLB[w].left != NIL) GLB[GLB[w].left].color = BLACK;
                    GLB[w].color = RED;
                    right_rotate(root_ptr, w);
                    w = GLB[current_parent].right;
                }

                // Case 4: Sibling w is BLACK, w's right child is RED
                if (w != NIL) {
                    GLB[w].color = GLB[current_parent].color;
                    if (GLB[w].right != NIL) GLB[GLB[w].right].color = BLACK;
                }
                GLB[current_parent].color = BLACK;
                left_rotate(root_ptr, current_parent);
                x = *root_ptr; // Fixup is complete
            }
        }
        // Case B: x is a RIGHT child (symmetric to Case A)
        else {
            w = GLB[current_parent].left; // Sibling

            // Case 1: Sibling w is RED
            if (w != NIL && GLB[w].color == RED) {
                GLB[w].color = BLACK;
                GLB[current_parent].color = RED;
                right_rotate(root_ptr, current_parent);
                w = GLB[current_parent].left; // New sibling (must be BLACK)
            }

            // w is now guaranteed to be BLACK (or NIL)
            // Case 2: Sibling w is BLACK, and both of w's children are BLACK
            if ((w == NIL) ||
                ((GLB[w].left == NIL || GLB[GLB[w].left].color == BLACK) &&
                 (GLB[w].right == NIL || GLB[GLB[w].right].color == BLACK)))
            {
                if (w != NIL) GLB[w].color = RED;
                x = current_parent;
                // Update x_parent for next iteration (if x becomes NIL)
                x_parent = (x == NIL) ? NIL : GLB[x].parent;
            } else {
                // Case 3: Sibling w is BLACK, w's right child is RED, left is BLACK
                if (w != NIL && (GLB[w].left == NIL || GLB[GLB[w].left].color == BLACK)) {
                    if (GLB[w].right != NIL) GLB[GLB[w].right].color = BLACK;
                    GLB[w].color = RED;
                    left_rotate(root_ptr, w);
                    w = GLB[current_parent].left;
                }

                // Case 4: Sibling w is BLACK, w's left child is RED
                if (w != NIL) {
                    GLB[w].color = GLB[current_parent].color;
                    if (GLB[w].left != NIL) GLB[GLB[w].left].color = BLACK;
                }
                GLB[current_parent].color = BLACK;
                right_rotate(root_ptr, current_parent);
                x = *root_ptr; // Fixup is complete
            }
        }
    }

    // Ensure root is BLACK, or if x is RED, make it BLACK
    if (x != NIL) {
        GLB[x].color = BLACK;
    }
}


// --- Requested Functions ---

/**
 * @brief Searches for a key in the R-B tree.
 *
 * @param root The index of the root node of the tree.
 * @param key The key value (double) to search for.
 * @return The index of the node if found, otherwise -1 (NIL).
 */
int search(int root, double key) {
    int current = root;

    while (current != NIL) {
        if (double_equals(key, GLB[current].key)) {
            // Found the key
            return current;
        }

        if (key < GLB[current].key) {
            // Go left
            current = GLB[current].left;
        } else {
            // Go right
            current = GLB[current].right;
        }
    }

    // Not found
    return -1;
}

/**
 * @brief Inserts a node (pre-allocated in GLB) into the R-B tree.
 * The node to be inserted *must* already have its `key` set.
 *
 * @param root_ptr A pointer to the variable holding the root index.
 * This is necessary because rotations might change which
 * node is the root.
 * @param node The index of the node (in GLB) to be inserted.
 * @return 0 on successful insertion.
 */
int insert(int* root_ptr, int node) {
    int y = NIL; // Parent of x
    int x = *root_ptr; // Current node, starting at root
    double key = GLB[node].key;

    // 1. Perform standard Binary Search Tree insertion
    while (x != NIL) {
        y = x; // Remember parent
        
        if (key < GLB[x].key) {
            x = GLB[x].left;
        } else {
            x = GLB[x].right;
        }
    }

    // 2. Link the new node
    GLB[node].parent = y;
    if (y == NIL) {
        // Tree was empty, this node is the new root
        *root_ptr = node;
    } else if (key < GLB[y].key) {
        GLB[y].left = node;
    } else {
        GLB[y].right = node;
    }

    // 3. Initialize the new node's properties
    GLB[node].left = NIL;
    GLB[node].right = NIL;
    GLB[node].color = RED; // New nodes are always RED (initially)

    // 4. Update minimum_key if this is the new minimum
    if (minimum_key == NIL || key < GLB[minimum_key].key) {
        minimum_key = node;
    }

    // 5. Fix any Red-Black property violations
    insert_fixup(root_ptr, node);

    // Successful insertion
    return 0;
}

/**
 * @brief Deletes a node from the R-B tree.
 * Assumes the node index provided is valid and exists in the tree.
 *
 * @param root_ptr A pointer to the variable holding the root index.
 * @param node The index of the node (in GLB) to be deleted.
 * @return 0 on successful deletion.
 */
int delete(int* root_ptr, int node) {
    if (node == NIL) return -1; // Cannot delete NIL

    int z = node; // z is the node to be deleted
    int y = z;    // y is the node that is *actually* spliced out
    int y_original_color = GLB[y].color;
    
    int x;        // x is the child that moves into y's position
    int x_parent; // parent of x (needed for fixup if x is NIL)

    int was_minimum = (node == minimum_key);

    if (GLB[z].left == NIL) {
        // Case 1: z has no left child
        x = GLB[z].right;
        x_parent = GLB[z].parent; // Get parent *before* transplant
        transplant(root_ptr, z, x);
    } else if (GLB[z].right == NIL) {
        // Case 2: z has a left child, but no right child
        x = GLB[z].left;
        x_parent = GLB[z].parent; // Get parent *before* transplant
        transplant(root_ptr, z, x);
    } else {
        // Case 3: z has two children
        y = tree_minimum(GLB[z].right); // y is z's successor
        y_original_color = GLB[y].color;
        x = GLB[y].right; // x is y's right child (can be NIL)

        if (GLB[y].parent == z) {
            // y is z's direct child
            x_parent = y; // x's parent *will be* y (even if x is NIL)
        } else {
            // y is further down the right subtree
            x_parent = GLB[y].parent; // x's parent is y's old parent
            transplant(root_ptr, y, x); // Splice out y
            GLB[y].right = GLB[z].right;  // Attach z's right tree to y
            GLB[GLB[y].right].parent = y;
        }
        
        transplant(root_ptr, z, y); // Replace z with y
        GLB[y].left = GLB[z].left;    // Attach z's left tree to y
        GLB[GLB[y].left].parent = y;
        GLB[y].color = GLB[z].color;  // Give y z's original color

        // If x is not NIL, its parent might be wrong if x_parent was y
        if (x != NIL) {
            GLB[x].parent = x_parent;
        }
    }

    // 4. Fix Red-Black violations if a BLACK node was spliced out
    if (y_original_color == BLACK) {
        delete_fixup(root_ptr, x, x_parent);
    }

    // 5. Update minimum_key if the minimum was deleted
    if (was_minimum) {
        if (*root_ptr == NIL) {
            minimum_key = NIL; // Tree is now empty
        } else {
            minimum_key = tree_minimum(*root_ptr); // Find new minimum
        }
    }

    // Optional: "free" the node `z` (e.g., set key to -1, parent/children to NIL)
    // GLB[z].key = -1.0; 
    // GLB[z].parent = NIL;
    // GLB[z].left = NIL;
    // GLB[z].right = NIL;
    // GLB[z].color = BLACK;

    return 0;
}


// --- Demonstration (main) ---

/**
 * @brief Helper function to print the tree (in-order traversal).
 * @param root The index of the current node.
 */
void print_in_order(int root) {
    if (root == NIL) {
        return;
    }
    
    print_in_order(GLB[root].left);
    
    printf("  Node %d: \tkey = %.2f, \tcolor = %s, \tparent = %d, \tleft = %d, \tright = %d\n",
           root,
           GLB[root].key,
           (GLB[root].color == RED ? "RED" : "BLACK"),
           GLB[root].parent,
           GLB[root].left,
           GLB[root].right);
           
    print_in_order(GLB[root].right);
}

int main() {
    // The main root index of our tree. Starts as NIL (empty tree).
    int root = NIL;
    // Reset minimum key for this run
    minimum_key = NIL;

    // We must "allocate" nodes by simply picking an index in GLB.
    // Let's start from index 1 (index 0 could be used, but 1 is clearer).
    int node_index = 1;

    // Keys to insert
    double keys_to_insert[] = {10.5, 5.2, 15.1, 3.0, 7.8, 12.0, 18.5, 20.0};
    int num_keys = sizeof(keys_to_insert) / sizeof(double);

    printf("--- Inserting nodes ---\n");
    for (int i = 0; i < num_keys; i++) {
        if (node_index >= MAX_NODES) {
            printf("Error: Global array GLB is full.\n");
            break;
        }

        // 1. "Allocate" a node: set its key in the GLB array
        int current_node_index = node_index++;
        GLB[current_node_index].key = keys_to_insert[i];

        // 2. Insert the node
        printf("Inserting key %.2f (at GLB index %d)...\n",
               keys_to_insert[i], current_node_index);
               
        int result = insert(&root, current_node_index);
        
        if (result == -1) {
            printf("  Error: Key %.2f already exists.\n", keys_to_insert[i]);
            // We "wasted" this index, but that's ok for this demo
        } else {
            printf("  Success. New root index is: %d\n", root);
        }
    }

    printf("\n--- Final Tree Structure (in-order) ---\n");
    print_in_order(root);
    if (minimum_key != NIL) {
        printf("\nMinimum key is at index: %d (key: %.2f)\n", minimum_key, GLB[minimum_key].key);
    } else {
        printf("\nTree is empty, no minimum key.\n");
    }

    printf("\n--- Searching for keys ---\n");
    
    // Search for a key that exists
    double key_to_find = 7.8;
    int found_index = search(root, key_to_find);
    if (found_index != -1) {
        printf("Found key %.2f at index %d.\n", key_to_find, found_index);
    } else {
        printf("Key %.2f NOT found.\n", key_to_find);
    }

    // Search for a key that does not exist
    key_to_find = 99.9;
    found_index = search(root, key_to_find);
    if (found_index != -1) {
        printf("Found key %.2f at index %d.\n", key_to_find, found_index);
    } else {
        printf("Key %.2f NOT found.\n", key_to_find);
    }

    // Search for a duplicate
    printf("\n--- Testing duplicate insert ---\n");
    int duplicate_node_index = node_index++;
    GLB[duplicate_node_index].key = 10.5; // This key already exists
    
    printf("Attempting to insert duplicate key 10.5...\n");
    int result = insert(&root, duplicate_node_index);

    printf("\n--- Testing deletion ---\n");

    // Test 1: Delete the minimum (3.0), which is a leaf
    // Node 3.0 is at index 4
    int node_to_delete = search(root, 3.0);
    if (node_to_delete != NIL) {
        printf("Deleting minimum key %.2f (index %d)...\n", GLB[node_to_delete].key, node_to_delete);
        delete(&root, node_to_delete);
        print_in_order(root);
        if (minimum_key != NIL) {
            printf("New minimum key is at index: %d (key: %.2f)\n", minimum_key, GLB[minimum_key].key);
        } else {
            printf("Tree is empty.\n");
        }
    }

    // Test 2: Delete a node with two children (15.1)
    // Node 15.1 is at index 3
    node_to_delete = search(root, 15.1);
     if (node_to_delete != NIL) {
        printf("\nDeleting node with two children %.2f (index %d)...\n", GLB[node_to_delete].key, node_to_delete);
        delete(&root, node_to_delete);
        print_in_order(root);
        printf("New root is: %d\n", root);
    }
    
    // Test 3: Delete the root (now 10.5)
    // Node 10.5 is at index 1
    node_to_delete = search(root, 10.5);
     if (node_to_delete != NIL) {
        printf("\nDeleting root node %.2f (index %d)...\n", GLB[node_to_delete].key, node_to_delete);
        delete(&root, node_to_delete);
        print_in_order(root);
        printf("New root is: %d\n", root);
    }
    
    // Test 4: Delete another leaf (7.8)
    // Node 7.8 is at index 5
    node_to_delete = search(root, 7.8);
     if (node_to_delete != NIL) {
        printf("\nDeleting leaf node %.2f (index %d)...\n", GLB[node_to_delete].key, node_to_delete);
        delete(&root, node_to_delete);
        print_in_order(root);
        printf("New root is: %d\n", root);
    }

    printf("\n--- Final Tree (after deletions) ---\n");
    print_in_order(root);
    if (minimum_key != NIL) {
        printf("Final minimum key is at index: %d (key: %.2f)\n", minimum_key, GLB[minimum_key].key);
    }

    return 0;
}


