

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"




/*
 * The real definition of struct list / struct node
 */

struct dplist_node {
    dplist_node_t *prev, *next;
    void *element;
};

struct dplist {
    dplist_node_t *head;

    void *(*element_copy)(void *src_element);

    void (*element_free)(void **element);

    int (*element_compare)(void *x, void *y);
};


dplist_t *dpl_create(// callback functions
        void *(*element_copy)(void *src_element),
        void (*element_free)(void **element),
        int (*element_compare)(void *x, void *y)
) {
    dplist_t *list;
    list = malloc(sizeof(struct dplist));
    list->head = NULL;
    list->element_copy = element_copy;
    list->element_free = element_free;
    list->element_compare = element_compare;
    return list;
}

void dpl_free(dplist_t **list, bool free_element) {

    //Do extensive testing with valgrind.
    if(list == NULL){
        return;
    }
    dplist_t *p = *list;
    if(p == NULL){
        return;
    }
    else if(p->head == NULL){
        free(p);
        *list=NULL;
        return;
    }

    else{
        dplist_node_t *node = p->head;
        dplist_node_t *next = node->next;

        while (next != NULL) {
            if(free_element) {
                void* element = next->prev->element;
                void** pointer = &element;
                p->element_free(pointer);
            }
            free(next->prev);
            node = next;
            next = next->next;
        }
        if(free_element) {
            void* element = node->element;
            void** pointer = &element;
            p->element_free(pointer);
        }

        free(node);
        free(p);
        *list=NULL;
        return;
    }
}

dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy) {

    dplist_node_t *ref_at_index, *list_node;
    if (list == NULL) return NULL;

    list_node = malloc(sizeof(dplist_node_t));

    if(insert_copy){
        void* e = list->element_copy(element);
        list_node->element = e;
    }
    else{
        list_node->element = element;
    }

    // pointer drawing breakpoint
    if (list->head == NULL) { // covers case 1
        list_node->prev = NULL;
        list_node->next = NULL;
        list->head = list_node;
        // pointer drawing breakpoint
    } else if (index <= 0) { // covers case 2
        list_node->prev = NULL;
        list_node->next = list->head;
        list->head->prev = list_node;
        list->head = list_node;
        // pointer drawing breakpoint
    } else {
        ref_at_index = dpl_get_reference_at_index(list, index);
        assert(ref_at_index != NULL);
        // pointer drawing breakpoint
        if (index < dpl_size(list)) { // covers case 4
            list_node->prev = ref_at_index->prev;
            list_node->next = ref_at_index;
            ref_at_index->prev->next = list_node;
            ref_at_index->prev = list_node;
            // pointer drawing breakpoint
        } else { // covers case 3
            assert(ref_at_index->next == NULL);
            list_node->next = NULL;
            list_node->prev = ref_at_index;
            ref_at_index->next = list_node;
            // pointer drawing breakpoint
        }
    }
    return list;

}

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element) {

    if(list == NULL){
        return NULL;
    }
    else if(list->head == NULL){
        return list;
    }
    else{
        dplist_node_t *node = dpl_get_reference_at_index(list, index);
        if(index <= 0){
            if(dpl_size(list) == 1){
                list->head = NULL;
            }
            else{
                list->head = node->next;
                node->next->prev = NULL;
            }
        }
        else if(index >= dpl_size(list)-1){
            if(dpl_size(list) == 1){
                list->head = NULL;
            }
            else {
                node->prev->next = NULL;
            }
        }
        else{
            node->next->prev = node->prev;
            node->prev->next = node->next;
        }
        if(free_element){
            void* element = node->element;
            void** pointer = &element;
            list->element_free(pointer);
        }
        free(node);
        return list;
    }

}

int dpl_size(dplist_t *list) {
    int count = 1;

    if(list == NULL){
        return -1;
    }
    else if(list->head == NULL){
        return 0;
    }
    else{
        dplist_node_t *dplist_node = list->head;
        while(dplist_node->next != NULL){
            dplist_node = dplist_node->next;
            count++;
        }
        return count;
    }
}

void *dpl_get_element_at_index(dplist_t *list, int index) {
    dplist_node_t* node = dpl_get_reference_at_index(list, index);

    if(node == NULL){
        return NULL;
    }
    else{
        return node->element;
    }

}

int dpl_get_index_of_element(dplist_t *list, void *element) {

    int count;

    if(list == NULL || list->head == NULL){
        return -1;
    }
    else{
        dplist_node_t *dplist_node = list->head;
        for(count=0; count < dpl_size(list); count++){
            if(!list->element_compare(dplist_node->element,element)){
                return count;
            }
            dplist_node = dplist_node->next;
        }
        return -1;
    }

}

dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {
    int count;

    if(list == NULL || list->head == NULL){
        return NULL;
    }
    else if(index <= 0){
        return list->head;
    }
    else if(index > dpl_size(list)-1){
        dplist_node_t *dplist_node = list->head;
        while(dplist_node->next != NULL){
            dplist_node = dplist_node->next;
        }
        return dplist_node;
    }
    else{
        dplist_node_t *dplist_node = list->head;
        for(count=0; count < index; count++){
            dplist_node = dplist_node->next;
        }
        return dplist_node;
    }

}

void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference) {

    if(list == NULL || list->head == NULL){
        return NULL;
    }
    if(reference == NULL){
        return NULL;
    }
    dplist_node_t *node = list->head;
    for(int count = 0; count<dpl_size(list); count++){
        if(node == reference){
            return node->element;
        }
        node = node->next;
    }
    return NULL;
}


