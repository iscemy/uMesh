struct entry{
	unsigned int key;
	unsigned int size;
	unsigned char data[0];
};

struct entry*  get_an_entry();
int insert_entry(unsigned int key, unsigned int size, unsigned char *data);
int delete_entry(unsigned int key);
struct entry*  find_entry(unsigned int key);
int flush_map();
int get_num_of_enrties();


