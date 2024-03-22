#include "WebSocketContext.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <regex>

bool WebSocketContext::decode(Buffer *buf) {
  while (state_ != READY and state_ != FATAL) {
    if (state_ == BASIC_HEADER) {
      size_t leftLen = buf->readableBytes();
      if (leftLen < frame::BASIC_HEADER_LENGTH)
        break;

      reqData_.setBasicHeader(*(buf->peek()), *(buf->peek() + 1));

      if (frame::opcode::reserved(reqData_.opcode())) {
        state_ = FATAL;
        break;
      }

      buf->retrieve(frame::BASIC_HEADER_LENGTH);
      reqData_.setPayloadLen(reqData_.payloadSize());
      bool masked = reqData_.mask();
      if (reqData_.payloadSize() == 0) {
        state_ = masked ? GET_MASK : READY;
      } else {
        if (reqData_.payloadSize() <= frame::PAYLOAD_SIZE_BASIC) {
          state_ = masked ? GET_MASK : APPLICATION;
        } else {
          if (reqData_.payloadSize() == frame::PAYLOAD_SIZE_16BIT) {
            reqData_.setExtendedPayloadLength(frame::EXTENED_LENGTH_16BIT);
          } else {
            reqData_.setExtendedPayloadLength(frame::EXTENED_LENGTH_64BIT);
          }
          state_ = EXTENDED_HEADER_LENGTH;
        }
      }
    } else if (state_ == EXTENDED_HEADER_LENGTH) {
      size_t leftLen = buf->readableBytes();
      if (leftLen < reqData_.extendedPayloadLength())
        break;

      if (reqData_.extendedPayloadLength() == frame::EXTENED_LENGTH_16BIT) {
        reqData_.setPayloadLen(
            ntoh16(*(reinterpret_cast<const uint16_t *>(buf->peek()))));
      } else {
        uint64_t len =
            ntoh64(*(reinterpret_cast<const uint64_t *>(buf->peek())));
        if (len > frame::MAX_PAYLOAD_LENGTH) {
          state_ = FATAL;
          break;
        }
        reqData_.setPayloadLen(len);
      }

      buf->retrieve(reqData_.extendedPayloadLength());
      state_ = reqData_.mask() ? GET_MASK : APPLICATION;
    } else if (state_ == GET_MASK) {
      size_t leftLen = buf->readableBytes();
      if (leftLen < frame::MASK_BYTES)
        break;

      reqData_.setMaskKey(reinterpret_cast<const uint8_t *>(buf->peek()));

      buf->retrieve(frame::MASK_BYTES);
      state_ = reqData_.payloadSize() == 0 ? READY : APPLICATION;
    } else if (state_ == APPLICATION) {
      size_t leftLen = buf->readableBytes();
      if (leftLen < reqData_.payloadLen())
        break;

      reqData_.handlePayload(buf->peek(), reqData_.payloadLen());

      buf->retrieve(reqData_.payloadLen());
      state_ = READY;
    }
  }

  return state_ != FATAL;
}

void WebSocketContext::encode(Buffer *buf, const WebSocketData &data) {
  buf->append(data.basicHeaderByte1(), 1);
  buf->append(data.basicHeaderByte2(), 1);

  buf->ensureWritableBytes(data.extendedPayloadLength());
  data.getEXtendedPayloadLengthInfo(buf->beginWrite());
  buf->hasWritten(data.extendedPayloadLength());

  if (data.mask()) {
    buf->ensureWritableBytes(frame::MASK_BYTES);
    data.getMaskKey(buf->beginWrite());
    buf->hasWritten(frame::MASK_BYTES);
  }

  if (data.payloadSize() > 0) {
    buf->ensureWritableBytes(data.payloadLen());
    data.getPayload(buf->beginWrite());
    buf->hasWritten(data.payloadLen());
  }
}

void WebSocketContext::makeFrame(Buffer *buf, uint8_t opcode, bool mask,
                                 Buffer *payload) {
  WebSocketData data;
  data.setFin();
  data.setOpcode(opcode);

  if (mask) {
    data.setMask();
    uint32_converter maskKey;
    maskKey.i = rand();
    data.setMaskKey(maskKey.c);
  }

  if (payload and payload->readableBytes() > 0) {
    data.setPayload(payload);
  }

  encode(buf, data);
}

void WebSocketContext::makePingFrame(Buffer *buf) {
  makeFrame(buf, frame::opcode::PING);
}

void WebSocketContext::makePongFrame(Buffer *buf) {
  makeFrame(buf, frame::opcode::PONG, true);
}
