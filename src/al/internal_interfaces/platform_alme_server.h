/*
 *  Broadband Forum IEEE 1905.1/1a stack
 *
 *  Copyright (c) 2017, Broadband Forum
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef _PLATFORM_ALME_SERVER_H_
#define _PLATFORM_ALME_SERVER_H_

////////////////////////////////////////////////////////////////////////////////
// ALME message server related functions
////////////////////////////////////////////////////////////////////////////////

// This function is called everytime a new ALME RESPONSE/CONFIRMATION meesage
// is generated by the AL entity.
//
// A RESPONSE/CONFIRMATION that is produced as a result of a previous REQUEST,
// contains the same 'alme_client_id' as the one the REQUEST originally inserted
// in the messages queue (as explained in the "PLATFORM_REGISTER_QUEUE_EVENT()"
// documentation).
//
// 'alme_message' is a pointer to the ALME payload and is 'alme_message_len'
// bytes long
//
// Return '0' if there was some problem processing the RESPONSE/CONFIRMATION,
// "1" otherwise.
//
INT8U PLATFORM_SEND_ALME_REPLY(INT8U alme_client_id, INT8U *alme_message, INT16U alme_message_len);

#endif
