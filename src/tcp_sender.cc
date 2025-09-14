#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return bytes_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return consecutive_retx_;
}

void TCPSender::push(const TransmitFunction &transmit) {
    // 1. 先发送 SYN，如果还没发
    if (!syn_flag_) {
        TCPSenderMessage syn_msg;
        syn_msg.seqno = Wrap32::wrap(next_abs_seqno_, isn_);
        syn_msg.SYN = true;
        syn_flag_ = true;
        next_abs_seqno_ += 1;
        bytes_in_flight_ += 1;
        syn_msg.RST = input_.reader().has_error();
        // ByteStream 已结束且窗口允许，可 piggyback FIN
        if (!fin_flag_ && input_.reader().is_finished() && windows_size_ > 0) {
            syn_msg.FIN = true;
            fin_flag_ = true;
            next_abs_seqno_ += 1;
            bytes_in_flight_ += 1;
        }
        
        outstanding_segments_.push(syn_msg);
        transmit(syn_msg);

        timer_running_ = true;
        time_since_last_transmission_ = 0;
        return;
    }

    // 2. 填充窗口发送数据
    while ((windows_size_ == 0 ? 1 : windows_size_) > bytes_in_flight_ &&
           (input_.reader().bytes_buffered() > 0)) {

        uint64_t effective_window = windows_size_ > bytes_in_flight_ ? windows_size_ - bytes_in_flight_ : 1;
        uint64_t payload_len = min({effective_window, TCPConfig::MAX_PAYLOAD_SIZE, input_.reader().bytes_buffered()});
        if (payload_len == 0) break;

        TCPSenderMessage msg;
        msg.seqno = Wrap32::wrap(next_abs_seqno_, isn_);
        msg.payload = input_.reader().peek().substr(0, payload_len);
        input_.reader().pop(payload_len);
        if (input_.reader().is_finished() && !fin_flag_ &&
            (effective_window > payload_len)) {
            msg.FIN = true;
            fin_flag_ = true;
            next_abs_seqno_ += 1;
            bytes_in_flight_ += 1;
        }
        if (input_.reader().has_error()) {
            msg.RST = true;
        }
        next_abs_seqno_ += payload_len;
        bytes_in_flight_ += payload_len;
        outstanding_segments_.push(msg);
        transmit(msg);
        if (fin_flag_) break;
    }

    // 3. 仅在窗口允许且 ByteStream 已结束时发送 FIN
    if (!fin_flag_ && input_.reader().is_finished()) {
        uint64_t effective_window = windows_size_ > bytes_in_flight_ ? windows_size_ - bytes_in_flight_ : 0;
        if (effective_window > 0) {
            TCPSenderMessage fin_msg;
            fin_msg.seqno = Wrap32::wrap(next_abs_seqno_, isn_);
            fin_msg.FIN = true;
            outstanding_segments_.push(fin_msg);
            next_abs_seqno_ += 1;
            bytes_in_flight_ += 1;
            fin_flag_ = true;
            transmit(fin_msg);
        }
    }

    // 4. 启动计时器，如果有未确认段
    if (!timer_running_ && !outstanding_segments_.empty()) {
        timer_running_ = true;
        time_since_last_transmission_ = 0;
    }
}


TCPSenderMessage TCPSender::make_empty_message() const {
    TCPSenderMessage msg;
    msg.seqno = Wrap32::wrap(next_abs_seqno_, isn_);
    msg.RST = input_.reader().has_error();  // 当流出错时设置RST标志
    return msg;
}

void TCPSender::receive(const TCPReceiverMessage& msg) {
    if (msg.RST) {
        // 连接被重置，进入错误状态
        // Your code here.
        input_.reader().set_error();
        return;
    }
    if (!msg.ackno.has_value()) {
        return;
    }

    uint64_t ackno_abs = msg.ackno->unwrap(isn_, next_abs_seqno_);
    if (ackno_abs > next_abs_seqno_) {
        return;
    }

    // 移除已确认的段（原有逻辑不变）
    uint64_t old_flight = bytes_in_flight_;
    while (!outstanding_segments_.empty()) {
        TCPSenderMessage &front = outstanding_segments_.front();
        uint64_t seg_first = front.seqno.unwrap(isn_, next_abs_seqno_);
        uint64_t seg_end = seg_first + front.sequence_length();
        if (ackno_abs >= seg_end) {
            bytes_in_flight_ -= front.sequence_length();
            outstanding_segments_.pop();
        } else {
            break;
        }
    }

    // 关键修改：记录原始窗口大小，同时更新转换后的窗口大小
    original_window_size_ = msg.window_size; // 存储接收方的原始窗口（未转换）
    // 转换窗口大小：0→1，非零保持不变（原有逻辑）
    windows_size_ = (original_window_size_ == 0) ? 1 : original_window_size_;

    // 重置计时器（原有逻辑不变）
    if (bytes_in_flight_ < old_flight) {
        consecutive_retx_ = 0;
        RTO_ = initial_RTO_ms_;
        time_since_last_transmission_ = 0;
        timer_running_ = !outstanding_segments_.empty();
    }
}


void TCPSender::tick(uint64_t ms_since_last_tick, const TransmitFunction& transmit) {
    if (outstanding_segments_.empty()) {
        timer_running_ = false;
        return;
    }

    time_since_last_transmission_ += ms_since_last_tick;

    if (time_since_last_transmission_ >= RTO_) {
        // 重传最早未确认的段（原有逻辑不变）
        TCPSenderMessage &seg = outstanding_segments_.front();
        transmit(seg);
        time_since_last_transmission_ = 0;

        // 关键修改：准确判断零窗口探测包
        // 条件：1. 原始窗口为 0（接收方实际窗口是 0，当前是探测）；2. 段长度为 1；3. 非 SYN 段
        bool zero_window_probe = (original_window_size_ == 0) && 
                                 (seg.sequence_length() == 1) && 
                                 (!seg.SYN);

        // 非探测包才需要翻倍 RTO 和增加重传计数（原有逻辑不变）
        if (!zero_window_probe) {
            consecutive_retx_ += 1;
            RTO_ *= 2;
        }
    }
}