//! @file
//! @brief WebSocket module - Source file.
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
#include "websocket.hpp"
#include <random>
#include <mutex>
//===========================================
namespace ict { namespace websocket {
//===========================================
template <typename I=int> I randomInteger(I from,I to){
  static std::mutex mutex;
  std::lock_guard<std::mutex> lock(mutex);
  static std::random_device rd;
  static std::mt19937 generator(rd());
  std::uniform_int_distribution<I> distribution(from,to);
  return(distribution(generator));
}
bool read(std::string & input_stream,std::string & output_message,opcode_t & input_opcode){
  header1_t *  header1_ptr=nullptr;
  header2a_t * header2a_ptr=nullptr;
  header2b_t * header2b_ptr=nullptr;
  header3_t *  header3_ptr=nullptr;
  body_t       body=nullptr;
  std::size_t body_len=0;
  std::size_t input_pos=0;
  std::string output_tmp;
  for (std::size_t input_size=input_stream.size();input_pos<input_size;){
    if ((input_size-input_pos)<sizeof(header1_t)) return(false);
    header1_ptr=(header1_t*)(input_stream.c_str()+input_pos);//Nagłówek 1
    if (!input_pos) input_opcode=(opcode_t)(header1_ptr->opcode);
    input_pos+=sizeof(header1_t);
    switch (header1_ptr->len){
      case 126:{
        if ((input_size-input_pos)<sizeof(header2a_t)) return(false);
        header2a_ptr=(header2a_t*)(input_stream.c_str()+input_pos);//Nagłówek 2a
        input_pos+=sizeof(header2a_t);
        header2b_ptr=nullptr;
      } break;
      case 127 :{
        header2a_ptr=nullptr;
        if ((input_size-input_pos)<sizeof(header2b_t)) return(false);
        header2b_ptr=(header2b_t*)(input_stream.c_str()+input_pos);//Nagłówek 2b
        input_pos+=sizeof(header2b_t);
      } break;
      default:{
        header2a_ptr=nullptr;
        header2b_ptr=nullptr;
      } break;
    }
    if (header1_ptr->mask){
      if ((input_size-input_pos)<sizeof(header3_t)) return(false);
      header3_ptr=(header3_t*)(input_stream.c_str()+input_pos);//Nagłówek 3
      input_pos+=sizeof(header3_t);
    } else {
      header3_ptr=nullptr;
    }
    {
      std::size_t tmp_len=output_tmp.size();
      body_len=(header2b_ptr?header2b_ptr->len:(header2a_ptr?header2a_ptr->len:header1_ptr->len));
      if ((input_size-input_pos)<body_len) return(false);
      body=(body_t)(input_stream.c_str()+input_pos);//Body
      input_pos+=body_len;
      output_tmp.append(body,body_len);
      if (header3_ptr) for (std::size_t k=0;k<body_len;k++){
        output_tmp[tmp_len+k]^=header3_ptr->mask[k%sizeof(header3_t)];
      }
    }
    if (header1_ptr->fin) break;//Jest cała wiadomość.
    header1_ptr=nullptr;
  }
  if ((output_message.max_size()-output_message.size())<output_tmp.size()) return(false);
  input_stream.erase(0,input_pos);
  output_message.append(output_tmp);
  return(true);
}
bool write(std::string & input_message,std::string & output_stream,opcode_t & output_opcode,uint64_t max_payload,bool mask_payload){
  header1_t *  header1_ptr=nullptr;
  header2a_t * header2a_ptr=nullptr;
  header2b_t * header2b_ptr=nullptr;
  header3_t *  header3_ptr=nullptr;
  std::size_t input_pos=0;
  std::string output_tmp;
  uint64_t body_len=0;
  for (std::size_t input_size=input_message.size();input_pos<input_size;){
    uint8_t mask[sizeof(header3_t)];
    body_len=(input_size-input_pos);
    if (max_payload) if (max_payload<body_len) body_len=max_payload;
    output_tmp.append(sizeof(header1_t),'\0');
    header1_ptr=(header1_t*)(output_tmp.c_str()+output_tmp.size()-sizeof(header1_t));//Nagłówek 1
    header1_ptr->rsv1=0;header1_ptr->rsv2=0;header1_ptr->rsv3=0;
    if (input_pos){
      header1_ptr->opcode=opcode_continuation;
    } else switch (output_opcode){
      case opcode_text:
      case opcode_binary:
      case opcode_close:
      case opcode_ping:
      case opcode_pong:{
        header1_ptr->opcode=output_opcode;
      } break;
      default:{
        header1_ptr->opcode=opcode_binary;
      } break;
    }
    header1_ptr->fin=((input_pos+body_len)<input_size)?0:1;
    header1_ptr->mask=mask_payload?1:0;
    if (body_len<126){
      header1_ptr->len=body_len;
    } else if (body_len<=0xffff){
      header1_ptr->len=126;
    } else {
      header1_ptr->len=127;
    }
    switch (header1_ptr->len){
      case 126:{
        output_tmp.append(sizeof(header2a_t),'\0');
        header2a_ptr=(header2a_t*)(output_tmp.c_str()+output_tmp.size()-sizeof(header2a_t));//Nagłówek 2a
        header2a_ptr->len=body_len;
        header2b_ptr=nullptr;
      } break;
      case 127 :{
        header2a_ptr=nullptr;
        output_tmp.append(sizeof(header2b_t),'\0');
        header2b_ptr=(header2b_t*)(output_tmp.c_str()+output_tmp.size()-sizeof(header2b_t));//Nagłówek 2b
        header2b_ptr->len=body_len;
      } break;
      default:{
        header2a_ptr=nullptr;
        header2b_ptr=nullptr;
      } break;
    }
    if (mask_payload){
      output_tmp.append(sizeof(header3_t),'\0');
      header3_ptr=(header3_t*)(output_tmp.c_str()+output_tmp.size()-sizeof(header3_t));//Nagłówek 3
      for (int k=0;k<sizeof(header3_t);k++) {
        mask[k]=randomInteger<uint8_t>(0x0,0xff);
        header3_ptr->mask[k]=mask[k];
      }
    } else {
      header3_ptr=nullptr;
    }
    {
      std::size_t tmp_len=output_tmp.size();
      output_tmp.append(input_message,input_pos,body_len);//Body.
      if (mask_payload) for (std::size_t k=0;k<body_len;k++){
        output_tmp[tmp_len+k]^=mask[k%sizeof(header3_t)];
      }
    }
    input_pos+=body_len;
    header1_ptr=nullptr;
  }
  if ((output_stream.max_size()-output_stream.size())<output_tmp.size()) return(false);
  input_message.clear();
  output_stream.append(output_tmp);
  return(true);
}
//===========================================
}}
//===========================================
#ifdef ENABLE_TESTING
#include "test.hpp"
REGISTER_TEST(websocket,tc1){
  try{
    ict::websocket::opcode_t output_opcode=ict::websocket::opcode_text;
    std::string text_in(ict::test::test_string[8]);
    std::string ws;
    std::string text_out;
    ict::websocket::write(text_in,ws,output_opcode);
    ict::websocket::read(ws,text_out,output_opcode);
    if (ict::test::test_string[8]!=text_out){
      std::cerr<<"ERROR: '"<<ict::test::test_string[8]<<"'!='"<<text_out<<"'"<<std::endl;
      return(1);
    }
  }catch(const std::exception & e){
    std::cerr<<"ERROR: "<<e.what()<<"!"<<std::endl;
    return(1);
  }
  return(0);
}
REGISTER_TEST(websocket,tc2){
  try{
    ict::websocket::opcode_t output_opcode=ict::websocket::opcode_text;
    std::string text_in(ict::test::test_string[4]);
    std::string ws;
    std::string text_out;
    if (!ict::websocket::write(text_in,ws,output_opcode,10)){
      std::cerr<<"ERROR: ict::websocket::write"<<std::endl;
    }
    if (!ict::websocket::read(ws,text_out,output_opcode)){
      std::cerr<<"ERROR: ict::websocket::read"<<std::endl;
    }
    if (ict::test::test_string[4]!=text_out){
      std::cerr<<"ERROR: '"<<ict::test::test_string[4]<<"'!='"<<text_out<<"'"<<std::endl;
      return(1);
    }
  }catch(const std::exception & e){
    std::cerr<<"ERROR: "<<e.what()<<"!"<<std::endl;
    return(1);
  }
  return(0);
}
REGISTER_TEST(websocket,tc3){
  try{
    ict::websocket::opcode_t output_opcode=ict::websocket::opcode_text;
    std::string text_in(ict::test::test_string[2]);
    std::string ws;
    std::string text_out;
    if (!ict::websocket::write(text_in,ws,output_opcode,0,false)){
      std::cerr<<"ERROR: ict::websocket::write"<<std::endl;
    }
    if (!ict::websocket::read(ws,text_out,output_opcode)){
      std::cerr<<"ERROR: ict::websocket::read"<<std::endl;
    }
    if (ict::test::test_string[2]!=text_out){
      std::cerr<<"ERROR: '"<<ict::test::test_string[2]<<"'!='"<<text_out<<"'"<<std::endl;
      return(1);
    }
  }catch(const std::exception & e){
    std::cerr<<"ERROR: "<<e.what()<<"!"<<std::endl;
    return(1);
  }
  return(0);
}
REGISTER_TEST(websocket,tc4){
  try{
    ict::websocket::opcode_t output_opcode=ict::websocket::opcode_text;
    std::string text_in1(ict::test::test_string[8]);
    std::string text_in2(ict::test::test_string[5]);
    std::string ws;
    std::string text_out1;
    std::string text_out2;
    if (!ict::websocket::write(text_in1,ws,output_opcode)){
      std::cerr<<"ERROR: ict::websocket::write"<<std::endl;
    }
    if (!ict::websocket::write(text_in2,ws,output_opcode)){
      std::cerr<<"ERROR: ict::websocket::write"<<std::endl;
    }
    if (!ict::websocket::read(ws,text_out1,output_opcode)){
      std::cerr<<"ERROR: ict::websocket::read"<<std::endl;
    }
    if (!ict::websocket::read(ws,text_out2,output_opcode)){
      std::cerr<<"ERROR: ict::websocket::read"<<std::endl;
    }
    if (ict::test::test_string[8]!=text_out1){
      std::cerr<<"ERROR: '"<<ict::test::test_string[8]<<"'!='"<<text_out1<<"'"<<std::endl;
      return(1);
    }
    if (ict::test::test_string[5]!=text_out2){
      std::cerr<<"ERROR: '"<<ict::test::test_string[8]<<"'!='"<<text_out2<<"'"<<std::endl;
      return(1);
    }
  }catch(const std::exception & e){
    std::cerr<<"ERROR: "<<e.what()<<"!"<<std::endl;
    return(1);
  }
  return(0);
}
#endif
//===========================================