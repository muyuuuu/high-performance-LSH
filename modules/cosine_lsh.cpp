#include "cosine_lsh.h"

/*
  构造函数  
  创建 LSH 对象
*/
LSH::LSH(){
}

LSH::LSH(std::string s) {
  config_path = s;
  parse_config(config_path);
}

/*
  解析配置文件
*/
void LSH::parse_config(std::string s) {
  // 输出结果保留两位小数
  oFile << std::fixed << std::setprecision(2);

  std::ifstream temp;
  std::string str;
  temp.open(s);
  int n_t;

  // 哈希表的数量
  move_to_line(temp, 0);
  temp >> n_t;
  n_tables = n_t;

  // 读取哈希函数的数目
  move_to_line(temp, 1);
  temp >> n_t;
  n_functions = n_t;

  // 读取每张图片返回多少查询结果
  move_to_line(temp, 2);
  temp >> n_t;
  n_query_number = n_t;

  // base file 路径
  move_to_line(temp, 3);
  temp >> str;
  bFile.open(str);

  // query file 路径
  move_to_line(temp, 4);
  temp >> str;
  qFile.open(str);

  // out file 路径
  move_to_line(temp, 5);
  temp >> str;
  oFile.open(str);

  // base query 文件有多少行
  get_n_lines();
  // 数据维度
  char sp = '\t';
  get_n_dimensions(sp);

  // 存储结果，query 有多少行，就存储多少结果
  // 优先级队列，存储结果，选择相似度最接近的
  res.resize(n_query_lines);

  oFile << "Base file dimension : " << n_base_lines 
            << " X " << n_dim << std::endl;
  oFile << "Query file dimension : " << n_query_lines
            << " X " << n_dim << std::endl;

  // 从文件读取哈希表
  move_to_line(temp, 6);
  temp >> n_t;

  // read pointer name
  if (n_t == 1) {
    isRead = true;

    temp >> str;
    r_p_hash_table.open(str);

    temp >> str;
    r_p_hash_function.open(str);

    temp >> str;
    r_p_amp_function.open(str);

    // 读取数据
    read_data();
  }

  move_to_line(temp, 7);
  // 保存文件的路径，默认为空
  temp >> n_t;
  // 保存哈希表等数据
  // save pointer name
  if (n_t == 1) {
    save = true;

    temp >> str;
    s_p_hash_table.open(str);

    temp >> str;
    s_p_hash_function.open(str);

    temp >> str;
    s_p_amp_function.open(str);
  }

  // 即读取也保存，则扩容，但不建议扩容太猛
  if (isRead == true && save == true) {
    hash_from_file();
  }

  temp.close();
}

bool LSH::get_isRead() const {
  return isRead;
}

void LSH::read_data() {
  // 读取哈希表
  int n_t, n_f;
  std::string str;

  r_p_hash_table >> n_t >> n_f;
  n_tables = n_t;
  n_functions = std::log2(n_f);
  move_to_line(r_p_hash_table, 1);

  init_hash_table();
  int cnt{0}, x;
  double y;
  for (int i = 0; i < n_tables; i++) {
    for (int j = 0; j < std::pow(2, n_functions); j++) {
      std::getline(r_p_hash_table, str);
      std::stringstream s{str};
      while (s >> x) {
        hashTables[i][j].push_back(x);
      }
    }
  }
  r_p_hash_table.close();
  
  // 读取二级哈希 function dim
  r_p_hash_function >> n_t >> n_f;
  n_functions = n_t;
  n_dim = n_f;
  hashFunction.resize(n_functions, std::vector<double>(n_dim));

  move_to_line(r_p_hash_function, 1);
  for (int i = 0; i < n_functions; i++) {
    for (int j = 0; j < n_dim; j++) {
      r_p_hash_function >> y;
      hashFunction[i][j] = y;
    }
  }
  oFile << "Hash function's dimension: " << n_functions 
            << " X " << n_dim << std::endl;
  r_p_hash_function.close();

  // 读取一级哈希函数
  r_p_amp_function >> n_t >> n_f;
  amplifyFunction.resize(n_tables, std::vector<int>(n_functions));
  oFile << "amplify function's dimension: " << n_tables 
            << " X " << n_functions << std::endl;
  
  move_to_line(r_p_amp_function, 1);
  for (int i = 0; i < n_tables; i++) {
    for (int j = 0; j < n_functions; j++) {
      r_p_amp_function >> x;
      amplifyFunction[i][j] = x;
    }
  }
  r_p_amp_function.close();
}

/*
  n_tables, 2^n_functions, []
*/
void LSH::init_hash_table() {
  hashTables.resize(n_tables,
    std::vector<std::list<int> > (std::pow(2, n_functions), std::list<int>(0)));
  for (int i = 0; i < n_tables; i++) {
    for (int j = 0; j < std::pow(2, n_functions); j++) {
      hashTables[i][j].resize(0);
    }
  }
  oFile << "Hash tables' dimension: " << n_tables
            << " X " << std::pow(2, n_functions) << std::endl;
}


/*
  初始化两个哈希函数
    一级哈希：
        amplifyFunction， 维度是 [n_tables, n_functions]，每一行的取值是 [0, n_functions-1]
    二级哈希：
        hashFunction，维度是 [n_functions, dim]，获取 amplifyFunction 的值，作为一维索引
                    ，取值范围是 0 为均值，0.2 为方差的正态分布随机数
*/
void LSH::init_hash_function() {

  std::default_random_engine gen(3);
  // 初始化
  std::normal_distribution<double> dis(0, 1);

  hashFunction.resize(n_functions, std::vector<double>(n_dim));
  oFile << "Hash function's dimension: " << n_functions 
            << " X " << n_dim << std::endl;
  
  for (int i = 0; i < n_functions; i++) {
    hashFunction[i].clear();
    for (int j = 0; j < n_dim; j++) {
      hashFunction[i][j] = dis(gen);
    }
  }

  amplifyFunction.resize(n_tables, std::vector<int>(n_functions));
  oFile << "amplify function's dimension: " << n_tables 
            << " X " << n_functions << std::endl;
  
  std::vector<int> ivec(n_functions);
  std::iota(ivec.begin(), ivec.end(), 0);
  std::srand ( unsigned ( 1 ) );
  for (int i = 0; i < n_tables; i++) {
    // 乱序并分配
    std::random_shuffle(ivec.begin(), ivec.end());
    amplifyFunction[i].assign(ivec.begin(), ivec.end());
  }
}

/*
  单表哈希
*/
void LSH::hash_one_table(std::vector<double>& tmp, int t, int line) {
  int pos{0};
  double sum{0.0};

  // 每一个函数
  // t 是第 t 个表
  for (int f = 0; f < n_functions; f++) {
    sum = 0;
    // 维度相乘
    for (int i = 0; i < n_dim; i++) {
      sum += tmp[i] * hashFunction[ amplifyFunction[t][f] ][i];
    }
    if (sum > 0)
      pos += std::pow(2, f);
  }
  // 越界处理
  if (pos >= std::pow(2, n_functions)) {
      // 2^9 = 511.9998 -> 511
      int a = std::pow(2, n_functions);
      pos %= a;
  }
  // 容器追加，避免处理哈希冲突
  // 追加到第 t 个表的第 pos 个桶中
  hashTables[t][pos].push_back(line);
}

/*
  由文件构建哈希表，放入某个表的 pos 位置的桶中
*/
void LSH::hash_from_file() {
  double x;
  std::vector<double> tmp;
  auto start = std::chrono::high_resolution_clock::now();

  // 以表为单位，并行
  ThreadPool pool{n_tables};
  pool.init();
  // 处理每一条数据
  set_pointer_begin(bFile);
  // 第 0 行开始处理 base
  for (int line = 0; line < n_base_lines; line++) {
    if (line % 500 == 499) {
      auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed1 = end - start;
      oFile << "After " << elapsed1.count() << " seconds, "
                << line + 1 <<  " items has been hashed ! " << std::endl;
    }
    // 空间换时间
    tmp.clear();
    for (int i = 0; i < n_dim + 1; i++) {
      bFile >> x;
      if (i != 0)
        tmp.push_back(x);
    }

    // 处理每一张表
    // for (int t = 0; t < n_tables; t++)
    //   hash_one_table(tmp, t, line);
    // 第一条数据，对应第 0 行
    for (int t = 0; t < n_tables; t++) {
      auto func = std::bind(&LSH::hash_one_table, this, tmp, t, line);
      pool.submit(func);
    }
  }
  pool.shutdown();

  // 保存数据
  if (save == true) {
    save_data();
  }

}

/*
  保存结果
*/
void LSH::save_data() {
  
  // 保存哈希表
  s_p_hash_table << n_tables << " " 
                       << std::pow(2, n_functions) << std::endl;
  for (int i = 0; i < n_tables; i++) {
		for (int j = 0; j < std::pow(2, n_functions); j++) {
			for (auto& k : hashTables[i][j]) {
					s_p_hash_table << k << " ";
			}
			if (i == n_tables-1 && j == std::pow(2, n_functions)-1)
				continue;
			s_p_hash_table << std::endl;
		}
	}
  s_p_hash_table.close();
  
  // 保存 hash function
  s_p_hash_function << n_functions << " " << n_dim << std::endl;
  for (int i = 0; i < n_functions; i++) {
    for (int j = 0; j < n_dim; j++) {
      if (j == 0)
        s_p_hash_function << hashFunction[i][j];
      else
        s_p_hash_function << " " << hashFunction[i][j];
    }
    if (i != n_functions-1)
      s_p_hash_function << std::endl;
  }
  s_p_hash_function.close();

  // 保存 amp function
  s_p_amp_function << n_tables << " " << n_functions << std::endl;
  for (int i = 0; i < n_tables; i++) {
    for (int j = 0; j < n_functions; j++) {
      if (j == 0)
        s_p_amp_function << amplifyFunction[i][j];
      else
        s_p_amp_function << " " << amplifyFunction[i][j];
    }
    if (i != n_tables-1)
      s_p_amp_function << std::endl;
  }
  s_p_amp_function.close();

  oFile << "Hash Table, Hash Function, AmplifyFunction has been saved. "
              << std::endl
              << "===================================================="
              << std::endl;
}


/*
  单表查询
*/
void LSH::query_one_table(int t, int line, const std::vector<double>& tmp) {
  auto s = std::chrono::high_resolution_clock::now();
  double sum{0};
  int pos{0};

  for (int i = 0; i < n_functions; i++) {
    sum = 0;
    for (int j = 0; j < n_dim; j++) {
      sum += tmp[j] * hashFunction[amplifyFunction[t][i]][j];
    }
    if (sum > 0)
      pos += std::pow(2, i);
  }
  if (pos >= std::pow(2, n_functions)) {
    int a = std::pow(2, n_functions);
    pos %= a;
  }

  // 遍历这个桶中所有的项
  for (auto& i: hashTables[t][pos]) {
    int res_item = -10;
    // i 是桶中的元素，也就是 base 的第 i 行
    double dis = calCosineDis(i, tmp, &res_item);
    // 距离与项，按照第一项降序
    // line 是 query 的第 line 行
    res.emplace(line, dis, res_item);
  }
}


/*
  对 query 文件的每一行进行查询
  记录查询的平均时间
*/
void LSH::query_from_file() {
  double s{0.0};

  ThreadPool pool{100};
  pool.init();
  n_query_lines = 30;
  auto start = std::chrono::high_resolution_clock::now();
  for (int t = 0; t < n_tables; t++) {
    for (int line = 0; line < n_query_lines; line++) {
      std::vector<double> tmp;
      move_to_line(qFile, line);
      double x{0.0};
      for (int i = 0; i < n_dim + 1; i++) {
        qFile >> x;
        if (i != 0)
          tmp.push_back(x);
      }
      auto func = std::bind(&LSH::query_one_table, this,
            t, line, tmp);
      pool.submit(func);
    }
  }
  oFile << "All Query Item has been submitted !"
        << std::endl
        << "===================================================="
        << std::endl;
  pool.shutdown();
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed1 = end - start;
  s += elapsed1.count();

  for (int line = 0; line < n_query_lines; line++) {
    // 当前行的查询与计时
    oFile << "Query: " << line << ", NN LSH Items:" 
                << std::endl;

    int tmp{-2}, cnt{0};
    while (!res.empty(line) && cnt < n_query_number) {
      if (tmp != res.top(line).second) {
        tmp = res.top(line).second;
        oFile << " ==>> " << res.top(line).first
                    << "\t" << res.top(line).second << std::endl;
        cnt++;
      }
      res.pop(line);
    }

    oFile << "Item " << line << " has been queried" << std::endl;
    oFile << "====================================================" << std::endl;

  }
  oFile << "Average Time: " << s / n_query_lines << " seconds. ";
}


/*
  计算余弦相似度，取值范围是 [-1, 1]
    -1 完全相反
    0 毫无关系
    1 正相关
*/
double LSH::calCosineDis(int line, const std::vector<double>& v2, int* res_item) {
  double x{0.0}, product{0.0}, x_norm{0.0}, y_norm{0.0};

  std::ifstream temp, b;
  temp.open(config_path);
  move_to_line(temp, 3);
  std::string str;
  temp >> str;
  temp.close();

  b.open(str);
  move_to_line(b, line);

  std::vector<double> v1;
  for (int i = 0; i < n_dim + 1; i++) {
    b >> x;
    if (i == 0) {
      *res_item = static_cast<int>(x);
    } else {
      v1.push_back(x);
    }
  }
  b.close();

  for (int i = 0; i < n_dim; i++) {
    product += v1[i] * v2[i];
    x_norm += v1[i] * v1[i];
    y_norm += v2[i] * v2[i];
  }

  x_norm = std::sqrt(x_norm);
  y_norm = std::sqrt(y_norm);
  if (std::abs(x_norm * y_norm) < 1e-6)
    return -1;

  return (product / (x_norm * y_norm));
}


/*
  获取数据的维度。第一项是索引号，要忽略
*/
void LSH::get_n_dimensions(char split) {
  int dim{0};
  std::string str;
  std::getline(bFile, str);
  for (int i = 0; i < str.size(); i++) {
    if (str[i] == split)
      dim ++;
  }
  set_pointer_begin(bFile);
  n_dim = dim;
}


/*
  获取 query 和 base 两个文件有多少行
*/
void LSH::get_n_lines() {
  // base 文件
  unsigned long long lines{0};
  std::string str;
  set_pointer_begin(bFile);
  while (std::getline(bFile, str))
    lines++;
  set_pointer_begin(bFile);
  n_base_lines = lines;

  // query 文件
  lines = 0;
  set_pointer_begin(qFile);
  while (std::getline(qFile, str))
    lines++;
  set_pointer_begin(qFile);
  n_query_lines = lines;
}

/*
  每次建立哈希表都太慢了保存哈希表
*/

void LSH::set_pointer_begin(std::ifstream& f) {
  f.clear();
  f.seekg(0, std::ios::beg);
}


void LSH::move_to_line(std::ifstream& f, int line) {
  std::string s;
  f.clear();
  f.seekg(0, std::ios::beg);
  for (int i = 0; i < line; i++)
    std::getline(f, s);
}


void LSH::finish() {
  bFile.close();
  qFile.close();
  oFile.close();
}