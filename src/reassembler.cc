#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring ) {
	// 判断是否是最后一个字符串
	if (is_last_substring) {
		eof_ = true;
		last_index_ = first_index + data.size();
	}
	uint64_t end = expect_index_ + output_.writer().available_capacity();

	if (first_index + data.size() < expect_index_ || first_index > end) {
		return;
	}
	if (first_index < expect_index_ && first_index + data.size() > expect_index_) {
		data = data.substr(expect_index_ - first_index);
		first_index = expect_index_;
	}
	if (first_index + data.size() > end) {
		data = data.substr(0, end - first_index);
	}

	for (const auto& pair : buffer_) {
		// 已经存在，无需插入
		if (first_index >= pair.first && first_index + data.size() <= pair.first + pair.second.size()) {
			data = "";
		}
		if (first_index <= pair.first && first_index + data.size() <= pair.first + pair.second.size() && first_index + data.size() >= pair.first) {
			data = data.substr(0, pair.first - first_index);
		}
		if (first_index >= pair.first && first_index <= pair.first + pair.second.size() && first_index + data.size() >= pair.first + pair.second.size()) {
			data = data.substr(pair.first + pair.second.size() - first_index, first_index + data.size() - pair.first - pair.second.size());
			first_index = pair.first + pair.second.size();

		}
		if (first_index <= pair.first && first_index + data.size() >= pair.first + pair.second.size()) {
			add_[first_index] = data.substr(0, pair.first - first_index);
			data = data.substr(pair.first + pair.second.size() - first_index, first_index + data.size() - pair.first - pair.second.size());
			first_index = pair.first + pair.second.size();

		}
	}
	for (const auto& pair : add_) {
		buffer_[pair.first] = pair.second;
	}
	if (!data.empty()) {
		buffer_[first_index] = data;
	}
	for (const auto& pair : buffer_) {
		if (pair.first == expect_index_ && end > 0) {
			output_.writer().push(pair.second);
			delete_.push_back(pair.first);
			expect_index_ += pair.second.size();
		}
	}
	for (auto index : delete_) {
		buffer_.erase(index);
	}
	add_.clear();
	delete_.clear();
	if (eof_ && expect_index_ == last_index_) {
		output_.writer().close();
	}

}

uint64_t Reassembler::bytes_pending() const {
	uint64_t cnt = 0;
	for (const auto& pair : buffer_) {
		cnt += pair.second.size();
	}
	return cnt;
}
