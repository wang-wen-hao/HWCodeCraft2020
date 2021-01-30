# HWCodeCraft2020
初赛+复赛代码
## 一、问题描述
题目背景是打击金融犯罪，给了我们账户的转账记录，让我们找出所有的循环转账信息，并按照字典序输出。
#### 输入信息
* 输入为包含资金流水的文本文件，每一行代表一次资金交易记录，包含本端账号ID, 对端账号ID, 转账金额，用逗号隔开。
* 本端账号ID和对端账号ID为一个32位的正整数
* 转账金额为一个32位的正整数
* 转账记录最多为28万条
* 每个账号平均转账记录数< 10
* 账号A给账号B最多转账一次
* 举例如下，其中第一行[1,2,100]表示ID为1的账户给ID为2的账户转账100元：

1,2,100

1,3,100
#### 要求的输出信息：
循环转账的路径长度最小为3（包含3）最大为7（包含7），例如账户A给账户B转账，账户B给账户A转账，循环转账的路径长度为2，不满足循环转账条件。
#### 评判标准是
环的结果正确的情况下，用时越短的胜出。
## 二、总体思路&思想
这道题就是一道有向图找环的问题.其实对于有向图找环的题目，最常用的就是dfs算法，而我们再改进了朴素dfs算法，还针对本题做了许多特定的优化。
算法主要分为五部分，**读数据、建图、拓扑排序、找环、写数据**。
![](https://github.com/wang-wen-hao/HWCodeCraft2020/img/节点图.jpg)
## 三、读数据
### 1.方案一：ifstream
```c
void load_data(string path)
{
	ifstream infile(path.c_str());
	string line;
	if (!infile) {
		cout << "文件打开失败!" << endl;
		exit(0);
	}
	while (infile) 
	{
		getline(infile, line);
		stringstream sin(line);
		char ch;
		int v, u, w;
		sin >> v >> ch >> u >> ch >> w;
		if (infile.fail())
		{
			break;
		}
	}
	infile.close();
}

```
### 2.方案二：fscanf
```c
void parseInput(string& testFile) {
	FILE* file = fopen(testFile.c_str(), "r");
	int u, v, c;
	int cnt = 0;
	while (fscanf(file, "%d,%d,%d", &u, &v, &c) != EOF) {
		inputs.push_back(u);
		inputs.push_back(v);
		++cnt;
	}
	#ifdef TEST
	printf("%d Records in Total\n", cnt);
	#endif
}
```
### 3.方案三：fread
```c
FILE* file = fopen(path.c_str(), "r");
char raw[100];
int v, u, w;
fseek(file, 0, SEEK_END);//次优
int file_size = ftell(file);
fseek(file, 0, SEEK_SET);
char* buffer = new char[file_size + 1];
fread(buffer, 1, file_size, file); //
buffer[file_size] = '\0';
for (char* sp = buffer;;) {
	char* ep = NULL;
	v = strtol(sp, &ep, 10);
	sp = ep + 1;
	u = strtol(sp, &ep, 10);
	sp = ep + 1;
	w = strtol(sp, &ep, 10);
	sp = ep + 2;
	vertex_raw.emplace_back(v);
	vertex_raw.emplace_back(u);
	if (*ep != '\r') break;//线上可能是\r\n，得改
}

```
### 4.方案四：mmap（最终方案）
```c
int fd = open(file_name.c_str(), O_RDONLY);
int file_size = lseek(fd, 0, SEEK_END);
char* buffer = (char *)mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
for (char* sp = buffer;;)
{
	char* ep = NULL;
	v = strtol(sp, &ep, 10);
	sp = ep + 1;
	u = strtol(sp, &ep, 10);
	sp = ep + 1;
	w = strtol(sp, &ep, 10);
	sp = ep + 2;
	input2values(v, u);
	if (*ep != '\r') break;//本地改为\n，+2改为+1
}

```
## 四、建图
这一步是进行找环的预备工作，主要是建立**入度图、出度图、节点到[0, n )的映射**。想得到这些，得先得到节点数量。
**要点：**
* unique去重+erase得到节点数量
* vector提前reserve
* 哈希表存放节点到索引的映射
* vector改为数组存储，提前用new 开辟空间
## 五、拓扑排序
因为有很多出度为零和入度为零的点，这些点是不可能构成环的。所以会被删除。
![](https://github.com/wang-wen-hao/HWCodeCraft2020/img/拓扑排序.jpg)
## 六、找环(重点！)
### 6.1基本思想
找环的思想简单来说，就是，循环从小到大，依此以每个节点作为头节点，从这个节点开始能不能找到环。
### 6.2常见易错点
* "8"字型环问题
* 大环套小环问题
### 6.3朴素DFS及其问题
最朴素的算法要跑好4个多小时...
### 6.4针对朴素DFS的第一次改进
提前存储一层路径
P2inv = vector<unordered_map<int, vector<int>>>(nodeCnt, unordered_map<int, vector<int>>());
### 6.5对朴素DFS的第二次改进之双向DFS的由来
last数组指示了走一步能够访问到头节点 vector<bool>
info数组指示了走两步能够访问到头节点 unordered_map<int, vector<int>>
info2指示了走三步能否访问到头节点 unordered_map<int, vector<seq>>
```c
	bool *last;//下一个结点是否为头结点
	last = new bool[nodesNum]();
 
	struct seq {
		int first;
		int second;
		seq(int f, int s) :first(f), second(s) {};
	};
 
	unordered_map<int, vector<int>> info;
	unordered_map<int, vector<seq>> info2;
 
	for (auto& iin : GraphIn[head]) {
		if (iin == -1) break;
		if (iin <= head) continue;
		last[iin] = 1;//iin的下一个是head结点
		for (auto& jin : GraphIn[iin]) {
			if (jin == -1) break;
			if (jin <= head) continue;
			info[jin].push_back(iin);
			for (auto& kin : GraphIn[jin]) {
				if (kin == -1) break;
				if (kin <= head) continue;
				int t = info2[kin].size()-1;
				for (; t >=0; t--) {
					if (info2[kin][t].first <= jin) break;
				}
				++t;
				info2[kin].insert(info2[kin].begin()+t,seq(jin, iin));
			}
		}
	}

```
### 6.6对双向DFS的改进1
这个这个存储两步的环判断是不是多余的呢？因为在建立info2的时候，如果逆向三步可以返回头节点，那就说明发现了长度为三的环了。这样在建立info2的时候就把长度三的环找到了。
```c
struct seq {
	items first;
	items second;
	seq(items f, items s) :first(f), second(s) {};
};
```
此时，用的info2的数据结构时：
```c
unordered_map<int, vector<seq>> info2;
```
存放的数据是：
如果k->j->i->head，那么info2[k][索引] = seq(j, i);
此时，比如头节点的出端节点是id1，如果info2中存在key值为id1，就代表id1走三步可以访问到head节点，那么也就存在长度为4的环，一次类推，第四层可以找到长度为7的环。
### 6.7对双向DFS的改进2之unordered_map改数组
```c
vector<vector<seq>> info2(nodesNum, vector<seq>());
```
同时引入了另一个辅助向量，记作reset_flags：
```c
bool* reset_flags = new bool[nodesNum]();
```
在往info2数组中添加元素之前，先判断这个索引有没有被清空过，如果没有被清空过，那么就先将这个索引里面的数据清空再往里面填入最新数据。如果对应索引的标志被设置为true，说明已经被清空过了，所以直接往info2对应索引中往里面填就可以了，只不过要保证有序。
代码示例：
```c
if(reset_flags[k_id] == true) {
	int t = info2[k_id].size() - 1;
	for (; t >= 0; t--) {
		if (info2[k_id][t].first.secondID <= j_id) break;
	}
	++t;                                                        
	info2[k_id].insert(info2[k_id].begin() + t, seq(it1, it2));
	}
else {                                                        
	info2[k_id].clear();
	reset_flags[k_id] = true;
	info2[k_id].push_back(seq(it1, it2));                                
}                
```
其实这个标志位的作用是判断对应的索引位置有没有被清空过，那么info2就可以实现局部clear了，不需要全局整个都clear，它还有一个作用，如果它被设置为true了，那代表这个节点走三步可以访问到head节点，所以在dfs的时候，判断reset_flags是不是true就可以了。
示例代码：
if(reset_flags[it1SID])则发现了长度为4的环
### 6.8在前面改进的基础上更上一层楼
```c
int* reset_flags = new int[nodesNum]();
memset(reset_flags, -1, sizeof(int) * nodesNum);
reset_flag[a] == head吗？如果等于，那么这个节点就可以走三步访问到head节点，如果不等于，就不会访问到head
```
到这里就是我们单线程下的最优方案了。
### 6.9多线程找环
两种线程分配方案的比较

### 6.10其他小改进
#### push_back vs emplace_back
原本的：all_path[depth].push_back(circuit(depth, path)); 
改进的：all_path[depth].emplace_back(depth, path);
#### 递归该迭代
将dfs的递归形式改为迭代形式，速度有所提升
#### 使用memcpy
void* memcpy( void* dest, const void* src, std::size_t count );
参考： https://zh.cppreference.com/w/cpp/string/byte/memcpy 

## 七、存储结果
### 方案一：ofstream
```c
void save_data(string path) {
	string line;
	ofstream outfile(path.c_str());
	outfile << result.size() << endl;
	for (int i = 0; i < result.size(); ++i) {
		for (int j = 0; j < result[i].size(); ++j) {
			outfile << result[i][j];
			if (j != result[i].size() - 1)
			outfile << ',';
		}
		outfile << endl;
	}
	outfile.close();
}
```
### 方案二：fputs
```c
void save_fputs(const string& outputFile) {
	auto t = clock();
	FILE* fp = fopen(outputFile.c_str(), "w");
	char buf[1024];
	printf("环总数: %d\n", ansCnt);
#ifdef TEST
	printf("Total Loops %d\n", ansCnt);
#endif
	int idx = sprintf(buf, "%d\n", ansCnt);
	buf[idx] = '\0';
	fputs(buf, fp);
	for (int i = DEPTH_LOW_LIMIT; i <= DEPTH_HIGH_LIMIT; i++) {
		//sort(ans[i].begin(),ans[i].end());
		for (auto& x : ans[i]) {
			auto path = x.path;
			int sz = path.size();
			idx = 0;
			for (int j = 0; j < sz - 1; j++) {
				idx += sprintf(buf + idx, "%s", idsComma[path[j]].c_str());
			}
			idx += sprintf(buf + idx, "%s", idsLF[path[sz - 1]].c_str());
			buf[idx] = '\0';
			fputs(buf, fp);
		}
	}
	fclose(fp);
	cout << clock() - t << endl;
}
```
### 方案三：fwrite
```c
void save_fwrite(const string& outputFile) {
	auto t = clock();
	FILE* fp = fopen(outputFile.c_str(), "wb");
	char buf[1024];
	printf("环总数: %d\n", ansCnt);
#ifdef TEST
	printf("Total Loops %d\n", ansCnt);
#endif
	int idx = sprintf(buf, "%d\n", ansCnt);
	buf[idx] = '\0';
	fwrite(buf, idx, sizeof(char), fp);
	for (int i = DEPTH_LOW_LIMIT; i <= DEPTH_HIGH_LIMIT; i++) {
		//sort(ans[i].begin(),ans[i].end());
		for (auto& x : ans[i]) {
			auto path = x.path;
			int sz = path.size();
			//idx=0;
			for (int j = 0; j < sz - 1; j++) {
				auto res = idsComma[path[j]];
				fwrite(res.c_str(), res.size(), sizeof(char), fp);
			}
			auto res = idsLF[path[sz - 1]];
			fwrite(res.c_str(), res.size(), sizeof(char), fp);
		}
	}
	fclose(fp);
	cout << clock() - t << endl;
}
```
### 方案四：多进程写入
```c
void save(const string& outputFile) {//最优版本
	/*
	模式：
		O_RDWR  读写模式
		O_CREAT 如果指定文件不存在，则创建这个文件 
		O_TRUNC 如果文件存在，并且以只写/读写方式打开，则清空文件全部内容 
	*/
	int fd = open(outputFile.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
	char* ansNumBuffer = new char[1024];
	int idx = sprintf(ansNumBuffer, "%d\n", ansNum);
	/*
	#include <unistd.h>
	ssize_t write(int filedes, const void *buf, size_t nbytes);
	返回值：写入文件的字节数（成功）；-1（出错）
	write 函数向 filedes 中写入 nbytes 字节数据，数据来源为 buf 。返回值一般总是等于 nbytes，
	否则就是出错了。常见的出错原因是磁盘空间满了或者超过了文件大小限制。
	*/
	write(fd, ansNumBuffer, idx);

	int buffs[] = { 0,buff3,buff3 + buff4,buff3 + buff4 + buff5,buff3 + buff4 + buff5 + buff6,buff3 + buff4 + buff5 + buff6 + buff7 };
	int iii;
	pid_t pid = 1;
	for (int i = 3; i <= 7; i++) {
		if (pid>0) {//确保只有父进程能产生子进程，否则会爆炸
			iii = i - 3;
			pid = fork();
		}
	}
	if (pid == -1) {
		cerr << "bad" << endl;
	}
	else {
		if (pid == 0) {
			int i = iii + 3;
			int j = 0;
			int buf_size = buffs[iii + 1] - buffs[iii];//每个子进程的buf的大小
			char* buf = new char[buf_size];
			int id = 0;
			while (ans[i - MIN_DEPTH][j] > 0 || ans[i - MIN_DEPTH][j + 1] > 0) {
				for (int u = 0; u< i - 1; ++u) {
					for (auto k : idsDouhao[ans[i - MIN_DEPTH][j]])
						buf[id++] = k;
					j++;
				}
				for (auto k : idsHuanhang[ans[i - MIN_DEPTH][j]])
					buf[id++] = k;
				j++;
			}
			lseek(fd, idx + buffs[iii], SEEK_SET);//移动到写的位置
			write(fd, buf, id);
			exit(0);
		}
	}

}
```