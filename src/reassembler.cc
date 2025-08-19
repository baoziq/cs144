#include "reassembler.hh"
#include <map>
#include <string>
#include <algorithm>

using namespace std;

void Reassembler::insert(uint64_t first_index, string data, bool is_last_substring) {
    // 标记是否是最后一个片段
    if (is_last_substring) {
        eof_ = true;
        last_index_ = first_index + data.size();
    }
    if (expect_index_ == last_index_ && eof_) {
        output_.writer().close();
        return;
    }
    uint64_t end = expect_index_ + output_.writer().available_capacity();
    if (first_index >= end || first_index + data.size() <= expect_index_) return;

    // 截断已经写过的部分
    if (first_index < expect_index_) {
        data = data.substr(expect_index_ - first_index);
        first_index = expect_index_;
    }

    if (first_index + data.size() > end) {
        data = data.substr(0, end - first_index);
    }

    // 使用 map 的有序特性合并重叠区间
    auto it = buffer_.lower_bound(first_index);
    if (it != buffer_.begin()) --it;

    while (it != buffer_.end()) {
        uint64_t seg_start = it->first;
        uint64_t seg_end = seg_start + it->second.size();

        if (seg_end < first_index) {
            ++it;
            continue; // 无重叠
        }
        if (seg_start > first_index + data.size()) {
            break; // 后续无重叠
        }

        // 计算重叠区域并合并
        uint64_t new_start = min(first_index, seg_start);
        uint64_t new_end = max(first_index + data.size(), seg_end);
        string merged(new_end - new_start, '\0');

        // 先拷贝现有段
        copy(it->second.begin(), it->second.end(), merged.begin() + (seg_start - new_start));
        // 再拷贝新数据
        copy(data.begin(), data.end(), merged.begin() + (first_index - new_start));

        first_index = new_start;
        data = move(merged);

        // 删除旧段
        it = buffer_.erase(it);
    }

    buffer_[first_index] = move(data);

    // 推送连续可写段
    while (!buffer_.empty()) {
        it = buffer_.find(expect_index_);
        if (it == buffer_.end()) break;

        output_.writer().push(it->second);
        expect_index_ += it->second.size();
        buffer_.erase(it);
    }

    // 如果到达最后一个字节，关闭输出
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
