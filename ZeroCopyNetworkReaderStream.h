#ifndef __ZERO_COPY_NETWORK_READER_STREAM_H__
#define __ZERO_COPY_NETWORK_READER_STREAM_H__

#include <google/protobuf/io/zero_copy_stream.h>

namespace protobuf = google::protobuf;

class ZeroCopyNetworkReaderStream : public protobuf::io::ZeroCopyInputStream {
public:
  ZeroCopyNetworkReaderStream(
    int fd,
    protobuf::uint32 totalMessageSize,
    protobuf::uint8* pDataBuffer,
    protobuf::uint32 bufferSize);

  virtual ~ZeroCopyNetworkReaderStream();

  virtual bool Next(const void** data, int* size);

  virtual void BackUp(int count);

  virtual bool Skip(int count);

  virtual protobuf::int64 ByteCount() const;

private:
  int m_fd;
  protobuf::uint32 m_remainingMessageSizeBytes;
  protobuf::uint8* m_pDataBuffer;
  const protobuf::uint32 m_bufferSize;
  protobuf::uint32 m_numBytesLastRead;
  protobuf::uint32 m_backupPos;
  protobuf::uint32 m_byteCount;

private:
 GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ZeroCopyNetworkReaderStream);
};

#endif // __ZERO_COPY_NETWORK_READER_STREAM_H__
