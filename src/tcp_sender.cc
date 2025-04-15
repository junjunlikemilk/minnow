#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // debug( "unimplemented sequence_numbers_in_flight() called" );
  // return {};
  return in_flight_cnt_;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  // debug( "unimplemented consecutive_retransmissions() called" );
  // return {};
  return retrans_cnt_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // debug( "unimplemented push() called" );
  // (void)transmit;
  bool window_zero = ( window_size_ == 0 );
  size_t avaviable_window
    = ( window_size_ + window_zero ) < in_flight_cnt_ ? 0 : window_size_ + window_zero - in_flight_cnt_;
  do {
    /* code */

    if ( is_fin_send_ )
      return;
    size_t payload_size = min( reader().bytes_buffered(), TCPConfig::MAX_PAYLOAD_SIZE );
    size_t seq_size = min( avaviable_window, payload_size + ( current_seq_ == 0 ) );
    payload_size = seq_size;
    TCPSenderMessage msg = TCPSenderMessage();
    if ( current_seq_ == 0 ) {
      msg.SYN = true;
      payload_size--;
    }
    if ( reader().has_error() ) {

      msg.RST = true;
    }
    while ( msg.payload.size() < payload_size ) {

      string_view front = reader().peek();
      size_t bytes_to_read = min( front.size(), payload_size - msg.payload.size() );
      msg.payload += front.substr( 0, bytes_to_read );
      reader().pop( bytes_to_read );
    }
    if ( reader().is_finished() && seq_size < avaviable_window ) {
      msg.FIN = true;
      seq_size++;
      is_fin_send_ = true;
    }
    if ( msg.sequence_length() == 0 )
      return;
    msg.seqno = Wrap32::wrap( current_seq_, isn_ );
    current_seq_ += msg.sequence_length();
    in_flight_cnt_ += msg.sequence_length();
    outstanding_msg_.push_back( msg );
    transmit( msg );
    if ( expire_time_ == UINT64_MAX ) {
      expire_time_ = current_time_ + rto_;
    }

    avaviable_window
      = ( window_size_ + window_zero ) < in_flight_cnt_ ? 0 : window_size_ + window_zero - in_flight_cnt_;
  } while ( reader().bytes_buffered() != 0 && avaviable_window != 0 );
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // debug( "unimplemented make_empty_message() called" );
  // return {};

  return { Wrap32::wrap( current_seq_, isn_ ), false, string(), false, reader().has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // debug( "unimplemented receive() called" );
  // (void)msg;
  if ( msg.ackno.has_value() ) {
    size_t ack_from_recv = msg.ackno.value().unwrap( isn_, ack_ );
    
    // size_t ack_from_recv= unwrap(msg.ackno.value());
    if ( ack_from_recv > ack_ && ack_from_recv <= current_seq_ ) {
      ack_ = ack_from_recv;
      rto_ = initial_RTO_ms_;
      expire_time_ = rto_ + current_time_;
      retrans_cnt_ = 0;
      while ( !outstanding_msg_.empty() ) {
        auto& front_msg = outstanding_msg_.front();
        if ( front_msg.seqno.unwrap( isn_, ack_ ) + front_msg.sequence_length() > ack_ ) {
          break;
        }
        in_flight_cnt_ -= front_msg.sequence_length();
        outstanding_msg_.pop_front();
      }
      if ( outstanding_msg_.empty() ) {
        expire_time_ = UINT64_MAX;
      }
    }
  }
  window_size_ = msg.window_size;
  if ( msg.RST ) {
    writer().set_error();
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // debug( "unimplemented tick({}, ...) called", ms_since_last_tick );
  // (void)transmit;
  current_time_ += ms_since_last_tick;
  if ( !outstanding_msg_.empty()&&expire_time_ != 0 && current_time_ >= expire_time_ ) {
    transmit( outstanding_msg_.front() );
    if ( window_size_ != 0 ) {
      retrans_cnt_++;
      rto_ *= 2;
    }
    expire_time_ = current_time_ + rto_;
  }
}

// uint64_t TCPSender::unwarp( const Wrap32& seq )
// {
//   return seq.unwrap( isn_, ack_ );
// }