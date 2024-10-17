/**********************************************************
               packetstruct.cpp - 10/17/2024
    Authors:
        * Charlie Koenig
        * Idil Kolabas

    This module provides an implementation for the 
    packetstruct interface, useful for sending data
    over the C150NETWORK
    
***********************************************************/
#include "packetstruct.h"
#include <cstring>
#include <string>
#include <cassert>
#include <iostream>

using namespace std;

/**********************************************************
 * Struct: packetStruct
 
 * Fields: 
    * char opcode       : Operation requested by packing being created
    * int contentLength : Length of the packet when converted to a strings
    * int packetNum     : Identifier for packet, in range 0 to (2^14 - 1)
    * char *content     : char pointer to the content of the new packet

 * Notes: 
    * packetstruct* is typedef'd as packet
***********************************************************/
struct packetStruct {
    char opcode;               // 8  bits
    unsigned int length;       // 10 bits
    unsigned int packetNum;    // 14 bits
    char *content;
};


/**********************************************************
 * Function: makePacket
 
 * Parameters: 
    * char opcode       : Operation requested by packing being created
    * int contentLength : Length of the content section in the packet
    * int packetNum     : Identifier for packet, in range 0 to (2^14 - 1)
    * char *content     : char pointer to the content of the new packet

 * Return: Address of a packetStruct with given values

 * Notes: 
    * Allocates memory for packet and content that must be freed by
      the caller
***********************************************************/
packet 
makePacket(char opcode, int contentLength, int packetNum, char *content) {
    packet newPacket = (packet)malloc(sizeof(*newPacket));

    newPacket->opcode = opcode;
    newPacket->length = contentLength + 5;  // add for opcode, length, num, null
    newPacket->packetNum = packetNum;

    newPacket->content = (char *)malloc(contentLength + 1);

    for (int i = 0; i < contentLength; i++) {
        newPacket->content[i] = content[i];
    }
    newPacket->content[contentLength] = '\0';

    return newPacket;
}


/**********************************************************
 * Function: stringToPacket
 
 * Parameters: 
    * unsigned char *packetString : a null terminated array encoding packet
                                    data

 * Return: Address of a packetStruct with values that were encoded in string

 * Notes: 
    * CRE for packetString to be NULL
    * Allocates memory for packet and content in makePacket call that must 
      be freed by the caller
***********************************************************/
packet 
stringToPacket(unsigned char *packetString) {
    assert(packetString != NULL);

    char opcode = packetString[0];
    int contentLength = ((packetString[1] << 2) + (packetString[2] >> 6)) - 5;
    int packetNumber = ((packetString[2] & 0x3F) << 8) + packetString[3];

    char content[contentLength];
    for (int i = 4; i < (contentLength + 4); i++) {
        content[i - 4] = packetString[i];
    }

    return makePacket(opcode, contentLength, packetNumber, content);
}


/**********************************************************
 * Function: packetContent
 
 * Parameters: 
    * packet packet : a packetStruct pointer

 * Return: Address of a char aray that is the packet's contents

 * Notes: 
    * CRE for packet to be NULL
***********************************************************/
char *
packetContent(packet packet) {
    assert(packet != NULL);
    return packet->content;
}


/**********************************************************
 * Function: packetLength
 
 * Parameters: 
    * packet packet : a packetStruct pointer

 * Return: length field of the packetStruct

 * Notes: 
    * CRE for packet to be NULL
***********************************************************/
int  
packetLength(packet packet) {
    assert(packet != NULL);
    return packet->length;
}


/**********************************************************
 * Function: packetNum
 
 * Parameters: 
    * packet packet : a packetStruct pointer

 * Return: packetNum field of the packetStruct

 * Notes: 
    * CRE for packet to be NULL
***********************************************************/
int  
packetNum(packet packet) {
    assert(packet != NULL);
    return packet->packetNum;
}


/**********************************************************
 * Function: packetOpcode
 
 * Parameters: 
    * packet packet : a packetStruct pointer

 * Return: opcode field of the packetStruct

 * Notes: 
    * CRE for packet to be NULL
***********************************************************/
char 
packetOpcode(packet packet) {
    assert(packet != NULL);
    return packet->opcode;
}


/**********************************************************
 * Function: packetToString
 
 * Parameters: 
    * packet packet : a packetStruct pointer

 * Return: a null terminated char array representation of the 
           provided packet

 * Notes: 
    * CRE for packet to be NULL
    * Allocates space for the returned C string that must be 
      freed by the caller
***********************************************************/
char *
packetToString(packet packet) {
    assert(packet != NULL);

    int length         = packetLength(packet);
    int packetNumber   = packetNum(packet);
    char *packetString = (char *) malloc(length);
    
    packetString[0] = packetOpcode(packet);
    packetString[1] = (length >> 2) & 0xFF;
    packetString[2] = ((length & 0x03) << 6) + ((packetNumber >> 8) & 0x3F);
    packetString[3] = (packetNumber & 0xFF);

    char *content = packetContent(packet);

    for (int i = 4; i < length; i++) {
        packetString[i] = content[i - 4];
    }

    return packetString;
}

/**********************************************************
 * Function: packetCompare
 
 * Parameters: 
    * packet p1 : a packetStruct pointer
    * packet p2 : a packetStruct pointer

 * Return: a boolean that is true if the packetStruct's fields
           are equivalent, not including packetNumber

 * Notes: 
    * CRE for p1 or p2 to be NULL
***********************************************************/
bool 
packetCompare(packet p1, packet p2) {
    assert((p1 != NULL) && (p2 != NULL));

    int p1Len = packetLength(p1);
    int p2Len = packetLength(p2);

    if (p1Len != p2Len) {
        cout << "Length wrong\n";
        return false;
    }

    if (packetOpcode(p1) != packetOpcode(p2)) {
        cout << "Opcode wrong\n";
        return false;
    }

    if ((memcmp(packetContent(p1), packetContent(p2), p1Len - 4) != 0)) {
        cout << "Content Wrong\n";
        return false;
    }

    return true;
}


/**********************************************************
 * Function: printContent
 
 * Parameters: 
    * packet packet : a packetStruct pointer

 * Return: none

 * Notes: 
    * CRE for packet to be NULL
    * Prints the bytes pointed to byt packetStrcut's content 
      field to standard output
***********************************************************/
void
printContent(packet packet) {
    assert(packet != NULL);

    int contentLength = packetLength(packet) - 5;
    for (int i = 0; i < contentLength; i++) {
        printf("%c", packetContent(packet)[i]);
    }
    cout << endl;
}

/**********************************************************
 * Function: printPacket
 
 * Parameters: 
    * packet packet : a packetStruct pointer

 * Return: none

 * Notes: 
    * CRE for packet to be NULL
    * Prints the packetStrcut's fields to standard output
***********************************************************/
void
printPacket(packet packet) {
    assert(packet != NULL);

    printf("PACKET CONTENTS\n");
    cout << packetOpcode(packet) << endl;
    printf("%d\n", packetLength(packet));
    printf("%d\n", packetNum(packet));
    printContent(packet);
    cout << "_________________________" << endl; 
}


/**********************************************************
 * Function: freePacket
 
 * Parameters: 
    * packet packet : a packetStruct pointer

 * Return: none

 * Notes: 
    * CRE for packet to be NULL
    * Frees the heap allocated memory pointed to by the 
      packetStruct's content field and the packetStruct
***********************************************************/
void 
freePacket(packet packet) {
    assert(packet != NULL);

    free(packet->content);
    free(packet);
}
