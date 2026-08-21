/*
MOST USELESS HALT CODE EVERRR
so basically this guy just halts the pc but i needed a excuse for a useless syscall and a even more useless variable SO HERE WE AREEE
this code halts the pc and changed the wigga value 
IF AT BOOT WIGGA ISNT 0 IT WONT BOOT 
written by vujuvuju for KuzuOS2 :)
*/
#include "z_syscalls.h" // yknow for z_print n shii
#include "z_utils.h"    // for printing za best wigga 
#define WIGGA_UPDATE  666 // syscall to update gigawigga value
#define GET_WIGGA     668 // getter function to get gigawigga 
unsigned int gigawigga = 0; // store ma wigga
static unsigned int temp; // used to hold the value of ma wigga 

void main() {
   __asm__ volatile("int $0x80" : : "a"(WIGGA_UPDATE)); // update ma wigga 

   __asm__ volatile(
        "int $0x80" 
        : "=a"(temp) 
        : "a"(GET_WIGGA) // gets za wigga 
    );

   gigawigga = temp; // get ma wigga AGAIN

   if(gigawigga != 0){ // check ma wigga 
        z_write(1, "gigawigga updated\n", 19); // yayyy
        z_printf("gigawigga value: %u\n", gigawigga); // show ma wigga value2
    } 
   else {
        z_write(1, "DIEEEE DIEE BY HANDD I CREEP ACROSS THE LANDDD KILLING FIRSTBORN WIGGAAAA\n", 27); // uh ohhh
   }
   z_write(1, "za system shall be halted bay bayyyy :)\n", 37 );
   
   __asm__ volatile("int $0x80" : : "a"(667)); // sistemi halt ettim  KILLING IN THE NAMEEE OFFFFF
}