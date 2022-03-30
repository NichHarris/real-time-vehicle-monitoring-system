#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dispatch.h>

// Define Attach Point
// - Server: Create a Channel with Attach Point Name
// - Client: Connect to Channel with Attach Point Name
#define ATTACH_POINT "myname"

// Define Struct for Msg Header as Pulse
// -> Contained at Start of All Messages Sent
// -> Used to Identify Message from Type/Subtype Fields (4 Bytes)
typedef struct _pulse msg_header_t;


// Define Struct for Msg, Including Header and Actual Data
typedef struct _my_data {
    msg_header_t hdr;
    int data;
} my_data_t;

// Server Side Code
int server() {
    // Define Msg
    my_data_t msg;

    // Define Receive Id
    int rcvid;

    // Define and Attach Name in Namespace (/dev/name/local/...) & Create Channel
    name_attach_t *attach;
    attach = name_attach(NULL, ATTACH_POINT, 0);
    if (attach == NULL) {
        return EXIT_FAILURE;
    }

   while (1) {
       // Call MsgReceive() and Wait Until Msg is Received through Channel
       rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);

       // Failed to Receive Pulse
       if (rcvid == -1) {
           break;
       }

        // Successfully Pulse Received
       if (rcvid == 0) {
            switch (msg.hdr.code) {
                // Detach Connection if Client Disconnected All Connections
                case _PULSE_CODE_DISCONNECT:
                    ConnectDetach(msg.hdr.scoid);
                    break;
                // Unblock Client and Reply to Client
                case _PULSE_CODE_UNBLOCK:
                    // TODO: Reply to Client
                    break;
                // Pulse Sent As_PULSE_CODE_COIDDEATH or _PULSE_CODE_THREADDEATH 
                default:
                    break;
            }
           continue;
       }

       // Check Msg Type and Reply with EOK to Connection Msg from name_open()
       if (msg.hdr.type == _IO_CONNECT ) {
           MsgReply( rcvid, EOK, NULL, 0 );
           continue;
       }

       // Check Msg Type and Reject QNX IO Msgs
       if (msg.hdr.type > _IO_BASE && msg.hdr.type <= _IO_MAX ) {
           MsgError( rcvid, ENOSYS );
           continue;
       }

        // TODO: 
        // 1) Read Msg from Client
        // 2) Check Type and Subtype
        // 3) Generate Response Msg Based on Type/Subtype Desired

        // Types:
        // 0x00 - Request Data
        // 0x01 - Update Configurations 
        //  -> Mechanism to Update Period and Frequency of Consumer/Server Thread

        // Subtypes: 
        // 0x01 - Fuel Consumption,
        // 0x02 - Engine Speed (RPM),
        // 0x03 - Engine Coolant Temperature
        // 0x04 - Current Gear
        // 0x05 - Vehicle Speed

        // TODO: Check Msg Header Type and Subtype
	   if (msg.hdr.type == 0x00) {
	      if (msg.hdr.subtype == 0x01) {
              printf("Server received: %d \n", msg.data);
	      }
	   }

	   // TODO: Generate Response
       MsgReply(rcvid, EOK, 0, 0);
   }

   // Remove and Detach Generated Name from Space in QNX
   name_detach(attach, 0);
   return EXIT_SUCCESS;
}


// Client Side Code
int client() {
    my_data_t msg;

    // Open Connection using name_open
    // -> Obtain Server Connection Id
    // -> Validate Connection Was Successful 
    int server_coid = name_open(ATTACH_POINT, 0);
    if (server_coid == -1) {
        return EXIT_FAILURE;
    }

    // Define Header Type and Subtype
    msg.hdr.type = 0x00;
    msg.hdr.subtype = 0x01;

    // TODO: Replace with Server Side Task to Be Performed
    for (msg.data=0; msg.data < 5; msg.data++) {
        printf("Client sending %d \n", msg.data);
        if (MsgSend(server_coid, &msg, sizeof(msg), NULL, 0) == -1) {
            break;
        }
    }

    // Close Connection using name_close
    name_close(server_coid);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    int ret;

    if (argc < 2) {
        printf("Usage %s -s | -c \n", argv[0]);
        ret = EXIT_FAILURE;
    } else if (strcmp(argv[1], "-c") == 0) {
        printf("Running Client ... \n");
        ret = client();
    } else if (strcmp(argv[1], "-s") == 0) {
        printf("Running Server ... \n");
        ret = server();
    } else {
        printf("Usage %s -s | -c \n", argv[0]);
        ret = EXIT_FAILURE;
    }

    return ret;
}

// Msg Passing in QNX
// 1- Create Channel Between Client and Server
// 2- Client Thread Makes Connnection by Attaching to Channel
// 3- Client Call MsgSend() to Send through Channel over Connection

/*
Functions Defined
- MsgSend() - Used by Client Threads to Send Messages to Server
- MsgReceive() - Used by Server Threads to Receive Messages from Client
- MsgReply() - Used by Server Threads to Send Messages to Client

Note: Send/Receive Functions -> Synchronous as Blocking Threads to Wait for Msgs

Client Initiates Communication + Server Receives and Responds to Client Requests

Client Thread: 
-> Ready State Then Call MsgSend()
-> Enters Blocked State, Waiting for Server to Call MsgReceive() (Send Blocked) then MsgReply() (Reply Blocked)
-> Or Enters Directly Reply Blocked if Server Already Called MsgReceive()
-> Back in Ready State Upon Receiving Reply from Server

Server Thread: 
-> Ready State Then Call MsgReceive()
-> Enters Blocked State, Waiting for Client to Send Msg (Receive Blocked)
-> Or Stays in Ready State if Client Already Called MsgSend()
-> Back in Ready State Upon Receiving Msg Sent By Client

// Note: Kernel Changes Status of Client Threads
*/