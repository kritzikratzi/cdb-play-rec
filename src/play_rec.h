
// lists all devices, and does not much else
int list_devices();

// plays a file and captures the input
// returns 0 on success
int play_rec(
			 const char * in_file,
			 const char * out_file_1,
			 const char * out_file_2,
			 uint32_t in_dev_idx,
			 uint32_t out_dev_idx
			 );

int find_in_dev(const char * name);
int find_out_dev(const char * name);
