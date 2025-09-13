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
  return {};
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  if (!syn_flag_) {
    TCPSenderMessage msg;
    msg.SYN = true;
    msg.seqno = Wrap32::wrap(next_abs_seqno_, isn_);
    transmit(msg);
    outstanding_segments.push(msg);
    next_abs_seqno_ += 1;
    bytes_in_flight_ += 1;
    syn_flag_ = true;
  } 

}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap(next_abs_seqno_, isn_);
  return msg;
  
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  // bytes_in_flight_--;
  (void)msg;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.

  (void)ms_since_last_tick;
  (void)transmit;
  (void)initial_RTO_ms_;
}
