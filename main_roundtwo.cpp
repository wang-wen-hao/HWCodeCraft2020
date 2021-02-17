#define _CRT_SECURE_NO_WARNINGS

#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
//#include <ctime>
#include <queue>
#include <unordered_map>
#include <string>
#include <string.h> 

#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>

using namespace std;

constexpr auto CacheLineSize = 64;
const int THREAD_COUNT = 4;
const int SIZE = 7 * 6000000;
#define MAX_DEPTH 7
#define MIN_DEPTH 3


int ans3[THREAD_COUNT][SIZE];
int ans4[THREAD_COUNT][SIZE];
int ans5[THREAD_COUNT][SIZE];
int ans6[THREAD_COUNT][SIZE];
int ans7[THREAD_COUNT][SIZE];

int i3[THREAD_COUNT], i4[THREAD_COUNT], i5[THREAD_COUNT], i6[THREAD_COUNT], i7[THREAD_COUNT];
int *is[] = { i3,i4,i5,i6,i7 };
int buff3[THREAD_COUNT], buff4[THREAD_COUNT], buff5[THREAD_COUNT], buff6[THREAD_COUNT], buff7[THREAD_COUNT]; 
int *buffs[] = { buff3,buff4,buff5,buff6,buff7 };
int ansNum[THREAD_COUNT];


struct items {
	items() = default;
	int secondID = -1;
	int money = -1;
	items(int secondid, int money) : secondID(secondid), money(money) {}
};


class Sol {
public:
	vector<vector<items > > Graph;
	vector<vector<items > > GraphIn;
	unordered_map<int, int> idMapping; //sorted id to 0...n
	vector<string> idsDouhao;
	vector<string> idsHuanhang;
	vector<int> data_pairs; //u-v pairs
	vector<int> data_tuple; //

	int *in_deg; 
	int *out_deg; 
	int *buff_count;

	int nodesNum; 
	int Items = CacheLineSize / sizeof(float);



	inline void input2values(int& u, int& v) {
		data_pairs.push_back(u);
		data_pairs.push_back(v);
	}

	inline void input3values(int& u, int& v, int& w) {
		data_tuple.push_back(u);
		data_tuple.push_back(v);
		data_tuple.push_back(w);
	}


	//mmap
	void parseInput(string& file_name) {
		int v, u, w;
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
			input2values(v, u);
			input3values(v, u, w);
			if (*ep == '\n')
				sp = ep + 1;
			else if (*ep == '\r')
				sp = ep + 2;
			else
				break;
		}
	}

	void construct_Graph() {
		sort(data_pairs.begin(), data_pairs.end());
		data_pairs.erase(unique(data_pairs.begin(), data_pairs.end()), data_pairs.end());
		nodesNum = data_pairs.size();

		Graph.resize(nodesNum);//
		GraphIn.resize(nodesNum);
		in_deg = new int[nodesNum](); 
		out_deg = new int[nodesNum](); 
		buff_count = new int[nodesNum]();

		nodesNum = 0;
		for (auto& x : data_pairs) {
			string tmp = to_string(x);
			idsDouhao.push_back(tmp + ','); 
			idsHuanhang.push_back(tmp + '\n'); 
			buff_count[nodesNum] = tmp.size() + 1;
			idMapping[x] = nodesNum++; 
			
		}

		int sz = data_tuple.size();  
		for (int i = 0; i < sz; i += 3) {
			int u = idMapping[data_tuple[i]]; 
			int v = idMapping[data_tuple[i + 1]]; 
			int w = data_tuple[i + 2];			
												
			Graph[u].push_back(items(v, w));
			GraphIn[v].push_back(items(u, w));		
			++in_deg[v];
			++out_deg[u];
		}



	}

	
	void topoSortIn() {

		queue<int> q; 		
		for (int i = 0; i < nodesNum; i += Items) {
			for (int ii = 0; ii < Items && i + ii < nodesNum; ++ii) {
				if (0 == in_deg[i + ii])
					q.push(i + ii);
			}
		}

		while (!q.empty()) { 
			int u = q.front(); 
			q.pop(); 
			for (auto& v : Graph[u]) {
				if (v.secondID == -1) break;
				if (0 == --in_deg[v.secondID]) 
					q.push(v.secondID);
			}
		}

		
		for (int i = 0; i < nodesNum; i += Items) {
			for (int ii = 0; ii < Items && i + ii < nodesNum; ++ii) {
				if (in_deg[i + ii] == 0 && (!Graph[i + ii].empty())) {
					Graph[i + ii][0].secondID = -1;
				}
				else {
					sort(Graph[i + ii].begin(), Graph[i + ii].end(), \
						[](items a, items b) {
						return a.secondID < b.secondID;
					}); //
				}
			}
		}

	}

	void topoSortOut() {

		for (int i = 0; i < nodesNum; i += Items) {
			for (int ii = 0; ii < Items && i + ii < nodesNum; ++ii) {
				
				sort(GraphIn[i + ii].begin(), GraphIn[i + ii].end(), \
					[](items a, items b) {
					return a.secondID < b.secondID;
				});
			}
		}
	}

	void topoSort() {
		thread threadIn = thread(&Sol::topoSortIn, this);
		thread threadOut = thread(&Sol::topoSortOut, this);

		threadIn.join();
		threadOut.join();
	}

	struct seq {
		items first;
		items second;
		seq(items f, items s) :first(f), second(s) {};
	};

	
	inline bool compare(int x, int y)
	{
		if ((double(y) / double(x)) >= 0.2 && (double(y) / double(x)) <= 3.0)
			return true;
		else
			return false;
	}

	void find_circle_iterMethod(int s) {

		bool *vertext_visit = new bool[nodesNum](); //����״̬
		// int *last = new int[nodesNum]();
		// memset(last, -1, sizeof(int) * nodesNum);
		// unordered_map<int, vector<items>> info;
		//unordered_map<int, vector<seq>> info2;
		int* price_to_head = new int[nodesNum];
		// vector<int> price_to_head(nodesNum, 0);

		//vector<int> reachable(nodesNum, -1);
		int* reset_flags = new int[nodesNum]();
		memset(reset_flags, -1, sizeof(int) * nodesNum);
		vector<vector<seq>> info2(nodesNum, vector<seq>());
		// info2.resize(nodesNum);

		for (int head = s; head < nodesNum; head += THREAD_COUNT) {
			if (Graph[head].empty()) continue;
			if (Graph[head][0].secondID == -1) continue;

			// memset(reset_flags, 0, sizeof(bool) * nodesNum);

			for (auto& iin : GraphIn[head]) {
				if (iin.secondID == -1) break;
				if (iin.secondID <= head) continue;
				int i_id = iin.secondID, i_money = iin.money;
				
				price_to_head[i_id] = i_money;
				for (auto& jin : GraphIn[iin.secondID]) {
					if (jin.secondID == -1) break;
					int j_id = jin.secondID, j_money = jin.money;
					if (j_id <= head || !compare(j_money, i_money)) continue;
					items it2 = items(i_id, j_money);
					// info[jin.secondID].push_back(it2);
					for (auto& kin : GraphIn[j_id]) {
						if (kin.secondID == -1) break;
						int k_id = kin.secondID, k_money = kin.money;
						if (k_id < head || \
							k_id == i_id || \
							!compare(k_money, j_money)) continue;

						if (k_id == head && \
						compare(i_money, k_money)) {
							//如果是空的呢？待会儿加上
							int tmp3[] = {head, j_id, i_id};
							// cout << "3circle:" << head << ", " << jin.secondID << ", " << iin.secondID << endl;


							if(i3[s] == 0 || ans3[s][i3[s] - 3] < head || ans3[s][i3[s] - 2] <= j_id) {
								memcpy(ans3[s] + i3[s], tmp3, sizeof tmp3);								
							}
							else {								
								int left = i3[s] / 3 - 1;
								while (ans3[s][left*3] == head && \
								ans3[s][left*3+1] > j_id)
								{
									left--;
								}
								left++;							
								int* tmp = new int[i3[s] - left * 3];
								memcpy(tmp, ans3[s] + left * 3, sizeof(int) * (i3[s] - left * 3) );
								memcpy(ans3[s] + left*3, tmp3, sizeof tmp3);
								memcpy(ans3[s] + (left+1)*3, tmp, sizeof(int) * (i3[s] - left * 3));
							}	
							i3[s] += 3;
							buff3[s] += buff_count[head];
							buff3[s] += buff_count[j_id];
							buff3[s] += buff_count[i_id];						
							++ansNum[s];
							continue;
						}
						//reachable[k_id] = head;
						items it1 = items(j_id, k_money);
						if(reset_flags[k_id] == head) {
							int t = info2[k_id].size() - 1;
							for (; t >= 0; t--) {
								if (info2[k_id][t].first.secondID <= j_id) break;
							}
							++t;							
							info2[k_id].insert(info2[k_id].begin() + t, seq(it1, it2));
						}
						else {							
							info2[k_id].clear();
							reset_flags[k_id] = head;
							info2[k_id].push_back(seq(it1, it2));				
						}									
					}
				}
			}

			
			vertext_visit[head] = true;
			auto it1 = lower_bound(Graph[head].begin(), Graph[head].end(), head, \
				[](items a, int b) {return a.secondID < b; }); 
			for (; it1 != Graph[head].end(); ++it1) {
				const int it1SID = it1->secondID;
				const int it1Money = it1->money;
				if (vertext_visit[it1SID]) continue; 

				vertext_visit[it1SID] = true;				

				if(reset_flags[it1SID] == head) {
					for (auto& trail : info2[it1SID]) {
						auto trail1 = trail.first;
						auto trail2 = trail.second;
						if(vertext_visit[trail1.secondID] || vertext_visit[trail2.secondID] || \
						!compare(it1Money, trail1.money)) continue;
						if(compare(price_to_head[trail2.secondID], it1Money)) {
							int tmp4[] = {head, it1SID, trail1.secondID, trail2.secondID};
							memcpy(ans4[s] + i4[s], tmp4, sizeof tmp4);
							i4[s] += 4;
							buff4[s] += buff_count[head];
							buff4[s] += buff_count[it1SID];
							buff4[s] += buff_count[trail1.secondID];
							buff4[s] += buff_count[trail2.secondID];
							++ansNum[s];
						}
					}
					
				}


				auto it2 = lower_bound(Graph[it1SID].begin(), Graph[it1SID].end(), head, \
					[](items a, int b) {return a.secondID < b; });

				for (; it2 != Graph[it1SID].end(); ++it2) {
					const int it2SID = it2->secondID;
					const int it2Money = it2->money;
					if (vertext_visit[it2SID] || \
						!compare(it1Money, it2Money)) continue;									

					vertext_visit[it2SID] = true;

					if(reset_flags[it2SID] == head) {
						for (auto& trail : info2[it2SID]) {
							auto trail1 = trail.first;
							auto trail2 = trail.second;
							if(vertext_visit[trail1.secondID] || vertext_visit[trail2.secondID] || \
							!compare(it2Money, trail1.money)) continue;
							if(compare(price_to_head[trail2.secondID], it1Money)) {
								int tmp5[] = {head, it1SID, it2SID, trail1.secondID, trail2.secondID};
								memcpy(ans5[s] + i5[s], tmp5, sizeof tmp5);
								i5[s] += 5;
								buff5[s] += buff_count[head];
								buff5[s] += buff_count[it1SID];
								buff5[s] += buff_count[it2SID];
								buff5[s] += buff_count[trail1.secondID];
								buff5[s] += buff_count[trail2.secondID];
								++ansNum[s];
							}
						}
						
					}


					auto it3 = lower_bound(Graph[it2SID].begin(), Graph[it2SID].end(), head, \
						[](items a, int b) {return a.secondID < b; });

					for (; it3 != Graph[it2SID].end(); ++it3) {
						const int it3SID = it3->secondID;
						const int it3Money = it3->money;
						if (vertext_visit[it3SID] || \
							!compare(it2Money, it3Money)) continue;
						
						
						vertext_visit[it3SID] = true;						

						if (reset_flags[it3SID] == head) {
							for (auto& trail : info2[it3SID]) {
								auto& trail1 = trail.first;
								auto& trail2 = trail.second;
								if (vertext_visit[(trail1).secondID] || \
									!compare(it3Money, trail1.money)) continue;

								if (!vertext_visit[(trail2).secondID] \
									&& compare(price_to_head[(trail2).secondID], it1Money)) {

									int tmp6[] = { head, it1SID, it2SID, it3SID, (trail1).secondID, (trail2).secondID };
									memcpy(ans6[s] + i6[s], tmp6, sizeof tmp6);
									i6[s] += 6;
									buff6[s] += buff_count[head];
									buff6[s] += buff_count[it1SID];
									buff6[s] += buff_count[it2SID];
									buff6[s] += buff_count[it3SID];

									buff6[s] += buff_count[(trail1).secondID];
									buff6[s] += buff_count[(trail2).secondID];
									++ansNum[s];
								}
							}
							
						}
					

						auto it4 = lower_bound(Graph[it3SID].begin(), Graph[it3SID].end(), head, \
							[](items a, int b) {return a.secondID < b; });

						for (; it4 != Graph[it3SID].end(); ++it4) {
							const int it4SID = it4->secondID;
							const int it4Money = it4->money;
							if (vertext_visit[it4SID] || \
								!compare(it3Money, it4Money)) continue;
							

							vertext_visit[it4SID] = true;							


							if (reset_flags[it4SID] == head) {
								for (auto& trail : info2[it4SID]) {
									auto& trail1 = trail.first;
									auto& trail2 = trail.second;
									if (vertext_visit[(trail1).secondID] || \
										!compare(it4Money, trail1.money)) continue;

									if (!vertext_visit[(trail2).secondID] \
										&& compare(price_to_head[(trail2).secondID], it1Money)) {

										int tmp7[] = { head, it1SID, it2SID, it3SID, it4SID, (trail1).secondID, (trail2).secondID };
										memcpy(ans7[s] + i7[s], tmp7, sizeof tmp7);
										i7[s] += 7;
										buff7[s] += buff_count[head];
										buff7[s] += buff_count[it1SID];
										buff7[s] += buff_count[it2SID];
										buff7[s] += buff_count[it3SID];
										buff7[s] += buff_count[it4SID];
										buff7[s] += buff_count[(trail1).secondID];
										buff7[s] += buff_count[(trail2).secondID];
										++ansNum[s];
									}
								}
								
							}

							vertext_visit[it4SID] = false;
						}
						vertext_visit[it3SID] = false;
					}
					vertext_visit[it2SID] = false;
				}
				vertext_visit[it1SID] = false;
			}
			vertext_visit[head] = false;			
			// info2.clear();
		}
	}


	void search() {
		thread threads[THREAD_COUNT];

		for (int i = 0; i < THREAD_COUNT; i++) {
			threads[i] = (thread(&Sol::find_circle_iterMethod, this, i));
			//cout << "thread " << i << " start!" << endl;
		}

		for (int i = 0; i < THREAD_COUNT; i++) {
			threads[i].join();
							 
		}
	}


	void save(const string& outputFile) {
		int fd = open(outputFile.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
		char* ansNumBuffer = new char[1024];
		int ansNumTotal = 0;
		for (int t = 0; t < THREAD_COUNT; ++t)
			ansNumTotal += ansNum[t];
		int idx = sprintf(ansNumBuffer, "%d\n", ansNumTotal);
		write(fd, ansNumBuffer, idx);

		int buffc[6];
		buffc[0] = 0;
		for (int i = 3; i <= 7; ++i) {
			int tmp = 0;
			for (int t = 0; t < THREAD_COUNT; ++t) {
				tmp += buffs[i - 3][t];
			}
			buffc[i - 2] = buffc[i - 3] + tmp;
		}
		int iii;
		pid_t pid = 1;
		for (int i = 3; i <= 7; i++) {
			if (pid>0) {
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

				int(*ans)[SIZE];
				switch (i) {
				case 3: ans = ans3; break;
				case 4: ans = ans4; break;
				case 5: ans = ans5; break;
				case 6: ans = ans6; break;
				case 7: ans = ans7; break;
				}

				int buf_size = buffc[iii + 1] - buffc[iii];
				char* buf = new char[buf_size];

				int j[THREAD_COUNT];
				memset(j, 0, THREAD_COUNT * sizeof(int));
				int id = 0;
				int t = -1;
				int node = -1;
				while (id < buf_size) {
					++t; ++node;
					if (t >= THREAD_COUNT) t = 0;
					if (node > nodesNum) break;
					while (j[t] < is[iii][t] && ans[t][j[t]] == node) {
						for (int u = 0; u< i - 1; ++u) {
							for (auto k : idsDouhao[ans[t][j[t]]])
								buf[id++] = k;
							j[t]++;
						}
						for (auto k : idsHuanhang[ans[t][j[t]]])
							buf[id++] = k;
						j[t]++;
					}

				}

				lseek(fd, idx + buffc[iii], SEEK_SET);
				write(fd, buf, id);
				exit(0);
			}
		}

	}


};




int main()
{
	// string testFile = "test_data_dense.txt";
	// string outputFile = "result_dense_v27_f3.txt";

	 string testFile = "test_data.txt";
	 string outputFile = "result_v27_f2.txt";

	// string testFile = "naive10.txt";
	// string outputFile = "result_naive10_v27_f3.txt";

	//string testFile = "test_data_new_v2.txt";
	//string outputFile = "result_3.txt";

	//string testFile = "test_data_fusai.txt";
	//string outputFile = "result_fusai_v27_f4.txt";

	/*string testFile = "/data/test_data.txt";
	string outputFile = "/projects/student/result.txt";*/

	//string testFile = "/root/data/test_data_N.txt";
	//string outputFile = "/root/projects/student/result_v30_f10_N.txt";

	//auto t = clock();
	Sol sol;
	//auto t1 = clock();
	sol.parseInput(testFile);
	//auto t2 = clock();
	sol.construct_Graph();
	//auto t2_2 = clock();
	sol.topoSort();
	//auto t2_3 = clock();
	sol.search();
	//auto t3 = clock();
	sol.save(outputFile);
	//	auto t4 = clock();
	//	cout << "reading data used time: " << (double)(t2 - t1) / CLOCKS_PER_SEC << "s" << endl;
	//	cout << "Construct graph used time: " << (double)(t2_2 - t2) / CLOCKS_PER_SEC << "s" << endl;
	//	cout << "tuopu sort used time: " << (double)(t2_2 - t2_2) / CLOCKS_PER_SEC << "s" << endl;
	//	cout << "find circle used time: " << (double)(t3 - t2_3) / CLOCKS_PER_SEC << "s" << endl;
	//	cout << "save time: " << (double)(t4 - t3) / CLOCKS_PER_SEC << " s" << endl;
	//	cout << "all used time: " << (double)(t4 - t) / CLOCKS_PER_SEC << " s" << endl;

	//system("pause");
	return 0;
}
