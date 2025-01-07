#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

/* added mutex lock to manage concurrent access to this node*/
struct node {
    int value;
    struct node* next; //ptr to the next value
    pthread_mutex_t lock; // mutex to synchronize access to this node.
    // add more fields
};

struct list {
    node* head;
    pthread_mutex_t entrance; // mutex to synchronize access to the head node.
    // add fields
};

void print_node(node* node)
{
    // DO NOT DELETE
    if (node)
    {

        printf("%d ", node->value);

    }
}

/* Creates and initializes a new empty linked list */
list* create_list()
{
    struct list* list = (struct list*)malloc(sizeof(struct list));
    if (!list) exit(EXIT_FAILURE);// check allocation
    pthread_mutex_init(&(list->entrance), NULL);
    list->head = NULL;
    return list;
}

/* Deletes a list, frees all nodes, and destroys their mutex locks */
void delete_list(list* list)
{
    // Check if the list exists
    if (list == NULL)
    {
        return;
    }

    // locking the entrance so no thread accesses the head node 
    pthread_mutex_lock(&(list->entrance));

    // Initiating iteration pointer
    node* iter = list->head;
    node* current = NULL;

    // Iterating over the list
    while (iter != NULL)
    {
        // Destroying the lock and the node
        current = iter;
        iter = iter->next;
        pthread_mutex_destroy(&(current->lock));
        free(current);
    }

    // Releasing the list's lock and destroying it
    pthread_mutex_unlock(&(list->entrance));
    pthread_mutex_destroy(&(list->entrance));
    free(list);
}

void insert_value(list* list, int value)
{
    // Check if the list exists
    if (list == NULL)
    {
        return;
    }

    // Allocate a new node with the given value
    node* added_node = (struct node*)malloc(sizeof(struct node));
    if (!added_node)
    {
        exit(EXIT_FAILURE);
        //not sure return or exit
        //return;
    }
    added_node->value = value;
    added_node->next = NULL;
    pthread_mutex_init(&(added_node->lock), NULL);

    // locking the enterance of  the list so no thread accesses the head node 
    pthread_mutex_lock(&(list->entrance));

    // Setting the new node as the head if the list is empty
    if (list->head == NULL) {

        list->head = added_node;

        // Releasing the locks
        pthread_mutex_unlock(&(list->entrance));
        return;
    }

    // Setting the new node as the head if that's the proper location
    if (list->head->value >= value) {
        added_node->next = list->head;
        list->head = added_node;

        // Releasing the locks
        pthread_mutex_unlock(&(list->entrance));
        return;
    }

    // Initiating iteration pointers
    node* iter = list->head;
    node* prev = NULL;

    // Locking iter
    pthread_mutex_lock(&(iter->lock));

    // Releasing the entrance
    pthread_mutex_unlock(&(list->entrance));

    // Iterating over the list
    while (iter != NULL && iter->value <= value) {

        // Releasing the previous node and moving on to the next
        if (prev != NULL) {
            pthread_mutex_unlock(&(prev->lock));
        }

        prev = iter;

        // Acquiring the next node
        if (iter->next != NULL) {
            pthread_mutex_lock(&(iter->next->lock));
        }

        iter = iter->next;
    }

    // Insert at the middle or the end
    prev->next = added_node;
    added_node->next = iter;

    if (iter != NULL) {
        pthread_mutex_unlock(&(iter->lock));
    }

    pthread_mutex_unlock(&(prev->lock));
}

void remove_value(list* list, int value)
{

    // Check if the list exists
    if (list == NULL)
    {
        return;
    }

    // locking the enterance of  the list so no thread accesses the head node 
    pthread_mutex_lock(&(list->entrance));

    // Checking if the head exists
    if (list->head == NULL) {
        pthread_mutex_unlock(&(list->entrance));
        return;
    }

    // Removing the head if that's the requested node
    if (list->head->value == value) {
        node* toDelete = list->head;
        list->head = list->head->next;

        pthread_mutex_destroy(&(toDelete->lock));
        free(toDelete);

        // Releasing the lock
        pthread_mutex_unlock(&(list->entrance));
        return;
    }

    // Initiating iteration pointers
    node* curr = list->head;
    node* prev = NULL;

    // Locking iter
    pthread_mutex_lock(&(curr->lock));

    // Releasing the entrance
    pthread_mutex_unlock(&(list->entrance));

    // Iterating over the list
    while (curr != NULL && curr->value != value)
    {
        // Checking if the next node exists
        if (curr->next != NULL) {

            // Locking the next node
            pthread_mutex_lock(&(curr->next->lock));

            // Unlocking the previous node
            if (prev != NULL)
                pthread_mutex_unlock(&prev->lock);

            // Moving the iterator to the next step
            prev = curr;
            curr = curr->next;
        }
        else {

            // Next is null, list does not contain value
            if (prev != NULL)
                pthread_mutex_unlock(&prev->lock);
            pthread_mutex_unlock(&curr->lock);
            return;
        }
    }

    // curr - node to be removed , prev - node before it
    prev->next = curr->next;

    // Unlocking the nodes and removing the requested one
    pthread_mutex_unlock(&curr->lock);
    pthread_mutex_destroy(&curr->lock);
    free(curr);
    pthread_mutex_unlock(&prev->lock);

}

void print_list(list* list)
{
    // Check if the list exists
    if (list == NULL)
    {
        printf("\n");
        return;
    }

    // locking the enterance of  the list so no thread accesses the head node 
    pthread_mutex_lock(&(list->entrance));

    // Checking if the head exists
    if (list->head == NULL) {
        printf("\n");
        pthread_mutex_unlock(&(list->entrance));//Releasing the entrance
        return;
    }

    // Initiating iteration pointer
    node* iter = list->head;

    // Locking iter
    pthread_mutex_lock(&(iter->lock));

    // Releasing the entrance
    pthread_mutex_unlock(&(list->entrance));

    // Iterating over the list
    while (iter != NULL) {

        // Printing the content of the current node
        printf("%d ", iter->value);

        // Locking the next node if it exists
        if (iter->next != NULL)
            pthread_mutex_lock(&(iter->next->lock));

        // Unlocking the current node
        pthread_mutex_unlock(&(iter->lock));

        // Moving to the next node
        iter = iter->next;
    }

    printf("\n"); // DO NOT DELETE
}

void count_list(list* list, int (*predicate)(int))
{
    // Check if the list exists
    if (list == NULL)
    {
        return;
    }

    // locking the enterance of  the list so no thread accesses the head node
    pthread_mutex_lock(&(list->entrance));

    // Checking if the head exists
    if (list->head == NULL) {
        printf("0 items were counted\n");
        pthread_mutex_unlock(&(list->entrance));
        return;
    }

    // Initiating iteration pointer and counter
    node* iter = list->head;
    node* next = NULL;
    int count = 0; // DO NOT DELETE

    // Locking iter
    pthread_mutex_lock(&(iter->lock));

    // Releasing the entrance
    pthread_mutex_unlock(&(list->entrance));

    // Iterating over the list
    while (iter != NULL)
    {
        // Checking if the value satisfies the predicate
        if (predicate(iter->value)) {
            count++;
        }

        // Moving on to the next node and locking it
        next = iter->next;
        if (next != NULL) {
            pthread_mutex_lock(&next->lock);
        }

        // Unlocking the current node
        pthread_mutex_unlock(&iter->lock);

        iter = next;
    }

    // Printing the count
    printf("%d items were counted\n", count); // DO NOT DELETE
}
