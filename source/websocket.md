# `ict::websocket` module

Provides a functions for a WebSocket message encoding (write) and decoding (read).

## Usage

Following functions are available:
* `ict::websocket::read(input_stream,output_message,input_opcode)` - Reads WS data from input_stream and writes message to output_message (opcode is set in input_opcode);
* `ict::websocket::write(input_message,output_stream,output_opcode,max_payload=0,mask_payload=true)` - Reads message from input_message and writes WS to output_stream (opcode must be given in output_opcode).
