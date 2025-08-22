#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if (message.RST) {
    reader().set_error();
    return;
  }
  if (message.SYN) {
    zero_point_ = message.seqno;
    start_ = true;
  }
  if (start_) {
    checkpoint_ = writer().bytes_pushed();
    string data = message.payload;
    int64_t first_index = message.seqno.unwrap(zero_point_, checkpoint_);
    first_index -= 1;
    if (message.SYN) {
      first_index = 0;
    }
    reassembler_.insert(first_index, data, message.FIN);
  }

  
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage msg;
  if (!start_) {
    msg.ackno = nullopt;
    msg.window_size = writer().available_capacity() > 65535 ? 65535 : writer().available_capacity();
    msg.RST = writer().has_error();
    return msg;
  }
  msg.window_size = writer().available_capacity();
  if (!start_) {
    msg.ackno = nullopt;
  } else {
    uint64_t ack_abs = writer().bytes_pushed() + 1;
    if (writer().is_closed()) {
      ack_abs += 1;
    }
    msg.ackno = Wrap32::wrap(ack_abs, zero_point_);
  }
  msg.RST = writer().has_error();
  return msg;

}
