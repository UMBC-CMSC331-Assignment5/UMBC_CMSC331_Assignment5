/*
* Authors: Will Morgan, Hamza Saeed, Christopher Blake
* Date: 11/08/2014
* Written in C
* The algorithms were found on:
* https://github.com/Umbc331Assignment5/assignment5/blob/master/assignment5.c
*/

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void read_sixteenbit_i(FILE * fp, uint8_t length);
void read_thirtytwobit_i(FILE * fp, uint8_t length);
void read_thirtytwobit_f(FILE * fp, uint8_t length);
void read_sixtyfourbit_f(FILE * fp, uint8_t length);
void read_junk(FILE * fp, uint8_t length);
void read_burn(FILE * fp, uint8_t length);
void read_skip(FILE * fp, uint8_t length);
void read_ascii(FILE * fp, uint8_t length);
int read_data(FILE * fp, uint8_t type, uint8_t length);

typedef struct header
{
	union versiontype 
	{
		uint8_t version;
		uint8_t type;
	}vt;
	uint8_t length;
	uint16_t s;
} header;


int main(int argc, char ** argv)
{
	argv++; // make it look at first argument
	FILE * fp = NULL;
	size_t result;

	header * headbuff = malloc(4); // all versions have a header 4 bytes long

	if (headbuff == NULL) { printf("Memory error\n"); return -1;}
	fp = fopen(*argv, "rb");
	if (fp == NULL) { printf("Error opening file\n"); return -2; }

	while((result = fread(headbuff, 4, 1, fp)) == 1)
	{
		//old way
		uint8_t the_version = headbuff->vt.version & 0x0F; //gets lower 4 bits
		uint8_t the_type = (headbuff->vt.type & 0xF0) >> 4; 	//gets higher 4 bits
		uint8_t the_length = headbuff->length;
		uint8_t skipbit = headbuff->s & 0x1;				//takes first bit


		//checks the skip bit if true proccess the next thelength bytes as junk
		if(skipbit) // if skipbit is set
		{
			printf("\nWe read version %d type %d length %d skipbit %d\n",
					(int)the_version, (int)the_type ,(int)the_length ,(int)skipbit);
			read_junk(fp,the_length);
			continue;
		}

		//version 1 datagram
		if(the_version == 1)
		{
			printf("\nWe read version: %d type: %d length: %d skipbit: %d\n",
					(int)the_version, (int)the_type ,(int)the_length ,(int)skipbit);

			if(read_data(fp, the_type, the_length)){ break;} //if true were done
			continue;
		} //end version 1
		
		//version 2 datagram
		if(the_version == 2)
		{
			uint8_t dupbit = headbuff->s & 0x2;
			uint16_t checksum = headbuff->s & 0xFF00;
			if(dupbit)
			{
				printf("\nWe read version: %d type %d length: %d skipbit: %d dupbit: %d\n",
						(int)the_version, (int)the_type ,
						(int)the_length, (int)skipbit, (int)dupbit);

				fpos_t pos;
				fgetpos(fp, &pos); //remember position
				if(read_data(fp, the_type, the_length)){ break;} //if true we're done
				fsetpos(fp, &pos);
				if(read_data(fp, the_type, the_length)){ break;} //Read it again
				continue;
			}

			if(read_data(fp, the_type, the_length)){ break;} //if true were done
			continue;
		} //end version 2

		//version 3 datagram
		if(the_version == 3)
		{
			uint8_t id = headbuff->s & 0xFE;
			uint16_t checksum = headbuff->s & 0xFF00;
			printf("\nWe read version: %d type: %d length: %d skipbit: %d id: %d checksum: %d\n",
					(int)the_version, (int)the_type,
					(int)the_length, (int)skipbit,
					(int)id, (int)checksum);

			if(read_data(fp, the_type, the_length)){ break;} //if true were done
			//TODO should care about checksum after he specifies what the algorithm is
			continue;
		} //end version 3
	}//end while

	free(headbuff);
	fclose(fp);

	return 0;
}
//TODO maybe abstract away reading header header to here
void read_header(FILE * fp)
{
	
}

//Big ol switch for which kind of data to read in
int read_data(FILE * fp, uint8_t type, uint8_t length)
{
	switch(type)
	{
		case 0: // 16 bit int
		{
			read_sixteenbit_i(fp,length);
			break;
		}
		case 1: // 32 bit int
		{
			read_thirtytwobit_i(fp, length);
			break;
		}
		case 2: // 32 bit float
		{
			read_thirtytwobit_f(fp,length);
			break;
		}
		case 3: // 64 bit float
		{
			read_sixtyfourbit_f(fp,length);
			break;
		}
		case 7: // ascii
		{
			read_ascii(fp,length);
			break;
		}
		case 8: // junk
		{
			read_junk(fp,length);
			break;
		}
		case 9: // skip
		{
			read_skip(fp,length);
			break;
		}
		case 10: // burn
		{
			read_burn(fp,length);
			break;
		}
		case 11: // stop
		{
			return 1; //Were done
			break;
		}
		default:
		{
			break;
		}
	}
	return 0;
}


/////All assume length is the number of the items specified

//Reads length number of 16 bit integers
void read_sixteenbit_i(FILE * fp, uint8_t length)
{
	uint16_t * numbers = malloc(sizeof(uint16_t) * length); //allocate space
	uint16_t * temp = numbers;				//pointer for iterating
	int num_bytes = length * 2;		//debugging
	int result;	//for storing number of successful things read by fread
	int i = 0;	//for our printing loop below

	//Didnt have enough file to read the number of items
	if((result = fread(numbers, sizeof(uint16_t), length, fp)) != length)
	{
		printf("Invalid number of items ran out of file\n");
		free(numbers);
		return;
	}
	//print what we read in TODO:double check this is correct representation
	for(;i < length;i++)
	{
		printf("16 bit number %u\n", (short)*temp);
		temp++;
	}
	//printf("Bytes: %d\n", num_bytes);
	free(numbers);
}


//Reads length number of 32 bit integers
void read_thirtytwobit_i(FILE * fp, uint8_t length)
{
	uint32_t * numbers = malloc(sizeof(uint32_t) * length); //allocate space
	uint32_t * temp = numbers; //get an iterating pointer
	int num_bytes = length * 4; //debuging
	int result; 	//for storing number of successful things read by fread
	int i = 0;		//for our printing loop below

	//Didnt have enough file to read the number of items
	if((result = fread(numbers, sizeof(uint32_t), length, fp)) != length)
	{
		printf("Invalid number of items ran out of file\n");
		free(numbers);
		return;
	}
	//print what we read in TODO:double check this is correct representation
	for(;i < length; i++)
	{
		printf("32 bit number %d\n",*temp);
		temp++;
	}

	//printf("Bytes: %d\n", num_bytes);
	free(numbers);
}


//Reads in length number of 32 bit floating point numbers
void read_thirtytwobit_f(FILE * fp, uint8_t length)
{
	uint32_t * numbers = malloc(sizeof(uint32_t) * length);
	uint32_t * temp = numbers;
	int num_bytes = (int)length * 4;

	int result; 	//for storing number of successful things read by fread
	int i = 0;		//for our printing loop below

	//Didnt have enough file to read the number of items
	if((result = fread(numbers, sizeof(uint32_t), length, fp)) != length)
	{
		printf("Invalid number of items ran out of file\n");
		free(numbers);
		return;
	}
	//print what we read in TODO:double check this is correct representation
	for(;i < length; i++)
	{
		printf("32 bit number %f\n",(float)*temp);
		temp++;
	}

	//printf("Bytes: %d\n", num_bytes);
	free(numbers);
}


//Reads in length number of 64 bit floating point numbers
void read_sixtyfourbit_f(FILE * fp, uint8_t length)
{
	uint64_t * numbers = malloc(sizeof(uint64_t) * length);
	uint64_t * temp = numbers;
	int num_bytes = (int)length * 8;

	int result; 	//for storing number of successful things read by fread
	int i = 0;		//for our printing loop below

	//Didnt have enough file to read the number of items
	if((result = fread(numbers, sizeof(uint64_t), length, fp)) != length)
	{
		printf("Invalid number of items ran out of file\n");
		free(numbers);
		return;
	}
	//print what we read in TODO:double check this is correct representation
	for(;i < length; i++)
	{
		printf("64 bit number %f\n",(double)*temp);
		temp++;
	}

	
	//printf("Bytes: %d\n", num_bytes);
	free(numbers);
}


//Reads in length number of bytes and tries to print them, adds '\0' 
void read_ascii(FILE * fp, uint8_t length)
{
	uint8_t * asciichars = malloc((sizeof(uint8_t) * length) +1); //one more for \0
	uint8_t * temp = asciichars;	//iterator pointer
	int num_bytes = (int)length;		//Debug
	int result;
	int i = 0;
	//make sure we have length number of bytes to read in the file
	if((result = fread(asciichars, sizeof(uint8_t), length, fp)) != length)
	{
		printf("Invalid number of items ran out of file\n");
		free(asciichars);
		return;
	}
	temp = temp + length;	//goto end of string
	*temp = '\0'; 			//add the nullbyte
	printf("Ascii chars: %s\n", asciichars);	//prints whole string
	//printf("Bytes: %d\n", num_bytes);	//Debug
	free(asciichars);
}


//iterates through length number of bytes
void read_junk(FILE * fp, uint8_t length)
{
	int num_bytes = (int)length;
	int i = 0;
	for(i; i < length; i++)
	{
		getc(fp); //grabs another char
	}
	printf("Ignored %d Bytes: \n", num_bytes);
}


//Different from skip bit, skips length number of 32 bit integers
void read_skip(FILE * fp, uint8_t length)
{
	int num_bytes = (int)length * 4; //number of 32 bit integers to skip
	int i = 0;
	for(i; i < num_bytes; i++)
	{
		getc(fp);
	}
	printf("Skipped %d 32 bit ints:\n", length);
}


//FYARRR
void read_burn(FILE * fp, uint8_t length)
{
	int num_bytes = (int)length;
	//FYYYARRR
	printf("Bytes: %d\n", num_bytes);
}






