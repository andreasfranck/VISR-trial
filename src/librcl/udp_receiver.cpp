/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "udp_receiver.hpp"

#include <libpml/empty_parameter_config.hpp>

#include <boost/asio/placeholders.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/lock_types.hpp>

#include <ciso646>
#include <memory>
#include <sstream>
#include <vector>

namespace visr
{
namespace rcl
{

  UdpReceiver::UdpReceiver( SignalFlowContext const & context,
                            char const * name,
                            CompositeComponent * parent,
                            std::size_t port,
                            Mode mode)
 : AtomicComponent( context, name, parent )
 , mMode( mode )
 , mDatagramOutput( "messageOutput", *this, pml::EmptyParameterConfig() )
{
    using boost::asio::ip::udp;
    mIoServiceInstance.reset(new boost::asio::io_service());
    mIoService = mIoServiceInstance.get();

    if (mMode == Mode::Synchronous)
    {
        mIoServiceWork.reset();
    }
    else
    {
        mIoServiceWork.reset(new  boost::asio::io_service::work(*mIoService));
    }
    mSocket.reset(new udp::socket(*mIoService));
    boost::system::error_code ec;
    mSocket->open(udp::v4(), ec);
    mSocket->set_option(boost::asio::socket_base::reuse_address(true));
    mSocket->bind(udp::endpoint(udp::v4(), static_cast<unsigned short>(port)));

    if (ec)
    {
        throw std::runtime_error("Error opening UDP port");
    }

    mSocket->async_receive_from(boost::asio::buffer(mReceiveBuffer),
        mRemoteEndpoint,
        boost::bind(&UdpReceiver::handleReceiveData, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)
    );
    if (mMode == Mode::Asynchronous)
    {
        mServiceThread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, mIoService)));
    }
}

UdpReceiver::~UdpReceiver()
{
  if( mIoServiceInstance.get() != nullptr )
  {
    mIoServiceInstance->stop();
  }
  if( mServiceThread.get() != nullptr  )
  {
    mServiceThread->join();
  }
}

void UdpReceiver::process()
{
  if(  mMode == Mode::Synchronous )
  {
    mIoService->poll();
  }
  boost::lock_guard<boost::mutex> lock( mMutex );
  while( not mInternalMessageBuffer.empty() )
  {
    pml::StringParameter const & nextMsg = mInternalMessageBuffer.front();
    mDatagramOutput.enqueue( nextMsg  );
    mInternalMessageBuffer.pop_front();
  }
}

void UdpReceiver::handleReceiveData( const boost::system::error_code& error,
                                     std::size_t numBytesTransferred )
{
  {
    boost::lock_guard<boost::mutex> lock( mMutex );
    mInternalMessageBuffer.push_back( pml::StringParameter( std::string( &mReceiveBuffer[0], numBytesTransferred ) ) );
  }
  mSocket->async_receive_from( boost::asio::buffer(mReceiveBuffer),
                               mRemoteEndpoint,
                               boost::bind(&UdpReceiver::handleReceiveData, this,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred)
                             );
}

} // namespace rcl
} // namespace visr
