#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <queue>
#include <unordered_map>
#include <string>
#include <string.h> 

#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

constexpr auto CacheLineSize = 64;
#define MAX_DEPTH 7
#define MIN_DEPTH 3


int ans3[3 * 500000];
int ans4[4 * 500000];
int ans5[5 * 1000000];
int ans6[6 * 2000000];
int ans7[7 * 3000000];
int *ans[] = { ans3,ans4,ans5,ans6,ans7 };
int i3, i4, i5, i6, i7;
int buff3 = 0, buff4 = 0, buff5 = 0, buff6 = 0, buff7 = 0; //buff������

class Sol {
public:
	//~Sol() {//���ؿ����ͷ�һ�£����Ի��Ͼ�û��Ҫ�ˣ��˷�ʱ��
	//	delete[] Graph;
	//	delete[] GraphIn;
	//	delete[] in_deg;
	//	delete[] out_deg;
	//	delete[] vertext_visit;
	//	delete[] last;
	//}



	int(*Graph)[50];//����ͼ
	int(*GraphIn)[255]; //���ͼ
	unordered_map<int, int> idMapping; //sorted id to 0...n
	vector<string> idsDouhao;
	vector<string> idsHuanhang;
	vector<int> data_pairs; //u-v pairs�����е�id���ݣ��������ظ�
	int *in_deg; //��ȣ���С�ǽڵ���Ŀ
	int *out_deg; //���ȣ���СҲ�ǽڵ���Ŀ
	bool *vertext_visit; //����״̬
	bool *last;//��һ������Ƿ�Ϊͷ���

	int *buff_count;//�ַ���������

	int nodesNum; //��¼�ڵ�id����Ŀ
	int ansNum;
	int Items = CacheLineSize / sizeof(float);
	inline void input2values(int& u, int& v) {
		data_pairs.push_back(u);
		data_pairs.push_back(v);
	}

	//to do: mmap����ȡ����id�ͶԶ�id���ݣ�����ŵ�һ��vector���У�����Ϊ�˵õ����е�id��
	void parseInput(string& file_name) {
		int v, u, w;
		//���ض�
		//FILE* file = fopen(file_name.c_str(), "r");
		//char raw[100];
		//fseek(file, 0, SEEK_END);//����
		//int file_size = ftell(file);
		//fseek(file, 0, SEEK_SET);
		//char* buffer = new char[file_size + 1];
		//fread(buffer, 1, file_size, file);
		//buffer[file_size] = '\0';

		//linux��
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
			if (*ep != '\r') break;//���ظ�Ϊ\n��+2��Ϊ+1
		}
	}


	void construct_Graph() {
		auto temp_data = data_pairs;
		sort(temp_data.begin(), temp_data.end());
		temp_data.erase(unique(temp_data.begin(), temp_data.end()), temp_data.end());
		nodesNum = temp_data.size();

		Graph = new int[nodesNum][50];//�����࣬����nodesNum�����ڴ�
		GraphIn = new int[nodesNum][255];
		in_deg = new int[nodesNum](); //��ȣ���С�ǽڵ���Ŀ
		out_deg = new int[nodesNum](); //���ȣ���СҲ�ǽڵ���Ŀ
		buff_count = new int[nodesNum]();
		vertext_visit = new bool[nodesNum](); //����״̬
		last = new bool[nodesNum]();
		memset(Graph, -1, sizeof(int) * nodesNum * 50);
		memset(GraphIn, -1, sizeof(int) * nodesNum * 255);


		nodesNum = 0; //����Ҫ����������ڵ��������¼���Ϊ0
		for (auto& x : temp_data) {
			string tmp = to_string(x);//ֻ��Ҫto_stringһ��
			idsDouhao.push_back(tmp + ','); //������id�ö��Ÿ����ͽ�ȥ
			idsHuanhang.push_back(tmp + '\n'); //������id�û��з������ͽ�ȥ

			buff_count[nodesNum] = tmp.size() + 1;//���ֵ��ַ����ȼ���һλ����

			idMapping[x] = nodesNum++; // ��¼id����������ǰ�������ں����ѯ���졣
		}

		int sz = data_pairs.size(); //Ӧ�õ���������������



		for (int i = 0; i < sz; i += Items) {
			for (int j = 0; j < Items && i + j < sz; j += 2) {
				int u = idMapping[data_pairs[i + j]], v = idMapping[data_pairs[i + j + 1]];
				Graph[u][out_deg[u]] = v;
				GraphIn[v][in_deg[v]] = u;
				++in_deg[v];
				++out_deg[u];
			}
		}

		for (int i = 0; i < nodesNum; i += Items) {
			for (int ii = 0; ii < Items && i + ii < nodesNum; ++ii) {
				sort(GraphIn[i + ii], &GraphIn[i + ii][in_deg[i + ii]]); //������ڵ�ĳ���id��������һ�£�
			}
		}

	}

	//degs--��Ȼ��߳��ȣ���ȥ����Ȼ��߳���Ϊ0�Ľڵ�
	void topoSort(int degs[], bool flag) {
		queue<int> q; //����һ������

					  //����cacheline
		for (int i = 0; i < nodesNum; i += Items) {
			for (int ii = 0; ii < Items && i + ii < nodesNum; ++ii) {
				if (0 == degs[i + ii])
					q.push(i + ii);
			}
		}

		while (!q.empty()) { //ֱ������Ϊ��
			int u = q.front(); //ȡ���е���Ԫ�أ��Ƚ��ȳ����ȳ�����ʱ����С��
			q.pop(); //����
					 //��������ڵ�����г���id������
			for (int& v : Graph[u]) {
				if (v == -1) break;
				if (0 == --degs[v]) //�������ڵ�Ķȼ�һ����0�ˣ���ô�����
					q.push(v);
			}
		}

		
		for (int i = 0; i < nodesNum; i += Items) {
			for (int ii = 0; ii < Items && i + ii < nodesNum; ++ii) {
				if (degs[i + ii] == 0) {
					Graph[i + ii][0] = -1;
					//cnt++; //�����������������Ϊ��TESTģʽ�¿�ɾ���˶��ٸ��ڵ㣬��ʵ����ȥ����
				}
				else if (flag) {
					sort(Graph[i + ii], &Graph[i + ii][out_deg[i + ii]]); //������ڵ�ĳ���id��������һ�£�
				}
			}
		}

	}

	struct seq {
		int first;
		int second;
		seq(int f, int s) :first(f), second(s) {};
	};

	void find_circle_iterMethod() {
		ansNum = 0;

		unordered_map<int, vector<int>> info;
		unordered_map<int, vector<seq>> info2;

		for (int i = 0; i < nodesNum; i += Items) {
			for (int ii = 0; ii < Items && i + ii < nodesNum; ++ii) {
				int head = i + ii;
				if (Graph[head][0] == -1) continue;
				memset(last, 0, sizeof(bool)*nodesNum);

				//for (auto& iin : GraphIn[head]) {
				//	if (iin == -1) break;
				//	if (iin <= head) continue;
				//	last[iin] = 1;//iin����һ����head���
				//	for (auto& jin : GraphIn[iin]) {
				//		if (jin == -1) break;
				//		if (jin <= head) continue;
				//		info[jin].push_back(iin);
				//	}
				//}


				for (auto& iin : GraphIn[head]) {
					if (iin == -1) break;
					if (iin <= head) continue;
					last[iin] = 1;//iin����һ����head���
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

				//�����ǵ�һ��
				vertext_visit[head] = true;
				auto& nodesOut1 = Graph[head]; //nodesOuti�����i��ĳ��˽ڵ㼯��
				const auto nodesOut1_end = &nodesOut1[out_deg[head]];
				auto it1 = lower_bound(nodesOut1, nodesOut1_end, head); //iti�����nodesOuti����ͷ
				for (; it1 != nodesOut1_end; ++it1) {
					if (vertext_visit[*it1]) continue;
					//�����ǵڶ���
					vertext_visit[*it1] = true;
					auto& nodesOut2 = Graph[*it1];
					const auto nodesOut2_end = &nodesOut2[out_deg[*it1]];
					auto it2 = lower_bound(nodesOut2, nodesOut2_end, head);

					for (; it2 != nodesOut2_end; ++it2) {
						if (vertext_visit[*it2]) continue;
						if (it2 != nodesOut2_end && last[*it2] == 1) {//it2����һ����head��ֱ�Ӵ�·�����������������һ��ѭ��
							//ans3[i3++] = head; ans3[i3++] = *it1; ans3[i3++] = *it2;
							int tmp3[] = { head, *it1, *it2 };
							memcpy(ans3 + i3, tmp3, sizeof tmp3);
							i3 += 3;
							buff3 += buff_count[head]; 
							buff3 += buff_count[*it1]; 
							buff3 += buff_count[*it2];
							++ansNum;
						}
						//�����ǵ�����
						vertext_visit[*it2] = true;
						auto& nodesOut3 = Graph[*it2];
						const auto nodesOut3_end = &nodesOut3[out_deg[*it2]];
						auto it3 = lower_bound(nodesOut3, nodesOut3_end, head);

						for (; it3 != nodesOut3_end; ++it3) {
							if (vertext_visit[*it3]) continue;
							if (it3 != nodesOut3_end && last[*it3] == 1) {
								//ans4[i4++] = head; ans4[i4++] = *it1; ans4[i4++] = *it2; ans4[i4++] = *it3;
								int tmp4[] = { head, *it1, *it2, *it3 };
								memcpy(ans4 + i4, tmp4, sizeof tmp4);
								i4 += 4;
								buff4 += buff_count[head]; 
								buff4 += buff_count[*it1]; 
								buff4 += buff_count[*it2]; 
								buff4 += buff_count[*it3];
								++ansNum;
							}

							//�Ѿ�������Ĳ�
							vertext_visit[*it3] = true;
							auto& nodesOut4 = Graph[*it3];
							const auto nodesOut4_end = &nodesOut4[out_deg[*it3]];
							auto it4 = lower_bound(nodesOut4, nodesOut4_end, head);

							for (; it4 != nodesOut4_end; ++it4) {
								if (vertext_visit[*it4]) continue;
								if (it4 != nodesOut4_end && last[*it4] == 1) {
									//ans5[i5++] = head; ans5[i5++] = *it1; ans5[i5++] = *it2; ans5[i5++] = *it3; ans5[i5++] = *it4;
									int tmp5[] = { head, *it1, *it2, *it3, *it4 };
									memcpy(ans5 + i5, tmp5, sizeof tmp5);
									i5 += 5;

									buff5 += buff_count[head]; 
									buff5 += buff_count[*it1]; 
									buff5 += buff_count[*it2];
									buff5 += buff_count[*it3];
									buff5 += buff_count[*it4];
									++ansNum;
								}
								if (info.count(*it4) > 0) {
									for (auto& trail : info[*it4]) {
										if (vertext_visit[trail]) continue;
										//ans6[i6++] = head; ans6[i6++] = *it1; ans6[i6++] = *it2; ans6[i6++] = *it3; ans6[i6++] = *it4; ans6[i6++] =trail;
										int tmp6[] = { head, *it1, *it2, *it3, *it4, trail };
										memcpy(ans6 + i6, tmp6, sizeof tmp6);
										i6 += 6;
										buff6 += buff_count[head];
										buff6 += buff_count[*it1];
										buff6 += buff_count[*it2];
										buff6 += buff_count[*it3];
										buff6 += buff_count[*it4];
										buff6 += buff_count[trail];
										++ansNum;
									}
								}
								vertext_visit[*it4] = true;
								if (info2.count(*it4) > 0) {
									for (auto& trail : info2[*it4]) {
										if (vertext_visit[trail.first] || vertext_visit[trail.second]) continue;
										//ans7[i7++] = head; ans7[i7++] = *it1; ans7[i7++] = *it2; ans7[i7++] = *it3; ans7[i7++] = *it4; ans7[i7++] = trail.first; ans7[i7++] = trail.second;
										int tmp7[] = { head, *it1, *it2, *it3, *it4, trail.first, trail.second };
										memcpy(ans7 + i7, tmp7, sizeof tmp7);
										i7 += 7;
										buff7 += buff_count[head];
										buff7 += buff_count[*it1];
										buff7 += buff_count[*it2];
										buff7 += buff_count[*it3];
										buff7 += buff_count[*it4];
										buff7 += buff_count[trail.first];
										buff7 += buff_count[trail.second];
										++ansNum;
									}
								}
								////�Ѿ���������
								//vertext_visit[*it4] = true;
								//auto& nodesOut5 = Graph[*it4];
								//const auto nodesOut5_end = &nodesOut5[out_deg[*it4]];
								//auto it5 = lower_bound(nodesOut5, nodesOut5_end, head);

								//for (; it5 != nodesOut5_end; ++it5) {
								//	if (vertext_visit[*it5]) continue;
								//	if (it5 != nodesOut5_end && last[*it5] == 1) {
								//		ans6[i6++] = head; ans6[i6++] = *it1; ans6[i6++] = *it2; ans6[i6++] = *it3; ans6[i6++] = *it4; ans6[i6++] = *it5;
								//		++ansNum;
								//	}

								//	//�������ﻹ��Ҫ����һ�㣬���ڲ����ˡ�

								//	if (info.count(*it5) > 0) {
								//		for (auto& trail : info[*it5]) {
								//			if (vertext_visit[trail]) continue;
								//			ans7[i7++] = head; ans7[i7++] = *it1; ans7[i7++] = *it2; ans7[i7++] = *it3; ans7[i7++] = *it4; ans7[i7++] = *it5; ans7[i7++] = trail;
								//			++ansNum;
								//		}
								//	}
								//}
								vertext_visit[*it4] = false;
							}
							vertext_visit[*it3] = false;
						}
						vertext_visit[*it2] = false;
					}
					vertext_visit[*it1] = false;
				}
				vertext_visit[i + ii] = false;
				info.clear();
				info2.clear();
			}
		}
	}


	void save(const string& outputFile) {//���Ű汾
		/*
		ģʽ��
			O_RDWR  ��дģʽ
			O_CREAT ���ָ���ļ������ڣ��򴴽�����ļ� 
			O_TRUNC ����ļ����ڣ�������ֻд/��д��ʽ�򿪣�������ļ�ȫ������ 
		*/
		int fd = open(outputFile.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
		char* ansNumBuffer = new char[1024];
		int idx = sprintf(ansNumBuffer, "%d\n", ansNum);
		/*
		#include <unistd.h>
		ssize_t write(int filedes, const void *buf, size_t nbytes);
		����ֵ��д���ļ����ֽ������ɹ�����-1������
		write ������ filedes ��д�� nbytes �ֽ����ݣ�������ԴΪ buf ������ֵһ�����ǵ��� nbytes��
		������ǳ����ˡ������ĳ���ԭ���Ǵ��̿ռ����˻��߳������ļ���С���ơ�
		*/
		write(fd, ansNumBuffer, idx);

		int buffs[] = { 0,buff3,buff3 + buff4,buff3 + buff4 + buff5,buff3 + buff4 + buff5 + buff6,buff3 + buff4 + buff5 + buff6 + buff7 };
		int iii;
		pid_t pid = 1;
		for (int i = 3; i <= 7; i++) {
			if (pid>0) {//ȷ��ֻ�и������ܲ����ӽ��̣�����ᱬը
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
				int buf_size = buffs[iii + 1] - buffs[iii];//ÿ���ӽ��̵�buf�Ĵ�С
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
				lseek(fd, idx + buffs[iii], SEEK_SET);//�ƶ���д��λ��
				write(fd, buf, id);
				exit(0);
			}
		}

	}


};




int main()
{
	/*string testFile = "data/test_data_very_big.txt";
	string outputFile = "result_very_big_0415.txt";*/

	//string testFile = "test_data_very_big.txt";
	//string outputFile = "result_very_big2.txt";

	//string testFile = "naive6.txt";
	//string outputFile = "result_naive6.txt";

	//string testFile = "test_data_new_v2.txt";
	//string outputFile = "result_3.txt";

	//string testFile = "/data/test_data.txt";
	//string outputFile = "/projects/student/result.txt";

	string testFile = "/root/data/test_data_big.txt";
	string outputFile = "/root/projects/student/result_big_v16.txt";

	auto t = clock();
	Sol sol;
	auto t1 = clock();
	sol.parseInput(testFile);
	auto t2 = clock();
	sol.construct_Graph();
	auto t2_2 = clock();
	sol.topoSort(sol.in_deg, true);
	auto t2_3 = clock();
	sol.find_circle_iterMethod();
	auto t3 = clock();
	sol.save(outputFile);
	auto t4 = clock();
	cout << "reading data used time: " << (double)(t2 - t1) / CLOCKS_PER_SEC << "s" << endl;
	cout << "Construct graph used time: " << (double)(t2_2 - t2) / CLOCKS_PER_SEC << "s" << endl;
	cout << "tuopu sort used time: " << (double)(t2_2 - t2_2) / CLOCKS_PER_SEC << "s" << endl;
	cout << "find circle used time: " << (double)(t3 - t2_3) / CLOCKS_PER_SEC << "s" << endl;
	cout << "save time: " << (double)(t4 - t3) / CLOCKS_PER_SEC << " s" << endl;
	cout << "all used time: " << (double)(t4 - t) / CLOCKS_PER_SEC << " s" << endl;

	//system("pause");
	return 0;
}
