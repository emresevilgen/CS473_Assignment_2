#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include "mmio.h"

// Adjacency List
struct node
{
    int vertex;
    struct node *next;
    double weight;
};

struct Graph
{
    int numVertices;
    struct node **adjLists;
};

struct node *createNode(int v)
{
    struct node *newNode = malloc(sizeof(struct node));
    newNode->vertex = v;
    newNode->next = NULL;
    newNode->weight = DBL_MAX;
    return newNode;
}

struct Graph *createAGraph(int vertices)
{
    struct Graph *graph = malloc(sizeof(struct Graph));
    graph->numVertices = vertices;

    graph->adjLists = malloc(vertices * sizeof(struct node *));

    int i;
    for (i = 0; i < vertices; i++)
        graph->adjLists[i] = NULL;

    return graph;
}

void addEdge(struct Graph *graph, int s, int d, double weight)
{
    struct node *newNode = createNode(d);
    newNode->next = graph->adjLists[s];
    newNode->weight = weight;
    graph->adjLists[s] = newNode;
}

// Min Heap
struct heap_node
{
    double cost;
    int index;
};

void swap_indexes(int *a, int *b)
{
    int temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

void swap(struct heap_node *a, struct heap_node *b, int *indexes)
{
    swap_indexes(&indexes[(*a).index], &indexes[(*b).index]);

    struct heap_node temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

int get_parent(struct heap_node *A, int size, int index)
{
    if ((index > 0) && (index < size))
    {
        return (index - 1) / 2;
    }
    return -1;
}

int get_left_child(struct heap_node *A, int size, int index)
{
    if (((2 * index + 1) < size) && (index >= 0))
        return 2 * index + 1;
    return -1;
}

int get_right_child(struct heap_node *A, int size, int index)
{
    if ((((2 * index) + 2) < size) && (index >= 0))
        return (2 * index) + 2;
    return -1;
}

void min_heapify(struct heap_node *A, int *A_indexes, int size, int index)
{
    int left_child_index = get_left_child(A, size, index);
    int right_child_index = get_right_child(A, size, index);

    // Finding smallest of current, left child and right child
    int smallest = index;

    if ((left_child_index < size) && (left_child_index > 0))
    {
        if (A[left_child_index].cost < A[smallest].cost)
        {
            smallest = left_child_index;
        }
    }

    if ((right_child_index < size && (right_child_index > 0)))
    {
        if (A[right_child_index].cost < A[smallest].cost)
        {
            smallest = right_child_index;
        }
    }

    // Smallest is not the current
    if (smallest != index)
    {
        swap(&A[index], &A[smallest], A_indexes);
        min_heapify(A, A_indexes, size, smallest);
    }
}

void build_min_heap(struct heap_node *A, int *A_indexes, int size)
{
    int i;
    for (i = size / 2; i >= 0; i--)
    {
        min_heapify(A, A_indexes, size, i);
    }
}

struct heap_node extract_min(struct heap_node *A, int *A_indexes, int *sizePtr)
{
    struct heap_node root = A[0];
    *sizePtr = *sizePtr - 1;
    swap(&A[0], &A[*sizePtr], A_indexes);
    min_heapify(A, A_indexes, *sizePtr, 0);
    return root;
}

void decrease_key(struct heap_node *A, int *A_indexes, int size, int index, double key)
{
    A[index].cost = key;
    while ((index > 0) && (A[get_parent(A, size, index)].cost > A[index].cost))
    {
        swap(&A[index], &A[get_parent(A, size, index)], A_indexes);
        index = get_parent(A, size, index);
    }
}

void print_heap(struct heap_node *A, int size)
{
    int i;
    int j = 0;
    for (i = 0; i < size; i++)
    {
        if (i == (1 << j) - 1)
        {
            printf("\n");
            j++;
        }
        printf("%lf ", A[i].cost);
    }
    printf("\n");
}

int readMatrixFile(FILE *file, struct Graph **graphPtr, int *no_of_verticesPtr)
{
    MM_typecode matcode;

    // if (mm_read_banner(file, &matcode) != 0)
    // {
    //     // printf("Could not process Matrix Market banner.\n");
    // }

    int ret_code;
    int M, N, nz;

    if ((ret_code = mm_read_mtx_crd_size(file, &M, &N, &nz)) != 0)
        return 1;

    *graphPtr = createAGraph(M);
    struct Graph *graph = *graphPtr;

    int i;
    int src;
    int dest;
    double weight;

    for (i = 0; i < nz; i++)
    {
        int ret = fscanf(file, "%d %d %lf\n", &src, &dest, &weight);
        addEdge(graph, src - 1, dest - 1, weight);
    }
    *no_of_verticesPtr = M;

    if (file != stdin)
        fclose(file);

    return 0;
}

int writeCostsToFile(char *filepath, double *costs, int size)
{
    FILE *fp;
    fp = fopen(filepath, "w");
    int i;
    for (i = 0; i < size; i++)
    {
        if (costs[i] == -1)
            fprintf(fp, "%d\n", -1);
        else
            fprintf(fp, "%.8f\n", costs[i]);
    }

    fclose(fp);
}

// Dijkstra
void dijkstra(struct Graph *graph, double *costs, int src)
{
    int no_of_vertices = graph->numVertices;
    int min_heap_size = no_of_vertices;
    struct heap_node *min_heap = malloc(min_heap_size * sizeof(struct heap_node));
    int *min_heap_indexes = malloc(min_heap_size * sizeof(struct heap_node));

    for (int i = 0; i < no_of_vertices; i++)
    {
        costs[i] = DBL_MAX;
        min_heap[i].cost = DBL_MAX;
        min_heap[i].index = i;
        min_heap_indexes[i] = i;
    }

    build_min_heap(min_heap, min_heap_indexes, no_of_vertices);

    costs[src - 1] = 0;
    decrease_key(min_heap, min_heap_indexes, no_of_vertices, min_heap_indexes[src - 1], 0);

    while (min_heap_size > 0)
    {
        struct heap_node min_heap_node = extract_min(min_heap, min_heap_indexes, &min_heap_size);
        int cost = min_heap_node.cost;
        int index = min_heap_node.index;

        struct node *curr = graph->adjLists[index];
        while (curr != NULL)
        {
            if (min_heap_indexes[curr->vertex] < min_heap_size &&
                costs[index] != DBL_MAX &&
                curr->weight + costs[index] < costs[curr->vertex])
            {
                costs[curr->vertex] = costs[index] + curr->weight;
                decrease_key(min_heap, min_heap_indexes, min_heap_size, min_heap_indexes[curr->vertex], costs[curr->vertex]);
            }
            curr = curr->next;
        }
    }

    for (int i = 0; i < no_of_vertices; i++)
    {
        if (costs[i] == DBL_MAX)
            costs[i] = -1;
    }

    free(min_heap);
    free(min_heap_indexes);
}

// Main function
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s [martix-market-filename]\n", argv[0]);
        exit(1);
    }

    // Open and read file
    FILE *file;
    if ((file = fopen(argv[1], "r")) == NULL)
    {
        fprintf(stderr, "File %s cannot be opened\n", argv[1]);
        exit(1);
    }

    struct Graph *graph;
    int no_of_vertices = 0;

    if (readMatrixFile(file, &graph, &no_of_vertices) != 0)
    {
        fprintf(stderr, "Could not read file %s\n", argv[1]);
        exit(1);
    }

    double *costs = malloc(no_of_vertices * sizeof(double));
    dijkstra(graph, costs, 1);
    char *filepath = "a.txt";
    writeCostsToFile(filepath, costs, no_of_vertices);
    printf("Done. Output file a.txt is created at the same directory with the code.\n");

    int i;
    struct node **lists = graph->adjLists;
    for (i = 0; i < no_of_vertices; i++)
    {
        struct node *cur = lists[i];
        struct node *next;
        while (cur != NULL)
        {
            next = cur->next;
            cur->next = NULL;
            free(cur);
            cur = next;
        }
    }
    free(graph->adjLists);
    free(graph);
    free(costs);
    return 0;
}
