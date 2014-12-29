#include "ZeroCopyNetworkWriterStream.h"

#include <sys/types.h>
#include <sys/socket.h>

ZeroCopyNetworkWriterStream::ZeroCopyNetworkWriterStream(
  int fd,
  protobuf::uint8* pDataBuffer,
  protobuf::uint32 bufferSize) :
  m_fd(fd),
  m_pDataBuffer(pDataBuffer),
  m_bufferSize(bufferSize),
  m_numBytesToWrite(0),
  m_byteCount(0)
{
}

//--------------------------------------------------------------------------------------------------

ZeroCopyNetworkWriterStream::~ZeroCopyNetworkWriterStream()
{
}

//--------------------------------------------------------------------------------------------------

bool ZeroCopyNetworkWriterStream::Next(void** data, int* size) {
  if (m_numBytesToWrite == m_bufferSize) {
    bool bret = Flush();
    if (!bret) {

      return false;
    }

    return true;
  }

  *size = m_bufferSize - m_numBytesToWrite;
  *data = &m_pDataBuffer[m_numBytesToWrite];
  m_numBytesToWrite += m_bufferSize - m_numBytesToWrite;

  return true;
}

//--------------------------------------------------------------------------------------------------

void ZeroCopyNetworkWriterStream::BackUp(int count) {
  m_numBytesToWrite -= count;
}

//--------------------------------------------------------------------------------------------------

protobuf::int64 ZeroCopyNetworkWriterStream::ByteCount() const {
  return m_byteCount;
}

//--------------------------------------------------------------------------------------------------

bool ZeroCopyNetworkWriterStream::Flush() {
  ssize_t writtenBytes = -1;
  do {
    writtenBytes = send(m_fd, m_pDataBuffer, m_numBytesToWrite, 0);
  } while (writtenBytes == -1 && errno == EINTR);
  if (writtenBytes == 0) {
    //pan::log(pan::notice, "Peer has performed an orderly shutdown on m_fd ", pan::i(m_fd),
    //  " during data flushing");
    return false;
  }
  if (writtenBytes == -1) {
    //pan::log(pan::error, "Failed to write bytes to m_fd: ", pan::i(m_fd), ", m_numBytesToWrite",
    //    pan::i(m_numBytesToWrite), ". ", ErrnoInserter());
    return false;
  }
  if (writtenBytes != m_numBytesToWrite) {
    //pan::log(pan::error, "Unexpected error: sending seems to have succeeded but writtenBytes: ",
    //    pan::i(writtenBytes), ", m_numBytesToWrite:", pan::i(m_numBytesToWrite));
    return false;
  }

  m_byteCount += writtenBytes;

  return true;
}
