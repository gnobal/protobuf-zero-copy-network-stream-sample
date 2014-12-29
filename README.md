protobuf-zero-copy-network-stream-sample
========================================

This is a sample of a ZeroCopyInputStream and a ZeroCopyOutputStream to handle a streaming socket using a pre-allocated buffer. This is useful if you have a buffer pool or wish to use an on-stack buffer.

Notes
-----
1. Logging code is commented out. You better uncomment and use your own logging framework to catch errors.
2. This code was used in a production server, but currently doesn't have any associated tests. It is provided as is and wihtout any guarantee.
