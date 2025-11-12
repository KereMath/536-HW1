/**
 * Standalone Min-Heap Queue (Priority Queue) Implementation in C
 *
 * This implementation adheres to the following constraints:
 * 1. Nodes are stored in a global array `GLB`.
 * 2. The heap itself is stored in a global array `HEAP_ARRAY` which
 * contains *indices* into the `GLB` array.
 * 3. A special index `NIL` (defined as -1) represents a null pointer
 * or "not in heap".
 * 4. Node data is accessed via `GLB[index].key`.
 *
 * Author: Gemini
 */

#include <stdio.h>
#include <stdlib.h> // For exit()

// --- Constants ---

// Define our "null pointer" / "not in heap" index
#define NIL -1

// Define the maximum number of nodes the global array can hold.
#define MAX_NODES 1024

// --- Data Structures ---

/**
 * @brief Defines the structure of a node.
 * This structure is stored in the `GLB` array.
 * It contains *only* the key and the heap_index back-pointer.
 */
struct Node {
    double key;   // The key value for the node
    
    /**
     * @brief Index in the HEAP_ARRAY.
     * NIL (-1) if the node is not in the heap.
     * This is essential for an efficient heap_delete.
     */
    int heap_index; 
};

// --- Global Arrays & Variables ---

/**
 * @brief The global array for storing all nodes.
 * Can be pointed to shared memory for multi-process usage.
 */
struct Node *GLB = NULL;

/**
 * @brief Array of indices into GLB, representing the binary heap.
 * HEAP_ARRAY[0] is the index of the root node (min key).
 * Can be pointed to shared memory for multi-process usage.
 */
int *HEAP_ARRAY = NULL;

/**
 * @brief Current number of elements in the heap.
 * For shared memory usage, this should point to shared memory location.
 */
int *heap_size_ptr = NULL;

// Macro to transparently use heap_size
#define heap_size (*heap_size_ptr)

// --- Heap Helper Functions ---

/**
 * @brief Returns the parent index in the heap array.
 */
int heap_parent(int i) {
    return (i - 1) / 2;
}

/**
 * @brief Returns the left child index in the heap array.
 */
int heap_left(int i) {
    return (2 * i) + 1;
}

/**
 * @brief Returns the right child index in the heap array.
 */
int heap_right(int i) {
    return (2 * i) + 2;
}

/**
 * @brief Swaps two nodes in the heap.
 * This swaps the *indices* in HEAP_ARRAY and updates the
 * `heap_index` field in the corresponding GLB nodes.
 * @param i Index in HEAP_ARRAY.
 * @param j Index in HEAP_ARRAY.
 */
void heap_swap(int i, int j) {
    // Get the GLB indices of the nodes
    int node_i = HEAP_ARRAY[i];
    int node_j = HEAP_ARRAY[j];

    // Swap them in the heap array
    HEAP_ARRAY[i] = node_j;
    HEAP_ARRAY[j] = node_i;

    // Update their back-pointers in GLB
    GLB[node_i].heap_index = j;
    GLB[node_j].heap_index = i;
}

/**
 * @brief "Bubbles down" a node to maintain the min-heap property.
 * Assumes the subtrees at left(i) and right(i) are already min-heaps.
 * @param i The index in HEAP_ARRAY to start from.
 */
void min_heapify(int i) {
    int l = heap_left(i);
    int r = heap_right(i);
    int smallest = i;

    // Find the smallest among node i, its left child, and its right child
    if (l < heap_size && GLB[HEAP_ARRAY[l]].key < GLB[HEAP_ARRAY[smallest]].key) {
        smallest = l;
    }
    if (r < heap_size && GLB[HEAP_ARRAY[r]].key < GLB[HEAP_ARRAY[smallest]].key) {
        smallest = r;
    }

    // If the smallest is not the current node, swap them and recurse
    if (smallest != i) {
        heap_swap(i, smallest);
        min_heapify(smallest);
    }
}

/**
 * @brief "Bubbles up" a node to maintain the min-heap property.
 * @param i The index in HEAP_ARRAY to start from.
 */
void bubble_up(int i) {
    // While not at the root and parent is larger than current node
    while (i > 0 && GLB[HEAP_ARRAY[heap_parent(i)]].key > GLB[HEAP_ARRAY[i]].key) {
        heap_swap(i, heap_parent(i));
        i = heap_parent(i);
    }
}

// --- Requested Public Functions ---

/**
 * @brief Inserts a node (pre-allocated in GLB) into the heap.
 *
 * @param nodeindex The index of the node (in GLB) to be inserted.
 * @return 0 on success, -1 if node is already in heap or heap is full.
 */
int heap_insert(int nodeindex) {
    if (heap_size >= MAX_NODES) {
        printf("HeapError: Heap is full.\n");
        return -1;
    }
    if (nodeindex < 0 || nodeindex >= MAX_NODES) {
         printf("HeapError: Invalid node index %d.\n", nodeindex);
         return -1;
    }
    // Check if the node is "allocated" (we assume a key != 0 means allocated)
    // A better check might be needed in a real system.
    if (GLB[nodeindex].key == 0.0) { 
         printf("HeapError: Node %d does not appear to be initialized.\n", nodeindex);
         // return -1; // We can let it insert, but it's good practice.
    }
    if (GLB[nodeindex].heap_index != NIL) {
        printf("HeapError: Node %d is already in the heap at index %d.\n", nodeindex, GLB[nodeindex].heap_index);
        return -1; // Node is already in the heap
    }

    // Add node to the end of the heap
    int i = heap_size;
    HEAP_ARRAY[i] = nodeindex;
    GLB[nodeindex].heap_index = i;
    heap_size++;

    // Bubble it up to its correct position
    bubble_up(i);

    return 0;
}

/**
 * @brief Removes and returns the node with the smallest key from the heap.
 *
 * @return The GLB index of the node with the smallest key, or NIL (-1) if empty.
 */
int heap_pop() {
    if (heap_size <= 0) {
        printf("HeapError: Cannot pop from an empty heap.\n");
        return NIL; // Heap is empty
    }

    // Get the minimum node (at the root)
    int min_node_index = HEAP_ARRAY[0];

    // Move the last node to the root
    heap_swap(0, heap_size - 1);
    
    // Decrease heap size (this effectively removes the old root)
    heap_size--;

    // Mark the popped node as "not in heap"
    GLB[min_node_index].heap_index = NIL;

    // Fix the heap property by bubbling down the new root (if heap not empty)
    if (heap_size > 0) {
        min_heapify(0);
    }

    return min_node_index;
}

/**
 * @brief Deletes an arbitrary node from the heap.
 *
 * @param nodeindex The GLB index of the node to delete.
 * @return 0 on success, -1 if the node is not in the heap.
 */
int heap_delete(int nodeindex) {
    if (nodeindex < 0 || nodeindex >= MAX_NODES || GLB[nodeindex].heap_index == NIL) {
        printf("HeapError: Node %d is not in the heap.\n", nodeindex);
        return -1; // Not in heap
    }

    int i = GLB[nodeindex].heap_index; // Get node's position in heap

    // Swap the node with the last element
    heap_swap(i, heap_size - 1);

    // Decrease heap size (removes the target node)
    heap_size--;

    // Mark the deleted node as "not in heap"
    GLB[nodeindex].heap_index = NIL;

    // Now, the node at index `i` (which was the *last* node)
    // might be in the wrong position. We must fix the heap.
    // This check is important: only fix if the swapped node is still in bounds.
    if (i < heap_size) {
        // We must check if it needs to bubble up *or* down.
        // Check if it needs to bubble up
        if (i > 0 && GLB[HEAP_ARRAY[i]].key < GLB[HEAP_ARRAY[heap_parent(i)]].key) {
            bubble_up(i);
        } else {
            // Otherwise, it might need to bubble down
            min_heapify(i);
        }
    }

    return 0;
}

// --- Demonstration (main) ---

/**
 * @brief Helper function to print the heap array and its keys.
 */
void print_heap() {
    printf("Heap (size %d):\n", heap_size);
    if (heap_size == 0) {
        printf("  [Empty]\n");
        return;
    }
    for (int i = 0; i < heap_size; i++) {
        int node_idx = HEAP_ARRAY[i];
        printf("  Heap[%d] = (GLB Index: %d, Key: %.2f, node_heap_index: %d)\n",
               i, node_idx, GLB[node_idx].key, GLB[node_idx].heap_index);
    }
}

/* COMMENTED OUT FOR hw1.c
int main() {
    // Reset heap size
    heap_size = 0;

    // --- "Allocate" nodes in GLB ---
    // We use indices 1-5 for clarity.
    int node_indices[] = {1, 2, 3, 4, 5, 6};
    double keys[] = {10.5, 5.2, 15.1, 3.0, 7.8, 12.0};
    int num_nodes = sizeof(keys) / sizeof(double);

    printf("--- Initializing GLB nodes ---\n");
    for (int i = 0; i < num_nodes; i++) {
        int idx = node_indices[i];
        GLB[idx].key = keys[i];
        GLB[idx].heap_index = NIL; // Mark as not in heap
        printf("GLB[%d] created with key %.2f\n", idx, GLB[idx].key);
    }

    printf("\n--- Testing Heap Insert ---\n");
    for (int i = 0; i < num_nodes; i++) {
        int idx = node_indices[i];
        printf("Inserting GLB[%d] (key %.2f) into heap...\n", idx, GLB[idx].key);
        heap_insert(idx);
    }

    print_heap();

    // Test duplicate insert
    printf("\n--- Testing Duplicate Insert ---\n");
    printf("Attempting to insert GLB[4] (key 3.0) again...\n");
    heap_insert(4);
    print_heap();

    // Test 2: Pop minimum
    printf("\n--- Testing Heap Pop (Smallest Key) ---\n");
    int min_idx = heap_pop();
    if (min_idx != NIL) {
        printf("Popped min node. GLB index: %d, Key: %.2f\n", min_idx, GLB[min_idx].key);
        printf("Checking GLB[%d].heap_index: %d (should be -1)\n", min_idx, GLB[min_idx].heap_index);
    }
    print_heap();

    // Test 3: Delete an arbitrary node (e.g., 15.1 at GLB index 3)
    int delete_idx = 3;
    printf("\n--- Testing Heap Delete (GLB index %d, key %.2f) ---\n", delete_idx, GLB[delete_idx].key);
    heap_delete(delete_idx);
    printf("Checking GLB[%d].heap_index: %d (should be -1)\n", delete_idx, GLB[delete_idx].heap_index);
    print_heap();

    // Test 4: Delete a node not in heap
    printf("\n--- Testing Delete of Popped Node (GLB index %d, key %.2f) ---\n", min_idx, GLB[min_idx].key);
    heap_delete(min_idx); // This was node with key 3.0, already popped
    print_heap();

    // Test 5: Pop all remaining
    printf("\n--- Popping all remaining nodes ---\n");
    while (heap_size > 0) {
        min_idx = heap_pop();
        if (min_idx != NIL) {
            printf("Popped node. GLB index: %d, Key: %.2f\n", min_idx, GLB[min_idx].key);
        }
    }
    print_heap();

    // Test 6: Pop from empty
    printf("\n--- Testing Pop from Empty Heap ---\n");
    heap_pop();
    print_heap();

    return 0;
}
*/

