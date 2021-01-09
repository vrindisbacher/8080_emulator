#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef struct ConditionCodes{
	uint8_t z:1; // the zero bit
	uint8_t s:1; // the sign bit
	uint8_t p:1; // the parity bit - if the the number of odd bits is odd -> set to 0; parity odd -> set to 1
	uint8_t cy:1; // carry bit
	uint8_t ac:1; // auxilliary carry bit - carry out of bit 3 THIS IS NEVER USED IN SPACE INVADERS
	uint8_t pad:3; 
	uint8_t interrupt:1;
} ConditionCodes;

typedef struct CPU8080{
	uint8_t a;
	uint8_t b;
	uint8_t c;
	uint8_t d;
	uint8_t e;
	uint8_t h;
	uint8_t l;
	uint16_t sp;
	uint16_t counter;
	unsigned char *memory;
	struct ConditionCodes cc;
	uint8_t int_enable;
} CPU8080;

int parity(unsigned int x, int size){
	int num_bits = 0, i;
	for (i = 0; i < size; i++){
		if (x & (0x01 << i)){
			num_bits++;
		}
	}
	if (num_bits % 2)
		return 0;
	
	return 1;
}

int Emulator(CPU8080 *cpu){
	uint16_t answer, hl, bc, de, address, ptr, ret;
	uint8_t x, psw;
	unsigned char *op = &cpu->memory[cpu->counter];
	dissasemble_hex(cpu->memory, cpu->counter);
	switch (*op){
		case 0x00: cpu->counter ++; break; // NOP
		case 0x01: // LXI B, D16
			cpu->c = op[1];
			cpu->b = op[2];
			cpu->counter ++;
			break;
		case 0x05:  // DCR B
			cpu->b -= 1; 
			cpu->counter ++;
			cpu->cc.z = ((cpu->b & 0xff) == 0); // if cpu-> b is 0 we set this bit to 1
			cpu->cc.s = ((cpu->b & 0x80) != 0); // last bit in 8 bit number is 1 the sign is negative
			cpu->cc.p = parity(cpu->b, 8); // sets to 1 if even parity 
			break;
		case 0x06: // MVI B, D8
			cpu->b = op[1];
			cpu->counter ++;
			break;
		case 0x09: // DAD B -- This means H and L iss HL + BC: ***Little Endian Computer***
			hl = cpu->l | (cpu->h << 8);
			bc = cpu->c | (cpu->b << 8);
			answer = hl + bc;
			cpu->h =(answer >> 8) & 0xff;
			cpu->l = answer & 0x00ff;
			cpu->cc.cy = answer > 0xffff;
			cpu->counter ++;
			break;
		case 0x0d: // DCR C
			cpu->c -= 1;
			cpu->counter ++;
			cpu->cc.z = ((cpu->c & 0xff) == 0);
			cpu->cc.s = ((cpu->c & 0x80) != 0);
			cpu->cc.p = parity(cpu->c, 8);
			break;
		case 0x0e: // MVI C, D8 
			cpu->c = op[1];
			cpu->counter ++;
			break;
		case 0x0f: // RRC
			uint8_t x = cpu->a;
			cpu->a = ((x & 0x01) << 7) | (x >> 1);
			cpu->cc.cy = (1 == (x&1));
			break;
		case 0x11: // LXI D, D16 
			cpu->e = op[1];
			cpu->d = op[2];
			cpu->counter ++;
			break;
		case 0x13: // INX D
			de = cpu->e | (cpu->d << 8);
			answer = de + 1;
			cpu->d = (answer >> 8) & 0xff;
			cpu->e = answer & 0x00ff;
			cpu->counter ++;
			break;
		case 0x19: // DAD D
			hl = cpu->l | (cpu->h << 8);;
			de = cpu->e | (cpu->d << 8);
			answer = hl + de;
			cpu->h = (answer >> 8) & 0xff;
			cpu->l =  answer & 0x00ff;
			cpu->cc.cy = answer > 0xffff;
			cpu->counter ++;
			break;
		case 0x20: cpu->counter ++; break; // NOP
		case 0x1a: // LDAX D 
			de = cpu->e | (cpu->d << 8);
			cpu->a = cpu->memory[de];
			cpu->counter ++;
			break;
		case 0x1b: // DCX D 
			de = cpu->e | (cpu->d << 8);
			answer = de - 1;
			cpu->d = (answer >> 8) & 0xff;
			cpu->e = answer & 0x00ff;
			cpu->counter ++;
			break;
		case 0x21: // LXI H, D16 
			cpu->l = op[1];
			cpu->h = op[2];
			cpu->counter ++;
			break;
		case 0x23: // INX H 
			hl = cpu->l | (cpu->h << 8);
			answer = hl + 1;
			cpu->h = (answer >> 8) & 0xff;
			cpu->l = answer & 0x00ff;
			cpu->counter ++;
			break;
		case 0x24: // INR H 
			cpu->h += 1;
			cpu->cc.z = ((cpu->h & 0xff) == 0); // if cpu-> b is 0 we set this bit to 1
			cpu->cc.s = ((cpu->h & 0x80) != 0); // last bit in 8 bit number is 1 the sign is negative
			cpu->cc.p = parity(cpu->h, 8); // sets to 1 if even parity 
			cpu->counter++;
			break;
			
		case 0x26: // MVI H D8 
			cpu->h = op[1];
			cpu->counter ++; 
			break;
		case 0x29: // DAD H 
			hl = cpu->l | (cpu->h << 8);
			answer = hl << 1;
			cpu->l = (answer >> 8) & 0xff;
			cpu->h =  answer & 0x00ff;
			cpu->cc.cy = answer > 0xffff; 
			cpu->counter ++;
			break;
		case 0x31: // LXI SP D16 
			ptr = op[1] | (op[2] << 8);
			cpu->sp = ptr;
			cpu->counter ++;
			break;
		case 0x32: // STA adr;
			address = op[1] | (op[2] << 8);
			cpu->memory[address] = cpu->a;
			cpu->counter ++;
			break; 
		case 0x36: // MVI M, D8
			hl = cpu->l | (cpu->h << 8);
			cpu->memory[hl] = op[1];
			cpu->counter ++;
			break;
		case 0x3a: // LDA adr 
			address = (op[2] << 8) | op[1];
			cpu->a = cpu->memory[address];
			cpu->counter ++;
			break;
		case 0x3e: // MVI A, D8 
			cpu->a = op[1];
			cpu->counter ++;
			break;
		case 0x56: // MOV D, M
			hl = cpu->l | (cpu->h << 8);
			cpu->d = cpu->memory[hl];
			cpu->counter ++;
			break;
		case 0x5e: // MOV E,M
			hl = cpu->l | (cpu->h << 8);
			cpu->e = cpu->memory[hl];
			cpu->counter ++;
			break;
		case 0x66: // MOV H,M 
			hl = cpu->l | (cpu->h << 8);
			cpu->h = cpu->memory[hl];
			cpu->counter ++;
			break;
		case 0x6f: 
			cpu->l = cpu->a;
			cpu->counter ++;
			break;
		case 0x77: // MOV M,A 
			hl = cpu->l | (cpu->h << 8);
			cpu->memory[hl] = cpu->a;
			cpu->counter ++;
			break;
		case 0x7a: // MOV A,D
			cpu->a = cpu->d;
			cpu->counter ++;
			break;
		case 0x7b:
			cpu->a = cpu->e;
			cpu->counter ++;
			break;
		case 0x7c:
			cpu->a = cpu->h;
			cpu->counter ++;
			break;
		case 0x7e:
			hl = cpu->l | (cpu->h << 8);
			cpu->a = cpu->memory[hl];
			cpu->counter ++;
			break;
		case 0xa7: // ANA A 
			x = cpu->a & cpu->a;
			cpu->a = x;
			cpu->cc.z = (x == 0);
			cpu->cc.s = (0x80 == (x & 0x80));
			cpu->cc.p = parity(x, 8);
			cpu->cc.cy = 0;
			cpu->counter ++;
			break;
		case 0xaf: // XRA A 
			x = cpu->a ^ cpu->a;
			cpu->a = x;
			cpu->cc.z = (x == 0);
			cpu->cc.s = (0x80 == (x & 0x80));
			cpu->cc.p = parity(x, 8);
			cpu->cc.cy = 0;
			cpu->counter ++;
			break;
		case 0xb6: // ORA M 
			hl = cpu->l | (cpu->h << 8);
			x = cpu->a | hl;
			cpu->a = x;
			cpu->cc.z = (x == 0);
			cpu->cc.s = (0x80 == (x & 0x80));
			cpu->cc.p = parity(x, 8);
			cpu->cc.cy = 0;
			cpu->counter ++;
			break;
		case 0xc1: // POP B 
			cpu->c = cpu->memory[cpu->sp];
			cpu->b = cpu->memory[cpu->sp + 1];
			cpu->sp += 2;
			cpu->counter ++;
			break;
		case 0xc2: // JNZ adr 
			if (cpu->cc.z == 0){
				cpu->counter = op[1] | (op[2] << 8);
			} else {
				cpu->counter ++; 
			}
			break;
		case 0xc3: // JMP adr
			cpu->counter = op[1] | (op[2] << 8);
			break;
		case 0xc5: // PUSH B 
			cpu->memory[cpu->sp - 2] = cpu->c; 
			cpu->memory[cpu->sp - 1] = cpu->b;
			cpu->sp -= 2; 
			cpu->counter ++;
			break;
		case 0xc6: // ADI D8 
			x = cpu->a + op[1];
			cpu->a = x;
			cpu->cc.z = (x == 0);
			cpu->cc.s = (0x80 == (x & 0x80));
			cpu->cc.p = parity(x, 8);
			cpu->cc.cy = (x > 0xff);
			cpu->counter ++;
			break;
		case 0xc9: // RET 
			cpu->counter = cpu->memory[cpu->sp] | (cpu->memory[cpu->sp + 1] << 8);
			cpu->sp += 2;
			cpu->counter ++;
			break;
		case 0xcd: // CALL adr
			ret = cpu->sp + 2;
			cpu->memory[cpu->sp + 1] = (ret >> 8) & 0xff;
			cpu->memory[cpu->sp - 2] = (ret & 0xff);
			cpu->sp -= 2;
			cpu->counter = op[1] | (op[2] << 8);
			break;
		case 0xd1: // POP D 
			cpu->e = cpu->memory[cpu->sp];
			cpu->d = cpu->memory[cpu->sp + 1];
			cpu->sp += 2;
			cpu->counter ++;
			break;
		case 0xd3: // OUT D8 
			cpu->counter ++;
			break;
		case 0xd5: // PUSH D 
			cpu->memory[cpu->sp - 2] = cpu->e; 
			cpu->memory[cpu->sp - 1] = cpu->d;
			cpu->sp -= 2; 
			cpu->counter ++;
			break;
		case 0xe1: // POP H 
			cpu->l = cpu->memory[cpu->sp];
			cpu->h = cpu->memory[cpu->sp + 1];
			cpu->sp += 2;
			cpu->counter ++;
			break;
		case 0xe5: // PUSH H 
			cpu->memory[cpu->sp - 2] = cpu->l; 
			cpu->memory[cpu->sp - 1] = cpu->h;
			cpu->sp -= 2; 
			cpu->counter ++;
			break;
		case 0xe6: // ANI D8 
			x = cpu->a & op[1];
			cpu->a = x;
			cpu->cc.z = (x == 0);
			cpu->cc.s = (0x80 == (x & 0x80));
			cpu->cc.p = parity(x, 8);
			cpu->cc.cy = 0;
			cpu->counter ++;
			break;
		case 0xeb: // XCHG 
			uint8_t copy_h = cpu->h;
			uint8_t copy_l = cpu->l;
			cpu->h = cpu->d;
			cpu->d = copy_h;
			cpu->l = cpu->e;
			cpu->e = copy_l;
			cpu->counter ++;
			break;
		case 0xf1: // POP PSW 
			cpu->a = cpu->memory[cpu->sp + 1];
			psw = cpu->memory[cpu->sp];
			cpu->cc.z = (0x01 == (psw & 0x01));
			cpu->cc.s = (0x02 == (psw & 0x02));
			cpu->cc.p = (0x04 == (psw & 0x04));
			cpu->cc.cy = (0x05 == (psw & 0x08));
			cpu->cc.ac = (0x10 == (psw & 0x10));
			cpu->sp += 2;
			cpu->counter ++;
			break;
		case 0xf5: // PUSH PSW 
			cpu->memory[cpu->sp - 1] = cpu->a;
			psw = (cpu->cc.z | cpu->cc.s << 1 | cpu->cc.p << 2 | cpu->cc.cy << 3 | cpu->cc.ac << 4);
			cpu->memory[cpu->sp - 2] = psw;
			cpu->sp -= 2;
			cpu->counter ++;
			break;
		case 0xfb: // EI 
			cpu->cc.interrupt = 1; 
			cpu->counter ++;
			break;
		case 0xfe: // CPI D8 
			x = cpu->a - op[1]; 
			cpu->cc.z = (x == 0);
			cpu->cc.s = (0x80 == (x & 0x80));
			cpu->cc.p = parity(x, 8);
			cpu->cc.cy = (cpu->a < op[1]);
			cpu->counter ++;
			break;	
		default:
			printf("Not Implemented\n");
			cpu->counter++;
			break;
	}
	printf("\tC=%d,P=%d,S=%d,Z=%d\n", cpu->cc.cy, cpu->cc.p,    
           cpu->cc.s, cpu->cc.z);    
    printf("\tA $%02x B $%02x C $%02x D $%02x E $%02x H $%02x L $%02x SP %04x\n",    
           cpu->a, cpu->b, cpu->c, cpu->d,    
           cpu->e, cpu->h, cpu->l, cpu->sp);
	return 0;
}
	
	




/*
dissasemble_hex:
*hexbuffer points to 8080 assembly code.
byteoffset is the offset in the hex file. 

returns the number of bytes of current operation
*/

int dissasemble_hex(unsigned char *hexbuffer, int byteoffset){
	unsigned char *code = &hexbuffer[byteoffset];
	int opbytes = 1;
	
	// "%04x " is the format string for a hex int with four digits and leading zeros
	printf("%04x ", byteoffset);
	switch (*code){
		case 0x00: printf("NOP\n"); break;
		case 0x01: printf("LXI B,#$%02x%02x\n", code[2], code[1]); opbytes=3;break; 
		case 0x02: printf("STAX B\n"); break;
		case 0x03: printf("INX B\n"); break;
		case 0x04: printf("INR B\n"); break;
		case 0x05: printf("DCR B\n"); break;
		case 0x06: printf("MVI B,#$%02x\n", code[1]); opbytes=2; break;
		case 0x07: printf("RLC\n"); break;
		case 0x08: printf("NOP\n"); break;
		case 0x09: printf("DAD B\n"); break;
		case 0x0a: printf("LDAX B\n"); break;
		case 0x0b: printf("DCX B\n"); break;
		case 0x0c: printf("INR C\n"); break;
		case 0x0d: printf("DCR C\n"); break;
		case 0x0e: printf("MVI C,#$%02x\n", code[1]); opbytes=2; break;
		case 0x0f: printf("RRC\n"); break;
		case 0x10: printf("NOP\n");
		case 0x11: printf("LXI D,#$%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0x12: printf("STAX D\n"); break;
		case 0x13: printf("INX D\n"); break;
		case 0x14: printf("INR D\n"); break;
		case 0x15: printf("DCR D\n"); break;
		case 0x16: printf("MVI D,#$%02x\n", code[1]); opbytes=2; break;
		case 0x17: printf("RAL\n"); break;
		case 0x18: printf("NOP\n"); break;
		case 0x19: printf("DAD D\n"); break;
		case 0x1a: printf("LDAX D\n"); break;
		case 0x1b: printf("DCX D\n"); break;
		case 0x1c: printf("INR E\n"); break;
		case 0x1d: printf("DCR E\n"); break;
		case 0x1e: printf("MVI E,#$%02x\n", code[1]); opbytes=2; break;
		case 0x1f: printf("RAR\n"); break;
		case 0x20: printf("NOP\n"); break;
		case 0x21: printf("LXI H,#$%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0x22: printf("SHLD $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0x23: printf("INX H\n"); break;
		case 0x24: printf("INR H\n"); break;
		case 0x25: printf("DCR H\n"); break;
		case 0x26: printf("MVI H,#$%02x\n", code[1]); opbytes=2; break;
		case 0x27: printf("DAA\n"); break;
		case 0x28: printf("NOP\n"); break;
		case 0x29: printf("DAD H\n"); break;
		case 0x2a: printf("LHLD $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0x2b: printf("DCX H\n"); break;
		case 0x2c: printf("INR L\n"); break;
		case 0x2d: printf("DCR L\n"); break;
		case 0x2e: printf("MVI L,#$%02x\n", code[1]); opbytes=2; break;
		case 0x2f: printf("CMA\n"); break;
		case 0x30: printf("SIM\n"); break;
		case 0x31: printf("LXI SP,#$%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0x32: printf("STA $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0x33: printf("INX SP\n"); break;
		case 0x34: printf("INR M\n"); break;
		case 0x35: printf("DCR M\n"); break;
		case 0x36: printf("MVI M,#$%02x\n", code[1]); opbytes=2; break;
		case 0x37: printf("STC\n"); break;
		case 0x38: printf("NOP\n"); break;
		case 0x39: printf("DAD SP\n"); break;
		case 0x3a: printf("LDA $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0x3b: printf("DCX SP\n"); break;
		case 0x3c: printf("INR A\n"); break;
		case 0x3d: printf("DCR A\n"); break;
		case 0x3e: printf("MVI A,#$%02x\n", code[1]); opbytes=2; break;
		case 0x3f: printf("CMC\n"); break;
		case 0x40: printf("MOV B,B\n"); break;
		case 0x41: printf("MOV B,C\n"); break;
		case 0x42: printf("MOV B,D\n"); break;
		case 0x43: printf("MOV B,E\n"); break;
		case 0x44: printf("MOV B,H\n"); break;
		case 0x45: printf("MOV B,L\n"); break;
		case 0x46: printf("MOV B,M\n"); break;
		case 0x47: printf("MOV B,A\n"); break;
		case 0x48: printf("MOV C,B\n"); break;
		case 0x49: printf("MOV C,C\n"); break;
		case 0x4a: printf("MOV C,D\n"); break;
		case 0x4b: printf("MOV C,E\n"); break;
		case 0x4c: printf("MOV C,H\n"); break;
		case 0x4d: printf("MOV C,L\n"); break;
		case 0x4e: printf("MOV C,M\n"); break;
		case 0x4f: printf("MOV C,A\n"); break;
		case 0x50: printf("MOV D,B\n"); break;
		case 0x51: printf("MOV D,C\n"); break;
		case 0x52: printf("MOV D,D\n"); break;
		case 0x53: printf("MOV D,E\n"); break;
		case 0x54: printf("MOV D,H\n"); break;
		case 0x55: printf("MOV D,L\n"); break;
		case 0x56: printf("MOV D,M\n"); break;
		case 0x57: printf("MOV D,A\n"); break;
		case 0x58: printf("MOV E,B\n"); break;
		case 0x59: printf("MOV E,C\n"); break;
		case 0x5a: printf("MOV E,D\n"); break;
		case 0x5b: printf("MOV E,E\n"); break;
		case 0x5c: printf("MOV E,H\n"); break;
		case 0x5d: printf("MOV E,L\n"); break;
		case 0x5e: printf("MOV E,M\n"); break;
		case 0x5f: printf("MOV E,A\n"); break;
		case 0x60: printf("MOV H,B\n"); break;
		case 0x61: printf("MOV H,C\n"); break;
		case 0x62: printf("MOV H,D\n"); break;
		case 0x63: printf("MOV H,E\n"); break;
		case 0x64: printf("MOV H,H\n"); break;
		case 0x65: printf("MOV H,:\n"); break;
		case 0x66: printf("MOV H,M\n"); break;
		case 0x67: printf("MOV H,A\n"); break;
		case 0x68: printf("MOV L,B\n"); break;
		case 0x69: printf("MOV L,C\n"); break;
		case 0x6a: printf("MOV L,D\n"); break;
		case 0x6b: printf("MOV L,E\n"); break;
		case 0x6c: printf("MOV L,H\n"); break;
		case 0x6d: printf("MOV L,L\n"); break;
		case 0x6e: printf("MOV L,M\n"); break;
		case 0x6f: printf("MOV L,A\n"); break;
		case 0x70: printf("MOV M,B\n"); break;
		case 0x71: printf("MOV M,C\n"); break;
		case 0x72: printf("MOV M,D\n"); break;
		case 0x73: printf("MOV M,E\n"); break;
		case 0x74: printf("MOV M,H\n"); break;
		case 0x75: printf("MOV M,L\n"); break;
		case 0x76: printf("HLT\n"); break;
		case 0x77: printf("MOV M,A\n"); break;
		case 0x78: printf("MOV A,B\n"); break;
		case 0x79: printf("MOV A,C\n"); break;
		case 0x7a: printf("MOV A,D\n"); break;
		case 0x7b: printf("MOV A,E\n"); break;
		case 0x7c: printf("MOV A,H\n"); break;
		case 0x7d: printf("MOV A,L\n"); break;
		case 0x7e: printf("MOV A,M\n"); break;
		case 0x7f: printf("MOV A,A\n"); break;
		case 0x80: printf("ADD B\n"); break;
		case 0x81: printf("ADD C\n"); break;
		case 0x82: printf("ADD D\n"); break;
		case 0x83: printf("ADD D\n"); break;
		case 0x84: printf("ADD E\n"); break;
		case 0x85: printf("ADD H\n"); break;
		case 0x86: printf("ADD M\n"); break;
		case 0x87: printf("ADD A\n"); break;
		case 0x88: printf("ADC B\n"); break;
		case 0x89: printf("ADC C\n"); break;
		case 0x8a: printf("ADC D\n"); break;
		case 0x8b: printf("ADC E\n"); break;
		case 0x8c: printf("ADC H\n"); break;
		case 0x8d: printf("ADC L\n"); break;
		case 0x8e: printf("ADC M\n"); break;
		case 0x8f: printf("ADC A\n"); break;
		case 0x90: printf("SUB B\n"); break;
		case 0x91: printf("SUB C\n"); break;
		case 0x92: printf("SUB D\n"); break;
		case 0x93: printf("SUB E\n"); break;
		case 0x94: printf("SUB H\n"); break;
		case 0x95: printf("SUB L\n"); break;
		case 0x96: printf("SUB M\n"); break;
		case 0x97: printf("SUB A\n"); break;
		case 0x98: printf("SBB B\n"); break;
		case 0x99: printf("SBB C\n"); break;
		case 0x9a: printf("SBB D\n"); break;
		case 0x9b: printf("SBB E\n"); break;
		case 0x9c: printf("SBB H\n"); break;
		case 0x9d: printf("SBB L\n"); break;
		case 0x9e: printf("SBB E\n"); break;
		case 0x9f: printf("SBB A\n"); break;
		case 0xa0: printf("ANA B\n"); break;
		case 0xa1: printf("ANA C\n"); break;
		case 0xa2: printf("ANA D\n"); break;
		case 0xa3: printf("ANA E\n"); break;
		case 0xa4: printf("ANA H\n"); break;
		case 0xa5: printf("ANA L\n"); break;
		case 0xa6: printf("ANA M\n"); break;
		case 0xa7: printf("ANA A\n"); break;
		case 0xa8: printf("XRA B\n"); break;
		case 0xa9: printf("XRA C\n"); break;
		case 0xaa: printf("XRA D\n"); break;
		case 0xab: printf("XRA E\n"); break;
		case 0xac: printf("XRA D\n"); break;
		case 0xad: printf("XRA L\n"); break;
		case 0xae: printf("XRA M\n"); break;
		case 0xaf: printf("XRA A\n"); break;
		case 0xb0: printf("ORA B\n"); break;
		case 0xb1: printf("ORA C\n"); break;
		case 0xb2: printf("ORA D\n"); break;
		case 0xb3: printf("ORA E\n"); break;
		case 0xb4: printf("ORA H\n"); break;
		case 0xb5: printf("ORA L\n"); break;
		case 0xb6: printf("ORA M\n"); break;
		case 0xb7: printf("ORA A\n"); break;
		case 0xb8: printf("CMP B\n"); break;
		case 0xb9: printf("CMP C\n"); break;
		case 0xba: printf("CMP D\n"); break;
		case 0xbb: printf("CMP E\n"); break;
		case 0xbc: printf("CMP H\n"); break;
		case 0xbd: printf("CMP L\n"); break;
		case 0xbe: printf("CMP M\n"); break;
		case 0xbf: printf("CMP A\n"); break;
		case 0xc0: printf("RNZ\n"); break;
		case 0xc1: printf("POP B\n"); break;
		case 0xc2: printf("JNZ $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xc3: printf("JMP $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xc4: printf("CNZ $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xc5: printf("PUSH B\n"); break;
		case 0xc6: printf("ADI #$%02x\n", code[1]); opbytes=2; break;
		case 0xc7: printf("RST 0\n"); break;
		case 0xc8: printf("RZ\n"); break;
		case 0xc9: printf("RET\n"); break;
		case 0xca: printf("JZ $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xcb: printf("NOP\n"); break;
		case 0xcc: printf("CZ $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xcd: printf("CALL $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xce: printf("ACI #$%02x\n", code[1]); opbytes=2; break;
		case 0xcf: printf("CNC $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xd0: printf("RNC\n"); break;
		case 0xd1: printf("POP D\n"); break;
		case 0xd2: printf("JNC $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xd3: printf("OUT #$%02x", code[1]); opbytes=2; break;
		case 0xd4: printf("CNC $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xd5: printf("PUSH D\n"); break;
		case 0xd6: printf("SUI #$%02x\n", code[1]); opbytes=2; break;
		case 0xd7: printf("RST 2\n"); break;
		case 0xd8: printf("RC\n"); break;
		case 0xd9: printf("NOP\n"); break;
		case 0xda: printf("JC $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xdb: printf("IN #$%02x\n", code[1]); opbytes=2; break;
		case 0xdc: printf("CC $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xdd: printf("NOP\n"); break;
		case 0xde: printf("SBI #$%02x\n", code[1]); opbytes=2; break;
		case 0xdf: printf("RST 3\n"); break;
		case 0xe0: printf("RPO\n"); break;
		case 0xe1: printf("POP H\n"); break;
		case 0xe2: printf("JPO $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xe3: printf("XTHL\n"); break;
		case 0xe4: printf("CPO $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xe5: printf("PUSH H\n"); break;
		case 0xe6: printf("ANI #$%02x\n", code[1]); opbytes=2; break;
		case 0xe7: printf("RST 4\n"); break;
		case 0xe8: printf("RPE\n"); break;
		case 0xe9: printf("PCHL\n"); break;
		case 0xea: printf("JPE $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xeb: printf("XCHG\n"); break;
		case 0xec: printf("CPE $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xed: printf("NOP\n"); break;
		case 0xee: printf("XRI #$%02x\n", code[1]); opbytes=2; break;
		case 0xef: printf("RST 5\n"); break;
		case 0xf0: printf("RP\n"); break;
		case 0xf1: printf("POP PSW\n"); break;
		case 0xf2: printf("JP $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xf3: printf("DI\n"); break;
		case 0xf4: printf("CP $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xf5: printf("PUSH PSW\n"); break;
		case 0xf6: printf("ORI #$02c\n"); opbytes=2; break;
		case 0xf7: printf("RST 6\n"); break;
		case 0xf8: printf("RM\n"); break;
		case 0xf9: printf("SPHL\n"); break;
		case 0xfa: printf("JM $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xfb: printf("EI\n"); break;
		case 0xfc: printf("CM $%02x%02x\n", code[2], code[1]); opbytes=3; break;
		case 0xfd: printf("NOP\n"); break;
		case 0xfe: printf("CPI #$%02x\n", code[1]); opbytes=2; break;
		case 0xff: printf("RST 7\n"); break;
	}
	
	return opbytes;
}


int main(){

	
	CPU8080 *cpu = malloc(sizeof(CPU8080));
	cpu->memory = malloc(0x10000);	
	cpu->counter=0;
	cpu->a=0;
	cpu->b=0;
	cpu->c=0;
	cpu->d=0;
	cpu->e=0;
	cpu->h=0;
	cpu->l=0;
	cpu->sp=0;
	
	FILE *file = fopen("source", "rb");
	if (file == NULL){
		printf("oops");
		exit(1);
	}
	fseek(file, 0L, SEEK_END);
	int file_size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	
	unsigned char *buff = &cpu->memory[0];
	fread(buff, file_size, 1, file);
	fclose(file);
	
	while (1){
		Emulator(cpu);
	}
	
	
	return 0;
}
	