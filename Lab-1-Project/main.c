__asm void my_hashcheck(const char *str, int *hash, const int* buff)
{
	PUSH {r4, r5}
cap_loop
	LDRB  r3, [r0]    // Load byte into register r3 from memory register r0 (str pointer)
	CMP   r3, #'A'-1  // compare it with the character before 'A'
	BLS   cap_numcheck    // If byte is lower or same, then skip this byte
	
	CMP   r3, #'Z'    // Compare it with the 'Z' character
	BHI   cap_numcheck    // If it is higher, then skip this byte
	
	SUBS  r3,#65	// Else subtract out difference to find the desired index of the buff array
	LDR   r4, [r2,r3, LSL#2] //We access the element of the array based on the given index
	
	LDR r5,[r1,#0]	//Here we calculate our hash variable by adding the value of the above element
	ADDS r5, r5, r4
	STR r5, [r1,#0]
	
cap_numcheck
	CMP r3, #'0'-1	// compare it with the character before '0'
	BLS cap_skip
	
	CMP r3, #'9'	// compare it with the character '9'
	BHI cap_skip
	
	SUBS r3,#48	//Else we subtract out the difference to find arithmetic value of the ascii character
	
	LDR r5,[r1, #0]	//Here we calculate our hash variable by subtracting the value of the above element
	SUBS r5, r5, r3
	STR r5, [r1,#0]
	
	
cap_skip
	LDRB  r3, [r0]		// We use this line in case of the given character being "A" which would result in 65-65=0 for R3.
	ADDS  r0, r0, #1  // Increment strs pointer
	CMP   r3, #0      // Was the byte 0?
	BNE   cap_loop    // If not, repeat the loop
	POP{r4,r5}
	BX    lr          // Else return from subroutine
}

#include <stdio.h>
#include <stdlib.h>
int main(void)
{
	const int buff[26] = {10,42,12,21,7,5,67,48,69,2,36,3,19,1,14,51,71,8,26,54,75,15,6,59,13,25};
	const char a[20] = "0\nAA ABA920 B9!";
	int* hash;
	hash = (int *) malloc(1*sizeof(int));
	*hash = 0;
		
	my_hashcheck(a, hash, buff); //Calling function responsible for calculating the final hash value
	printf("%d\n", *hash);  //printing hash value at stdout
}
