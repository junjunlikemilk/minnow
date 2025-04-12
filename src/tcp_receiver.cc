#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  // debug( "unimplemented receive() called" );
  // (void)message;
  const size_t check_point=reassembler_.writer().bytes_pushed()+ISN_.has_value();
  if(message.RST)
  {
    reassembler_.reader().set_error();
  }else if(check_point>0 &&check_point<UINT32_MAX&&message.seqno==ISN_)
  {
    return;
  }
  if(!ISN_.has_value())
  {
    if(!message.SYN)
    {
      return ;
    }
    ISN_=message.seqno;
  }
  const size_t abs_seqno=message.seqno.unwrap(*ISN_,check_point);
  reassembler_.insert(abs_seqno == 0 ? abs_seqno : abs_seqno - 1,std::move(message.payload),message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  // debug( "unimplemented send() called" );
  // return {};

  const size_t check_point=reassembler_.writer().bytes_pushed()+ISN_.has_value();
  const size_t capacity=reassembler_.writer().available_capacity();
  const uint16_t window_size=capacity>UINT16_MAX?UINT16_MAX:capacity;
  if(!ISN_.has_value())
  {
    return {{},window_size,reassembler_.writer().has_error()};
  }
  return { Wrap32::wrap( check_point + reassembler_.writer().is_closed(), *ISN_ ),
    window_size,
    reassembler_.writer().has_error() };
}
