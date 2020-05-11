#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "hash_map.h"
#define MAX_TABLE_SIZE 10

struct entry* table_array[MAX_TABLE_SIZE];
int num_of_enrties = 0;



int insert_entry(unsigned int key, unsigned int size, unsigned char *data){
	int index = 0;
	struct entry *new_item;
	while (table_array[index] != 0){
		index++;
		if(index >= MAX_TABLE_SIZE){
			return -2;// table full
		}
	}

	new_item = (struct entry*) malloc(sizeof(struct entry) + size);
	if((new_item != 0)&&(data != 0)){
		table_array[index] = new_item;
		new_item->key = key;
		new_item->size = size;
		memcpy(new_item->data, data, size);	
	}else{
		return -1;//not enough mem
	}
    num_of_enrties++;
	return 0;
}
/*ret = number of deleted entries*/
int delete_entry(unsigned int key){
	int index = 0, ret = 0;
	while (index < MAX_TABLE_SIZE){
		if(table_array[index] != 0){
			if (table_array[index]->key == key){
				free(table_array[index]);
				table_array[index] = 0;
				ret++;
			}
		}
		index++;
	}
    num_of_enrties = num_of_enrties - ret;
	return ret;
}

struct entry*  find_entry(unsigned int key){
	int index = 0;
	while (index < MAX_TABLE_SIZE){
		if(table_array[index] != 0){
			if (table_array[index]->key == key){
				return table_array[index];
			}
		}
		index++;
	}
	return 0;
}
/*  frees all allocated memory for hash map
    returns number of freed entry  */
int flush_map(){
    int ret=0;
    for(int index = 0; index < MAX_TABLE_SIZE; index++){
        if(table_array[index] != NULL){
            free(table_array[index]);
            table_array[index] = 0;
            ret++;
        }else{
            
        }
    }
    num_of_enrties = 0;
    return ret;
}

struct entry*  get_an_entry(){
	int index = 0;
    struct entry *temp;
	while (index < MAX_TABLE_SIZE){
		if(table_array[index] != 0){
            temp = table_array[index];
            table_array[index] = 0;
            num_of_enrties = num_of_enrties - 1;
            return temp;
		}
		index++;
	}
	return 0;
}

int get_num_of_enrties(){
    return num_of_enrties;
}

/*
test driver
int main(){
	char test[] = "asdas0";
	struct entry* na;
	for(int i = 0; i < MAX_TABLE_SIZE + 2; i++){
		printf("insert %d %s\n",i,test);
		insert_entry(i, sizeof(test), test);
		test[5] += 1;
	}
	test[5] = '0';
	printf("\ntest\n");
	for(int i = 0; i < MAX_TABLE_SIZE + 2; i++){
		na = find_entry(i);
		if(na != 0){
			printf("find %d    %s\n",i,na->data);				
		}else{
			printf("not found %d\n",i);
		}
	}

	delete_entry(1);
	delete_entry(3);
	delete_entry(5);

	printf("\ntest\n");
	for(int i = 0; i < MAX_TABLE_SIZE + 2; i++){
		na = find_entry(i);
		if(na != 0){
			printf("find %d    %s\n",i,na->data);				
		}else{
			printf("not found %d\n",i);
		}
	}
	char test2[] = "asdasd";
	insert_entry(1, sizeof(test2), test2);


	printf("\ntest\n");
	for(int i = 0; i < MAX_TABLE_SIZE + 2; i++){
		na = find_entry(i);
		if(na != 0){
			printf("find %d    %s\n",i,na->data);				
		}else{
			printf("not found %d\n",i);
		}
	}	
}
*/
