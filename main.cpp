#include "modules/cosine_lsh.h"

int main (int argc, char *argv[]) {

	// 读取哈希表的配置
	LSH lsh{"../modules/config"};

	// 如果不能从文件读入哈希表
	if (!lsh.get_isRead() ) {
		// initialize
		lsh.init_hash_table();
		lsh.init_hash_function();
		// create hash table
		lsh.hash_from_file();
	}
	// query from hash table
	lsh.query_from_file();

	// close files
	lsh.finish();
	return 0;
}
