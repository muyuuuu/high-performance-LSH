#pragma once

#include <queue>
#include <vector>
#include <mutex>

// 相似度，对应的编号
using PDI = std::pair<double, int>;

class SafeRes {
private:
    // 每个查询结果都放到优先级队列中
	std::vector<std::priority_queue<PDI> > q;
	std::mutex mtx;
public:
	SafeRes()=default;

	void resize(const int& n) {
		this->q.resize(n);
	}

	int size(const int& line) {
		std::lock_guard<std::mutex> l{mtx};
		return q[line].size();
	}

	void emplace(const int& line, const double& dis, const int& index) {
		std::lock_guard<std::mutex> l{mtx};
		q[line].emplace(dis, index);
	}

	PDI top(const int& line) {
		std::lock_guard<std::mutex> l{mtx};
		return q[line].top();
	}

	void pop(const int& line) {
		std::lock_guard<std::mutex> l{mtx};
		q[line].pop();
	}

	bool empty(const int& line) {
		std::lock_guard<std::mutex> l{mtx};
		return q[line].empty();
	}
};