#include "Buffer.h"
#include "Endian.h"
#include "base64.h"
#include "sha1.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace frame {
static const uint8_t FIN = 0x80;
static const uint8_t RSV1 = 0x40;
static const uint8_t RSV2 = 0x20;
static const uint8_t RSV3 = 0x10;
static const uint8_t OPCODE = 0x0F;
static const uint8_t MASK = 0x80;
static const uint8_t PAYLOAD_LEN = 0X7F;

namespace opcode {
static const uint8_t CONTINUATION = 0x0;
static const uint8_t TEXT = 0x1;
static const uint8_t BINARY = 0x2;
static const uint8_t CLOSE = 0x8;
static const uint8_t PING = 0x9;
static const uint8_t PONG = 0xA;

inline bool reserved(uint8_t opcode) {
  return (opcode >= 0x3 and opcode <= 0x7) or (opcode >= 0xB and opcode <= 0xF);
}

// 是否为控制帧
inline bool control(uint8_t opcode) { return opcode >= CLOSE; }
} // namespace opcode

// Minimum length of a WebSocket frame header
static const unsigned int BASIC_HEADER_LENGTH = 2;

static const unsigned int EXTENED_LENGTH_0BIT = 0;
static const unsigned int EXTENED_LENGTH_16BIT = 2;
static const unsigned int EXTENED_LENGTH_64BIT = 8;

static const uint8_t PAYLOAD_SIZE_BASIC = 0X7D;
static const uint8_t PAYLOAD_SIZE_16BIT = 0X7E;
static const uint8_t PAYLOAD_SIZE_64BIT = 0X7F;

static const unsigned int MAX_PAYLOAD_LENGTH = (2 << 21);

static const uint8_t MASK_BYTES = 4;

} // namespace frame

static const char HANDSHAKE_MAGIC[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

union uint16_converter {
  uint16_t i;
  uint8_t c[2];
};

union uint32_converter {
  uint32_t i;
  uint8_t c[4];
};

union uint64_converter {
  uint64_t i;
  uint8_t c[8];
};

class WebSocketData {
public:
  void setBasicHeader(uint8_t byte1, uint8_t byte2) {
    basic_header_byte1_ = byte1;
    basic_header_byte2_ = byte2;
  }

  const char *basicHeaderByte1() const {
    return reinterpret_cast<const char *>(&basic_header_byte1_);
  }
  const char *basicHeaderByte2() const {
    return reinterpret_cast<const char *>(&basic_header_byte2_);
  }

  void setOpcode(uint8_t opcode) {
    basic_header_byte1_ =
        ((basic_header_byte1_ >> 4) << 4) | ((opcode << 4) >> 4);
  }
  uint8_t opcode() const { return (basic_header_byte1_ & frame::OPCODE); }

  void setFin() { basic_header_byte1_ |= frame::FIN; }
  bool fin() const { return (basic_header_byte1_ & frame::FIN); }

  void setMask() { basic_header_byte2_ |= frame::MASK; }
  bool mask() const { return (basic_header_byte2_ & frame::MASK); }

  void setExtendedPayloadLength(uint8_t len) { extendedPayloadLength_ = len; }
  uint8_t extendedPayloadLength() const { return extendedPayloadLength_; }

  void getEXtendedPayloadLengthInfo(char *out) const {
    if (extendedPayloadLength_ == frame::EXTENED_LENGTH_16BIT) {
      uint16_converter len;
      len.i = hton16(static_cast<uint16_t>(payloadLen_));
      std::copy(len.c, len.c + 2, out);
    } else if (extendedPayloadLength_ == frame::EXTENED_LENGTH_64BIT) {
      uint64_converter len;
      len.i = hton64(static_cast<uint64_t>(payloadLen_));
      std::copy(len.c, len.c + 8, out);
    }
  }

  // 7 位 payload len
  uint8_t payloadSize() const {
    return (basic_header_byte2_ & frame::PAYLOAD_LEN);
  }
  void setPayloadSize(uint8_t size) { basic_header_byte2_ |= size; }

  // 真实 payload 长度
  void setPayloadLen(unsigned int len) { payloadLen_ = len; }
  unsigned int payloadLen() const { return payloadLen_; }

  void setMaskKey(const uint8_t *begin) {
    setMask();
    std::copy(begin, begin + frame::MASK_BYTES, maskKey_.c);
  }
  void getMaskKey(char *out) const {
    std::copy(maskKey_.c, maskKey_.c + 4, out);
  }

  void handlePayload(const char *begin, size_t len) {
    payload_.ensureWritableBytes(len);
    if (mask()) {
      for (size_t i = 0; i < len; ++i) {
        payload_.beginWrite()[i] = (*(begin + i)) ^ maskKey_.c[i % 4];
      }
    } else {
      for (size_t i = 0; i < len; ++i) {
        payload_.beginWrite()[i] = *(begin + i);
      }
    }
    payload_.hasWritten(len);
  }

  Buffer &payload() { return payload_; }

  void getPayload(char *out) const {
    if (mask()) {
      for (size_t i = 0; i < payloadLen_; ++i) {
        out[i] = (*(payload_.peek() + i)) ^ maskKey_.c[i % 4];
      }
    } else {
      for (size_t i = 0; i < payloadLen_; ++i) {
        out[i] = *(payload_.peek() + i);
      }
    }
  }

  void setPayload(const char *data, size_t len) {
    assert(len <= frame::MAX_PAYLOAD_LENGTH);
    payloadLen_ = static_cast<unsigned int>(len);
    uint8_t size = 0;
    if (len > 0xFFFF) {
      extendedPayloadLength_ = frame::EXTENED_LENGTH_64BIT;
      size = frame::PAYLOAD_SIZE_64BIT;
    } else if (len > frame::PAYLOAD_SIZE_BASIC) {
      extendedPayloadLength_ = frame::EXTENED_LENGTH_16BIT;
      size = frame::PAYLOAD_SIZE_16BIT;
    } else {
      size = static_cast<uint8_t>(len);
    }
    setPayloadSize(size);
    payload_.append(data, len);
  }

  void setPayload(const std::string &data) {
    setPayload(&(*data.begin()), data.size());
  }

  void setPayload(Buffer *buf) {
    setPayload(buf->peek(), buf->readableBytes());
  }

  void reset() {
    basic_header_byte1_ = basic_header_byte2_ = 0;
    extendedPayloadLength_ = frame::EXTENED_LENGTH_0BIT;
    payloadLen_ = 0;
    payload_.retrieveAll();
  }

private:
  uint8_t basic_header_byte1_ = 0;
  uint8_t basic_header_byte2_ = 0;

  uint8_t extendedPayloadLength_ = frame::EXTENED_LENGTH_0BIT;
  unsigned int payloadLen_ = 0;
  uint32_converter maskKey_;
  Buffer payload_;
};

class WebSocketContext {
public:
  enum ParserState {
    BASIC_HEADER,
    EXTENDED_HEADER_LENGTH,
    GET_MASK,
    APPLICATION,
    READY,
    FATAL
  };

  bool decode(Buffer *buf);
  static void encode(Buffer *buf, const WebSocketData &data);

  static void makeFrame(Buffer *buf, uint8_t opcode, bool mask = false,
                        Buffer *payload = nullptr);
  static void makePingFrame(Buffer *buf);
  static void makePongFrame(Buffer *buf);

  bool ready() { return state_ = READY; }

  WebSocketData &reqData() { return reqData_; }

  void reset() {
    reqData_.reset();
    state_ = BASIC_HEADER;
  }

private:
  WebSocketData reqData_;
  ParserState state_ = BASIC_HEADER;
};

using WebSocketContextPtr = std::shared_ptr<WebSocketContext>;

static std::string getSecWebsocketAccept(const std::string &key) {
  std::string res(key);
  res.append(HANDSHAKE_MAGIC);
  unsigned char sha1Key[20];
  calc(res.c_str(), res.size(), sha1Key);
  res = base64_encode(sha1Key, 20);
  return res;
}
