#include "ZeroCopyNetworkReaderStream.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

ZeroCopyNetworkReaderStream::ZeroCopyNetworkReaderStream(
  int fd,
  protobuf::uint32 totalMessageSize,
  protobuf::uint8* pDataBuffer,
  protobuf::uint32 bufferSize) :
  m_fd(fd),
  m_remainingMessageSizeBytes(totalMessageSize),
  m_pDataBuffer(pDataBuffer),
  m_bufferSize(bufferSize),
  m_numBytesLastRead(0),
  m_backupPos(m_numBytesLastRead),
  m_byteCount(0)
{
}

//--------------------------------------------------------------------------------------------------

ZeroCopyNetworkReaderStream::~ZeroCopyNetworkReaderStream()
{
}

//--------------------------------------------------------------------------------------------------

bool ZeroCopyNetworkReaderStream::Next(const void** data, int* size) {
  if (data == 0 || size == 0) {
    return false;
  }

  *size = 0;

  if (m_remainingMessageSizeBytes == 0) {
    return false;
  }

  if (m_backupPos < m_numBytesLastRead) {
    // A BackUp() call has been issued
    *size = m_numBytesLastRead - m_backupPos;
    *data = &m_pDataBuffer[m_backupPos];
    // We've returned everything from the backup position to the end of the last read
    m_backupPos = m_numBytesLastRead;
    return true;
  }

  const protobuf::uint32 numBytesToRead =
    std::min(m_remainingMessageSizeBytes, m_bufferSize);

  ssize_t bytesRead = -1;
  do {
    bytesRead = recv(m_fd, m_pDataBuffer, numBytesToRead, MSG_WAITALL);
  } while (bytesRead == -1 && errno == EINTR);
  if (bytesRead == 0) {
    //pan::log(pan::notice, "Peer has performed an orderly shutdown on fd: ", pan::i(m_fd),
    //  " during normal receive");
    return false;
  }
  if (bytesRead == -1) {
    //pan::log(pan::error, "Failed to read message, numBytesToRead: ", pan::i(numBytesToRead),
    //  ", m_remainingMessageSizeBytes: ", pan::i(m_remainingMessageSizeBytes), ", m_fd: ",
    //  pan::i(m_fd), ". ", ErrnoInserter());
    return false;
  }
  // TODO: for non-blocking I/O sockets the following test isn't correct
  if (bytesRead != numBytesToRead) {
    //pan::log(pan::error, "Partially read message, bytesRead: ", pan::i(bytesRead),
    //  ", numBytesToRead: ", pan::i(numBytesToRead), ", m_remainingMessageSizeBytes: ",
    //  pan::i(m_remainingMessageSizeBytes), ". ", ErrnoInserter());
    return false;
  }

  *data = m_pDataBuffer;
  *size = bytesRead;
  m_numBytesLastRead = m_backupPos = bytesRead;
  m_remainingMessageSizeBytes -= bytesRead;
  m_byteCount += bytesRead;

  return true;
}

//--------------------------------------------------------------------------------------------------

void ZeroCopyNetworkReaderStream::BackUp(int count) {
  m_backupPos -= count;
}

//--------------------------------------------------------------------------------------------------

bool ZeroCopyNetworkReaderStream::Skip(int count) {
  if (m_backupPos < m_numBytesLastRead) {
    const protobuf::uint32 numBytesToSkipInBackup =
      std::min(m_numBytesLastRead - m_backupPos, (protobuf::uint32) count);
    m_backupPos += numBytesToSkipInBackup;
    count -= numBytesToSkipInBackup;
  }

  if (count == 0) {
    return (m_remainingMessageSizeBytes > 0);
  }

  // Read the remaining <count> bytes from the stream and dump them
  // TODO: can we simply seek() instead of actually reading the data?
  char dumpBuf[128];
  protobuf::uint32 bytesToDump = std::min(m_remainingMessageSizeBytes, (protobuf::uint32) count);
  while (bytesToDump > 0) {
    protobuf::uint32 bytesToDumpThisIteration =
      std::min(bytesToDump, (protobuf::uint32) sizeof(dumpBuf));
    ssize_t bytesRead = -1;
    do {
      bytesRead = recv(m_fd, dumpBuf, bytesToDumpThisIteration, MSG_WAITALL);
    } while (bytesRead == -1 && errno == EINTR);
    if (bytesRead == 0) {
      //pan::log(pan::notice, "Peer has performed an orderly shutdown on m_fd ", pan::i(m_fd),
      //  " during data dump");
      return false;
    }
    if (bytesRead == -1) {
      //pan::log(pan::error, "Failed to read data to dump, bytesToDumpThisIteration: ",
      //  pan::i(bytesToDumpThisIteration), ". ", ErrnoInserter());
      return false;
    }

    // TODO: proper error handling of read()
    bytesToDump -= bytesToDumpThisIteration;
    m_remainingMessageSizeBytes -= bytesToDumpThisIteration;
    m_byteCount += bytesToDumpThisIteration;
  }

  return (m_remainingMessageSizeBytes > 0);
}

//--------------------------------------------------------------------------------------------------

protobuf::int64 ZeroCopyNetworkReaderStream::ByteCount() const {
  return m_byteCount;
}
