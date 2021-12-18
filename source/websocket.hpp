//! @file
//! @brief WebSocket module - Header file.
//! @author Mariusz Ornowski (mariusz.ornowski@ict-project.pl)
//! @version 1.0
//! @date 2012-2021
//! @copyright ICT-Project Mariusz Ornowski (ict-project.pl)
/* **************************************************************
Copyright (c) 2012-2021, ICT-Project Mariusz Ornowski (ict-project.pl)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. Neither the name of the ICT-Project Mariusz Ornowski nor the names
of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**************************************************************/
#ifndef _ICT_WEBSOCKET_HEADER
#define _ICT_WEBSOCKET_HEADER
#include <string>
//===========================================
namespace ict { namespace websocket {
//===========================================
enum opcode_t {
  opcode_continuation=0x0,//a continuation frame
  opcode_text=0x1,//a text frame
  opcode_binary=0x2,//a binary frame
  //  0x3 - 0x7 are reserved for further non-control frames
  opcode_close=0x8,//a connection close
  opcode_ping=0x9,//a ping
  opcode_pong=0xA//a pong
  // 0xB - 0xF are reserved for further control frames
};
//       0                   1                   2                   3
//       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//      +-+-+-+-+-------+-+-------------+-------------------------------+
//      |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
//      |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
//      |N|V|V|V|       |S|             |   (if payload len==126/127)   |
//      | |1|2|3|       |K|             |                               |
//      +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
//      |     Extended payload length continued, if payload len == 127  |
//      + - - - - - - - - - - - - - - - +-------------------------------+
//      |                               |Masking-key, if MASK set to 1  |
//      +-------------------------------+-------------------------------+
//      | Masking-key (continued)       |          Payload Data         |
//      +-------------------------------- - - - - - - - - - - - - - - - +
//      :                     Payload Data continued ...                :
//      + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
//      |                     Payload Data continued ...                |
//      +---------------------------------------------------------------+
struct header1_t {
  uint8_t fin:1;
  uint8_t rsv1:1;
  uint8_t rsv2:1;
  uint8_t rsv3:1;
  uint8_t opcode:4;
  uint8_t mask:1;
  uint8_t len:7;
};
struct header2a_t {
  uint16_t len;
};
struct header2b_t {
  uint64_t len;
};
struct header3_t {
  uint8_t mask[4];
};
typedef char * body_t;
//!
//! Odczytuje z input_stream dane i zapisuje wiadomość WebSocket w output_message.
//!
//! @param input_stream Dane w formacie WebSocket. Odcztana wiadomość jest kasowana na początku bufora.
//! @param output_message Dane wiadomości. Wiadomość jest umieszczana na końcu bufora.
//! @param input_opcode Rodzaj odczytanej wiadomości (w przypadku ramek kontrolnych wiadomość jest pusta).
//! @return Wartości:
//!  @li 'true' - wiadomość została odczytana z sukcesem;
//!  @li 'false' - wiadomość nie została odczytana (brak wystarczających danych).
//!
bool read(std::string & input_stream,std::string & output_message,opcode_t & input_opcode);
//!
//! Odczytuje wiadomość WebSocket z input_message i zapisuje dane w output_stream.
//!
//! @param input_message Dane wiadomości. Odcztana wiadomość jest kasowana.
//! @param output_stream Dane w formacie WebSocket. Wiadomość jest umieszczana na końcu bufora.
//! @param output_opcode Rodzaj zapisywanej wiadomości (w przypadku ramek kontrolnych wiadomość jest ignorowana).
//! @param max_payload Maksymalny rozmiar payload w ramce - jeśli 0, to brak ograniczeń.
//! @param mask_payload Czy payload ma być maskowany.
//! @return Wartości:
//!  @li 'true' - wiadomość została zapisana z sukcesem;
//!  @li 'false' - wiadomość nie została zapisana (brak miejsca w buforze).
//!
bool write(std::string & input_message,std::string & output_stream,opcode_t & output_opcode,uint64_t max_payload=0,bool mask_payload=true);

//===========================================
}}
//===========================================
#endif