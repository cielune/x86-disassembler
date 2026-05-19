#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


/* ========== doutes ou questions ==========
- get_ea    => pourquoi static pour le buffer ;
            => pourquoi : disp = (high << 8) | low;
============================================*/


/* ========== format de debug ========== 
printf("[GET_REG DEBUG] -----> w!=0 || w!=1"); 
========================================*/


// ======== prototypes & struct ========
typedef struct {
    int mod;
    int reg;
    int rm;
    char *reg_name;
    char *rm_name; //or ea_name
} ModRM;
long openBinaryFile(char *filename, FILE** fileptr);
ModRM decode_modrm(FILE* file, int w);
char* get_reg(int reg, int w);
char* get_seg(int reg);
uint8_t disp8(FILE * file);
uint16_t disp16(FILE * file);
char* get_ea(FILE* file, int rm, int mod);
void disasmbBase(FILE * file, long text_size);
void instructionDecoding(FILE * file);
// ===================================



int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("[MAINDECODING] please enter a valid a.out filename.\n");
        exit(EXIT_FAILURE);
    }

    FILE* file;
    long text_size = openBinaryFile(argv[1], &file);

    if (file==NULL)
    {
        printf("[MAINDECODING] error, could not open file.\n");
        exit(EXIT_FAILURE);
    }
    disasmbBase(file,text_size);

    fclose(file);
    return EXIT_SUCCESS; 
}


long openBinaryFile(char *filename, FILE** fileptr)
{
    int32_t a_text;

    *fileptr= fopen(filename,"rb");
    if (*fileptr==NULL)
    {
        printf("%s cannot be opened\n", filename);
        exit(EXIT_FAILURE);
    }
    fseek(*fileptr, 8, SEEK_SET);

    fread(&a_text,sizeof(int32_t),1, *fileptr);
    return a_text;
}


ModRM decode_modrm(FILE* file, int w)
{
    ModRM m;
    unsigned char byte;
    fread(&byte,1,1,file);

    m.mod= (byte >> 6) & 0b11;
    m.reg= (byte >> 3) & 0b111;
    m.rm= byte & 0b111;

    m.reg_name=get_reg(m.reg,w);
    if (m.mod==0b11)
    {
        m.rm_name= get_reg(m.rm,w);
    }
    else
    {
        m.rm_name=get_ea(file,m.rm,m.mod);
    }
    return m;
}


char* get_reg(int reg, int w)
{
    if (w==1)
    {
        switch (reg)
        {
            case 0b000: return "AX";
            case 0b001: return "CX";
            case 0b010: return "DX";
            case 0b011: return "BX";
            case 0b100: return "SP";
            case 0b101: return "BP";
            case 0b110: return "SI";
            case 0b111: return "DI";
            default:
                printf("[GET_REG DEBUG] -----> REG 16-Bit inconnu\n");
                return NULL;
                break;
        }
    }
    else if (w==0)
    {
        switch (reg)
        {
            case 0b000: return "AL";
            case 0b001: return "CL";
            case 0b010: return "DL";
            case 0b011: return "BL";
            case 0b100: return "AH";
            case 0b101: return "CH";
            case 0b110: return "DH";
            case 0b111: return "BH";
            default:
                printf("[GET_REG DEBUG] -----> REG 8-Bit inconnu");
                return NULL;
                break;
        }
    }
    else
    {
        printf("[GET_REG DEBUG] -----> w!=0 || w!=1");
        return NULL;
    }
}


char* get_seg(int seg)
{
    switch (seg)
    {
        case 0b00: return "ES";
        case 0b01: return "CS";
        case 0b10: return "SS";
        case 0b11: return "DS";
        default:
            printf("[GET_SEG DEBUG] -----> SEG (%i) inconnu\n", seg);
            return "UNKNOWN_SEG";
    }
}


uint8_t disp8(FILE* file)
{
    uint8_t val;
    fread(&val,1,1,file);
    return val;
}
uint16_t disp16(FILE* file)
{
    uint16_t disp;
    uint8_t displow;
    uint8_t disphigh;
    fread(&displow,1,1,file);
    fread(&disphigh,1,1,file);
    disp= (disphigh << 8) | displow;
    return disp;
}
char* get_ea(FILE* file, int rm, int mod)
{
    static char buffer[64];
    uint16_t d16;
    uint8_t d8;

    if (mod==0b00)
    {
        if (rm==0b110)
        {
            sprintf(buffer,"[0x%04x]",disp16(file));
        }
        else
        {
            //DISP= 0* donc pas de deplacement
            switch (rm)
            {
                case 0b000: sprintf(buffer,"[bx+si]"); break;
                case 0b001: sprintf(buffer,"[bx+di]"); break;
                case 0b010: sprintf(buffer,"[bp+si]"); break;
                case 0b011: sprintf(buffer,"[bp+di]"); break;
                case 0b100: sprintf(buffer,"[si]"); break;
                case 0b101: sprintf(buffer,"[di]"); break;
                case 0b111: sprintf(buffer,"[bx]"); break;
                default:
                    printf("[GET_EA DEBUG] -----> r/m (mod=00) inconnu | \t r/m = %i\n",rm);
                    break;
            }
        }
    }
    else if (mod==0b01)
    {
        //========== 8-bit DISP ==========
        d8=disp8(file);
        switch (rm)
        {
            case 0b000: sprintf(buffer,"[bx+si+0x%02x]",d8); break;
            case 0b001: sprintf(buffer,"[bx+di+0x%02x]",d8); break;
            case 0b010: sprintf(buffer,"[bp+si+0x%02x]",d8); break;
            case 0b011: sprintf(buffer,"[bp+di+0x%02x]",d8); break;
            case 0b100: sprintf(buffer,"[si+0x%02x]",d8); break;
            case 0b101: sprintf(buffer,"[di+0x%02x]",d8); break;
            case 0b110: sprintf(buffer,"[bp+0x%02x]",d8); break;
            case 0b111: sprintf(buffer,"[bx+0x%02x]",d8); break;
            default:
                printf("[GET_EA DEBUG] -----> r/m (mod=01) inconnu | \t r/m = %i\n",rm);
                break;
        }
    }
    else if (mod==0b10)
    {
        // ========== 16-bit DISP ==========
        d16=disp16(file);
        switch (rm)
        {
            case 0b000: sprintf(buffer,"[bx+si+0x%04x]",d16); break;
            case 0b001: sprintf(buffer,"[bx+di+0x%04x]",d16); break;
            case 0b010: sprintf(buffer,"[bp+si+0x%04x]",d16); break;
            case 0b011: sprintf(buffer,"[bp+di+0x%04x]",d16); break;
            case 0b100: sprintf(buffer,"[si+0x%04x]",d16); break;
            case 0b101: sprintf(buffer,"[di+0x%04x]",d16); break;
            case 0b110: sprintf(buffer,"[bp+0x%04x]",d16); break;
            case 0b111: sprintf(buffer,"[bx+0x%04x]",d16); break;
            default:
                printf("[GET_EA DEBUG] -----> r/m (mod=10) inconnu | \t r/m = %i\n",rm);
                break;
        }
        
    }
    else
    {
        printf("[GET_EA DEBUG] -----> mod!=00 || mod!=01 || mod!=10 | \t mod = %i\n",mod);
    }

    return buffer;
}


void disasmbBase(FILE* file, long text_size)
{
    fseek(file, 32, SEEK_SET);
    long curr_pos=0;

    while (curr_pos<text_size)
    {
        long avant=ftell(file);
        /*uint16_t address=(uint16_t)(start_pos-32);
        printf("%04x: ", addr);*/

        instructionDecoding(file);
        long apres=ftell(file);
        if (apres==avant)
        {
            unsigned char idk;
            fread(&idk,1,1,file);
            printf("[DISASMBBASEDECODING] opcode inconnu (0x%02x), skipped.\n",idk);
            curr_pos+=1;
        }
        curr_pos+=(apres-avant);
    }      
}


int decode8bits(FILE *file, unsigned char curr,int w, int d)
{
    switch(curr)
    {
    // =================================================== page 1 ===================================================
        case 0b10001110:
        {
            ModRM m=decode_modrm(file,1);
            printf("MOV %s, %s\n",get_seg(m.reg),m.rm_name);
            break;
        }
        case 0b10001100:
        {
            ModRM m=decode_modrm(file,1);
            printf("MOV %s, %s\n",m.rm_name,get_seg(m.reg));
            break;
        }
        case 0b11111111:
        {
            ModRM m=decode_modrm(file,1);
            switch (m.reg)
            {
                case 0b110:
                {
                    printf("PUSH %s\n",m.rm_name);
                    break;
                }
                case 0b010:
                {
                    printf("CALL %s\n",m.rm_name);
                    break;
                }
                case 0b011:
                {
                    printf("CALL FAR %s\n",m.rm_name); 
                    break;
                }
                case 0b100:
                {
                    printf("JMP %s\n",m.rm_name); 
                    break;
                }
                case 0b101:
                {
                    printf("JMP FAR %s\n",m.rm_name);
                    break;
                }
                default:
                    printf("[DECODE8BITS DEBUG] -----> case 0b11111111: unknown m.reg (%i)\n",m.reg);
                    break;
            }
            break;
        }
        case 0b10001111:
        {
            ModRM m=decode_modrm(file,1);
            if (m.reg==0b000)
            {
                printf("POP %s\n",m.rm_name);
            }
            else
            {
                printf("[DECODE8BITS DEBUG] -----> case 0b10001111: unknown m.reg (%i)",m.reg);
            }
            break;
        }
        case 0b11010111:
        {
            printf("XLAT\n");
            break;
        }
        case 0b10001101:
        {
            ModRM m=decode_modrm(file,1);
            printf("LEA %s, %s\n",m.reg_name,m.rm_name);
            break;
        }
        case 0b11000101:
        {
            ModRM m=decode_modrm(file,1);
            printf("LDS %s, %s\n",m.reg_name,m.rm_name);
            break;
        }
        case 0b11000100:
        {
            ModRM m=decode_modrm(file,1);
            printf("LES %s, %s\n",m.reg_name,m.rm_name);
            break;
        }
        case 0b10011111:
        {
            printf("LAHF\n");
            break;
        }
        case 0b10011110:
        {
            printf("SAHF\n");
            break;
        }
        case 0b10011100:
        {
            printf("PUSHF\n");
            break;
        }
        case 0b10011101:
        {
            printf("POPF\n");
            break;
        }


    // =================================================== page 2 ===================================================
        case 0b00110111:
        {
            printf("AAA\n");
            break;
        }
        case 0b00100111:
        {
            printf("BAA\n");
            break;
        }
        case 0b00111111:
        {
            printf("AAS\n");
            break;
        }
        case 0b00101111:
        {
            printf("DAS\n");
            break;
        }
        case 0b11010100:
        {
            unsigned char tmp;
            fread(&tmp,1,1,file);
            printf("AAM\n"); 
            break;
        }
        case 0b11010101:
        {
            unsigned char tmp;
            fread(&tmp,1,1,file);
            printf("AAD\n"); 
            break;
        }
        case 0b10011000:
        {
            printf("CBW\n");
            break;
        }
        case 0b10011001:
        {
            printf("CWD\n");
            break;
        }


    // =================================================== page 3 ===================================================
        case 0b11101000:
        {
            printf("CALL 0x%04x\n", disp16(file));
            break;
        }
        /*case 11111111 mod 010 géré ==> p1*/
        case 0b10011010:
        {
            uint16_t offset= disp16(file);
            uint16_t segment= disp16(file);
            printf("CALL 0x%04x:0x%04x\n",segment,offset);
            break;
        }
        /*case 11111111 mod 011 géré ==> p1*/
    

    // =================================================== page 4 ===================================================
        case 0b11101001:
        {
            printf("JMP 0x%04x\n", disp16(file));
            break;
        }
        case 0b11101011:
        {
            printf("JMP 0x%02x\n", disp8(file));
            break;
        }
        /*case 11111111 mod 100 géré ==> p1*/
        case 0b11101010:
        {
            uint16_t offset= disp16(file);
            uint16_t segment= disp16(file);
            printf("JMP 0x%04x:0x%04x\n",segment,offset);
            break;
        }
        /*case 11111111 mod 101 géré ==> p1*/
        
        case 0b11000011:
        {
            printf("RET\n");
            break;
        }
        case 0b11000010:
        {
            printf("RET 0x%04x\n", disp16(file));
            break;
        }
        case 0b11001011:
        {
            printf("RETF\n");
            break;
        }
        case 0b11001010:
        {
            printf("RETF 0x%04x\n", disp16(file));
            break;
        }
        case 0b01110100:
        {
            printf("JE 0x%02x\n", disp8(file));
            break;
        }
        
        case 0b01111100:
        {
            printf("JL 0x%02x\n", disp8(file));
            break;
        }
        case 0b01111110:
        {
            printf("JLE 0x%02x\n", disp8(file));
            break;
        }
        case 0b01110010:
        {
            printf("JB 0x%02x\n", disp8(file));
            break;
        }
        case 0b01110110:
        {
            printf("JBE 0x%02x\n", disp8(file));
            break;
        }
        case 0b01111010:
        {
            printf("JP 0x%02x\n", disp8(file));
            break;
        }

        case 0b01110000:
        {
            printf("JO 0x%02x\n", disp8(file));
            break;
        }
        case 0b01111000:
        {
            printf("JS 0x%02x\n", disp8(file));
            break;
        }
        case 0b01110101:
        {
            printf("JNE 0x%02x\n", disp8(file));
            break;
        }
        case 0b01111101:
        {
            printf("JNL 0x%02x\n", disp8(file));
            break;
        }
        case 0b01111111:
        {
            printf("JNLE 0x%02x\n", disp8(file));
            break;
        }

        case 0b01110011:
        {
            printf("JNB 0x%02x\n", disp8(file));
            break;
        }
        case 0b01110111:
        {
            printf("JNBE 0x%02x\n", disp8(file));
            break;
        }
        case 0b01111011:
        {
            printf("JNP 0x%02x\n", disp8(file));
            break;
        }
        case 0b01110001:
        {
            printf("JNO 0x%02x\n", disp8(file));
            break;
        }
        case 0b01111001:
        {
            printf("JNS 0x%02x\n", disp8(file));
            break;
        }
    
        case 0b11100010:
        {
            printf("LOOP 0x%02x\n", disp8(file));
            break;
        }
        case 0b11100001:
        {
            printf("LOOPZ 0x%02x\n", disp8(file));
            break;
        }
        case 0b11100000:
        {
            printf("LOOPNZ 0x%02x\n", disp8(file));
            break;
        }
        case 0b11100011:
        {
            printf("JCXZ 0x%02x\n", disp8(file));
            break;
        }

        case 0b11001101:
        {
            printf("INT 0x%02x\n", disp8(file));
            break;
        }
        case 0b11001100:
        {
            printf("INT 3\n");
            break;
        }
        case 0b11001110:
        {
            printf("INTO\n");
            break;
        }
        case 0b11001111:
        {
            printf("IRET\n");
            break;
        }
        

        // =================================================== page 5 ===================================================
        case 0b11111000:
        {
            printf("CLC\n");
            break;
        }
        case 0b11110101:
        {
            printf("CMC\n");
            break;
        }
        case 0b11111001:
        {
            printf("STC\n");
            break;
        }
        case 0b11111100:
        {
            printf("CLD\n");
            break;
        }
        case 0b11111101:
        {
            printf("STD\n");
            break;
        }
        case 0b11111010:
        {
            printf("CLI\n");
            break;
        }
        case 0b11111011:
        {
            printf("STI\n");
            break;
        }
        case 0b11110100:
        {
            printf("HLT\n");
            break;
        }
        case 0b10011011:
        {
            printf("WAIT\n");
            break;
        }
        case 0b11110000:
        {
            printf("LOCK\n");
            break;
        }

        default:
        {
            //printf("[DECODE8BITS DEBUG] -----> unknown opcode (0x%02x)\n",curr); 
            return 0;
        }
    }
    return 1;
}

int decode7bits(FILE *file, unsigned char curr, int w, int d)
{
    switch(curr & 0b11111110)
    {
    // =================================================== page 1 ===================================================
        case 0b11000110:
        {
            ModRM m= decode_modrm(file,w);
            if (w==0)
            {
                printf("MOV %s, 0x%02x\n",m.rm_name,disp8(file));
            }
            else if (w==1)
            {
                printf("MOV %s, 0x%04x\n",m.rm_name,disp16(file));
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b1100011w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b10100000:
        {
            if (w==0)
            {
                printf("MOV AL, [0x%04x]\n", disp16(file));
            }
            else if (w==1)
            {
                printf("MOV AX, [0x%04x]\n", disp16(file));
            }
            else 
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b1010000w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b10100010:
        {
            if (w==0)
            {
                printf("MOV [0x%04x], AL\n", disp16(file));
            }
            else if (w==1)
            {
                printf("MOV [0x%04x], AX\n", disp16(file));
            }
            else 
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b1010001w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b10000110:
        {
            ModRM m= decode_modrm(file,w);
            printf("XCHG %s, %s\n", m.reg_name, m.rm_name);
            break;
        }
        case 0b11100100:
        {
            if (w==0)
            {
                printf("IN AL, 0x%02x\n", disp8(file));
            }
            else if (w==1)
            {
                printf("IN AX, 0x%02x\n", disp8(file));
            }
            else 
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b1110010w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b11101100:
        {
            if (w==0)
            {
                printf("IN AL, DX\n");
            }
            else if (w==1)
            {
                printf("IN AX, DX\n");
            }
            else 
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b1110110w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b11100110:
        {
            if (w==0)
            {
                printf("OUT 0x%02x, AL\n", disp8(file));
            }
            else if (w==1)
            {
                printf("OUT 0x%02x, AX\n", disp8(file));
            }
            else 
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b1110011w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b11101110:
        {
            if (w==0)
            {
                printf("OUT AL, DX\n");
            }
            else if (w==1)
            {
                printf("OUT AX, DX\n");
            }
            else 
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b1110111w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }


    // =================================================== page 2 ===================================================
        case 0b00000100:
        {
            if (w==0)
            {
                printf("ADD AL, 0x%02x\n",disp8(file));
            }
            else if (w==1)
            {
                printf("ADD AX, 0x%04x\n",disp16(file));
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b0000010w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b00010100:
        {
            if (w==0)
            {
                printf("ADC AL, 0x%02x\n",disp8(file));
            }
            else if (w==1)
            {
                printf("ADC AX, 0x%04x\n",disp16(file));
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b0001010w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b11111110:
        {
            ModRM m=decode_modrm(file,w);
            switch (m.reg)
            {
                case 0b000:
                {
                    printf("INC %s\n",m.rm_name);
                    break;
                }
                case 0b001:
                {
                    printf("DEC %s\n",m.rm_name);
                    break;
                }
                default:
                {
                    printf("[DECODE7BITS DEBUG] -----> case 0b1111111w: m.reg inconnu\n");
                    return 0;
                }
            }
            break;
        }
        case 0b00101100:
        {
            if (w==0)
            {
                printf("SUB AL, 0x%02x\n",disp8(file));
            }
            else if (w==1)
            {
                printf("SUB AX, 0x%04x\n",disp16(file));
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b0010110w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b00011100:
        {
            if (w==0)
            {
                printf("SSB AL, 0x%02x\n",disp8(file));
            }
            else if (w==1)
            {
                printf("SSB AX, 0x%04x\n",disp16(file));
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b0001110w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        /*case 1111111w mod 001 géré ==> p2*/
        case 0b11110110:
        {
            ModRM m=decode_modrm(file,w);
            switch (m.reg)
            {
                case 0b011:
                {
                    printf("NEG %s\n",m.rm_name);
                    break;
                }
                case 0b100:
                {
                    printf("MUL %s\n",m.rm_name);
                    break;
                }
                case 0b101:
                {
                    printf("IMUL %s\n",m.rm_name);
                    break;
                }
                case 0b110:
                {
                    printf("DIV %s\n",m.rm_name);
                    break;
                }
                case 0b111:
                {
                    printf("IDIV %s\n",m.rm_name);
                    break;
                }
                case 0b010:
                {
                    printf("NOT %s\n",m.rm_name);
                    break;
                }
                case 0b000:
                {
                    if (w==0)
                        printf("TEST %s, 0x%02x\n",m.rm_name,disp8(file));
                    else if (w==1)
                        printf("TEST %s, 0x%04x\n",m.rm_name,disp16(file));
                    else
                    {
                        printf("[DECODE7BITS DEBUG] -----> case 0b1111011w, reg=0b000: w!=1 ou w!=0\n");
                        return 0;
                    }
                    break;
                }
                default:
                {
                    printf("[DECODE7BITS DEBUG] -----> case 0b1111011w: m.reg inconnu\n");
                    return 0;
                }
            }            
            break;
        }
        case 0b00111100:
        {
            if (w==0)
            {
                printf("CMP AL, 0x%02x\n",disp8(file));
            }
            else if (w==1)
            {
                printf("CMP AX, 0x%04x\n",disp16(file));
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b0011110w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        /*case 1111011w mod 100 géré ==> p2*/
        /*case 1111011w mod 101 géré ==> p2*/
        /*case 1111011w mod 110 géré ==> p2*/
        /*case 1111011w mod 111 géré ==> p2*/

    // =================================================== page 3 ===================================================
        /*case 1111011w mod 010 géré ==> p2*/
        /*case 0b10000000:
        {
            ModRM m=decode_modrm(file,w);
            switch (m.reg)
            {
                case 0b100:
                {
                    if (w==0)
                        printf("AND %s, 0x%02x\n",m.rm_name,disp8(file));
                    else if (w==1)
                        printf("AND %s, 0x%04x\n",m.rm_name,disp16(file));
                    else
                    {
                        printf("[DECODE7BITS DEBUG] -----> case 0b1000000w, reg=0b100: w!=1 ou w!=0\n");
                        return 0;
                    }
                    break;
                }
                case 0b001:
                {
                    if (w==0)
                        printf("OR %s, 0x%02x\n",m.rm_name,disp8(file));
                    else if (w==1)
                        printf("OR %s, 0x%04x\n",m.rm_name,disp16(file));
                    else
                    {
                        printf("[DECODE7BITS DEBUG] -----> case 0b1000000w, reg=0b001: w!=1 ou w!=0\n");
                        return 0;
                    }
                    break;
                }
                case 0b110:
                {
                    if (w==0)
                        printf("XOR %s, 0x%02x\n",m.rm_name,disp8(file));
                    else if (w==1)
                        printf("XOR %s, 0x%04x\n",m.rm_name,disp16(file));
                    else
                    {
                        printf("[DECODE7BITS DEBUG] -----> case 0b1000000w, reg=0b110: w!=1 ou w!=0\n");
                        return 0;
                    }
                    break;
                }
                default:
                {
                    printf("[DECODE7BITS DEBUG] -----> case 0b1000000w: m.reg inconnu (%d)\n",m.reg);
                    return 0;
                }
            }            
            break;
        }*/
        case 0b00100100:
        {
            if (w==0)
            {
                printf("AND AL, 0x%02x\n",disp8(file));
            }
            else if (w==1)
            {
                printf("AND AX, 0x%04x\n",disp16(file));
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b0010010w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b10000100:
        {
            ModRM m= decode_modrm(file,w);
            printf("TEST %s, %s\n", m.reg_name, m.rm_name);
            break;
        }
        /*case 1111011w mod 000 géré ==> p2*/
        case 0b10101000:
        {
            if (w==0)
            {
                printf("TEST AL, 0x%02x\n",disp8(file));
            }
            else if (w==1)
            {
                printf("TEST AX, 0x%04x\n",disp16(file));
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b1010100w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        /*case 1000000w mod 001 géré ==> p2*/
        case 0b00001100:
        {
            if (w==0)
            {
                printf("OR AL, 0x%02x\n",disp8(file));
            }
            else if (w==1)
            {
                printf("OR AX, 0x%04x\n",disp16(file));
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b000110w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }

        /*case 1000000w mod 110 géré ==> p2*/
        case 0b00110100:
        {
            if (w==0)
            {
                printf("XOR AL, 0x%02x\n",disp8(file));
            }
            else if (w==1)
            {
                printf("XOR AX, 0x%04x\n",disp16(file));
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b0011010w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b11110010:
        {
            printf("REP ");
            instructionDecoding(file);
            break;
        }
        case 0b10100100:
        {
            if (w==0)
            {
                printf("MOVSB\n");
            }
            else if (w==1)
            {
                printf("MOVSW\n");
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b1010010w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b10100110:
        {
            if (w==0)
            {
                printf("CMPSB\n");
            }
            else if (w==1)
            {
                printf("CMPSW\n");
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b1010011w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b10101110:
        {
            if (w==0)
            {
                printf("SCASB\n");
            }
            else if (w==1)
            {
                printf("SCASW\n");
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b1010111w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b10101100:
        {
            if (w==0)
            {
                printf("LODSB\n");
            }
            else if (w==1)
            {
                printf("LODSW\n");
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b1010110w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        case 0b10101010:
        {
            if (w==0)
            {
                printf("STOSB\n");
            }
            else if (w==1)
            {
                printf("STOSW\n");
            }
            else
            {
                printf("[DECODE7BITS DEBUG] -----> case 0b1010101w: w!=1 ou w!=0\n");
                return 0;
            }
            break;
        }
        default:
        {
            //printf("[DECODE7BITS DEBUG] -----> unknown opcode (0x%02x)\n",curr); 
            return 0;
        }
    }
    return 1; 
}

int decode6bits(FILE *file, unsigned char curr, int w, int d)
{
    switch(curr & 0b11111100)
    {
    // =================================================== page 1 =================================================== 
        case 0b10001000: 
        {
            ModRM m= decode_modrm(file,w);
            if (d==0)
            {
                printf("MOV %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                printf("MOV %s, %s\n", m.reg_name, m.rm_name);
            }
            else 
            {
                printf("[DECODE6BITS DEBUG] -----> case 0b100010dw: d!=1 ou d!=0\n");
                return 0;
            }
            break;
        }
    
    
    // =================================================== page 2 ===================================================   
        case 0b00000000:
        {
            ModRM m= decode_modrm(file,w);
            if (d==0)
            {
                printf("ADD %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                printf("ADD %s, %s\n", m.reg_name, m.rm_name);
            }
            else 
            {
                printf("[DECODE6BITS DEBUG] -----> case 0b000000dw: d!=1 ou d!=0\n");
                return 0;
            }
            break;
        }
        case 0b10000000:
        {
            ModRM m= decode_modrm(file,w);
            int s=d;
            switch (m.reg)
            {
                case 0b100:
                {
                    if (w==0)
                        printf("AND %s, 0x%02x\n",m.rm_name,disp8(file));
                    else if (w==1)
                        printf("AND %s, 0x%04x\n",m.rm_name,disp16(file));
                    else
                    {
                        printf("[DECODE6BITS DEBUG]. -----> case 0b1000000w, reg=0b100: w!=1 ou w!=0\n");
                        return 0;
                    }
                    break;
                }
                case 0b001:
                {
                    if (w==0)
                        printf("OR %s, 0x%02x\n",m.rm_name,disp8(file));
                    else if (w==1)
                        printf("OR %s, 0x%04x\n",m.rm_name,disp16(file));
                    else
                    {
                        printf("[DECODE6BITS DEBUG]. -----> case 0b1000000w, reg=0b001: w!=1 ou w!=0\n");
                        return 0;
                    }
                    break;
                }
                case 0b110:
                {
                    if (w==0)
                        printf("XOR %s, 0x%02x\n",m.rm_name,disp8(file));
                    else if (w==1)
                        printf("XOR %s, 0x%04x\n",m.rm_name,disp16(file));
                    else
                    {
                        printf("[DECODE6BITS DEBUG]. -----> case 0b1000000w, reg=0b110: w!=1 ou w!=0\n");
                        return 0;
                    }
                    break;
                }
                case 0b000:
                {
                    if (s==0)
                    {
                        if (w==0)
                        {
                            printf("ADD %s, 0x%02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            printf("ADD %s, 0x%04x\n", m.rm_name, disp16(file));
                        }
                        else
                        {
                            printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b000: w!=1 ou w!=0\n");
                            return 0;
                        }
                        break;
                    }
                    else if (s==1)
                    {
                        if (w==0)
                        {
                            printf("ADD %s, 0x%02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            printf("ADD %s, 0x%04x\n", m.rm_name, (int16_t)(int8_t)disp8(file));
                        }
                        else
                        {
                            printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b000: w!=1 ou w!=0\n");
                            return 0;
                        }
                        break;
                    }
                    else 
                    {
                        printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b000: s!=1 ou s!=0\n");
                        return 0;
                    }
                    break;
                }
                case 0b010:
                {
                    if (s==0) 
                    {
                        if (w==0)
                        {
                            printf("ADC %s, 0x%02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            printf("ADC %s, 0x%04x\n", m.rm_name, disp16(file));
                        }
                        else
                        {
                            printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b010: w!=1 ou w!=0\n");
                            return 0;
                        }
                        break;
                    }
                    else if (s==1)
                    {
                        if (w==0)
                        {
                            printf("ADC %s, 0x%02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            printf("ADC %s, 0x%04x\n", m.rm_name, (int16_t)(int8_t)disp8(file));
                        }
                        else
                        {
                            printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b010: w!=1 ou w!=0\n");
                            return 0;
                        }
                        break;
                    }
                    else 
                    {
                        printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b010: s!=1 ou s!=0\n");
                        return 0;
                    }
                    break;
                }
                case 0b101:
                {
                    if (s==0) 
                    {
                        if (w==0)
                        {
                            printf("SUB %s, 0x%02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            printf("SUB %s, 0x%04x\n", m.rm_name, disp16(file));
                        }
                        else
                        {
                            printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b101: w!=1 ou w!=0\n");
                            return 0;
                        }
                        break;
                    }
                    else if (s==1)
                    {
                        if (w==0)
                        {
                            printf("SUB %s, 0x%02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            printf("SUB %s, 0x%04x\n", m.rm_name, (int16_t)(int8_t)disp8(file));
                        }
                        else
                        {
                            printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b101: w!=1 ou w!=0\n");
                            return 0;
                        }
                        break;
                    }
                    else 
                    {
                        printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b101: s!=1 ou s!=0\n");
                        return 0;
                    }
                    break;
                }
                case 0b011:
                {
                    if (s==0) 
                    {
                        if (w==0)
                        {
                            printf("SSB %s, 0x%02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            printf("SSB %s, 0x%04x\n", m.rm_name, disp16(file));
                        }
                        else
                        {
                            printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b011: w!=1 ou w!=0\n");
                            return 0;
                        }
                        break;
                    }
                    else if (s==1)
                    {
                        if (w==0)
                        {
                            printf("SSB %s, 0x%02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            printf("SSB %s, 0x%04x\n", m.rm_name, (int16_t)(int8_t)disp8(file));
                        }
                        else
                        {
                            printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b011: w!=1 ou w!=0\n");
                            return 0;
                        }
                        break;
                    }
                    else 
                    {
                        printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b011: s!=1 ou s!=0\n");
                        return 0;
                    }
                    break;
                }
                case 0b111:
                {
                    if (s==0) 
                    {
                        if (w==0)
                        {
                            printf("CMP %s, 0x%02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            printf("CMP %s, 0x%04x\n", m.rm_name, disp16(file));
                        }
                        else
                        {
                            printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b111: w!=1 ou w!=0\n");
                            return 0;
                        }
                        break;
                    }
                    else if (s==1)
                    {
                        if (w==0)
                        {
                            printf("CMP %s, 0x%02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            printf("CMP %s, 0x%04x\n", m.rm_name, (int16_t)(int8_t)disp8(file));
                        }
                        else
                        {
                            printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b111: w!=1 ou w!=0\n");
                            return 0;
                        }
                        break;
                    }
                    else 
                    {
                        printf("[DECODE6BITS DEBUG] -----> case 0b100000sw, reg=0b111: s!=1 ou s!=0\n");
                        return 0;
                    }
                    break;
                }
                default:
                {
                    printf("[DECODE6BITS DEBUG] -----> case 0b100000sw: m.reg inconnu\n");
                    return 0;
                }
            }
            break;
        }
        case 0b00010000:
        {
            ModRM m= decode_modrm(file,w);
            if (d==0)
            {
                printf("ADC %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                printf("ADC %s, %s\n", m.reg_name, m.rm_name);
            }
            else 
            {
                printf("[DECODE6BITS DEBUG] -----> case 0b000100dw: d!=1 ou d!=0\n");
                return 0;
            }
            break;
        }
        /*case 100000sw mod 010 géré ==> p2*/
        case 0b00101000:
        {
            ModRM m= decode_modrm(file,w);
            if (d==0)
            {
                printf("SUB %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                printf("SUB %s, %s\n", m.reg_name, m.rm_name);
            }
            else 
            {
                printf("[DECODE6BITS DEBUG] -----> case 0b001010dw: d!=1 ou d!=0\n");
                return 0;
            }
            break;
        }
        /*case 100000sw mod 101 géré ==> p2*/
        case 0b00011000:
        {
            ModRM m= decode_modrm(file,w);
            if (d==0)
            {
                printf("SSB %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                printf("SSB %s, %s\n", m.reg_name, m.rm_name);
            }
            else 
            {
                printf("[DECODE6BITS DEBUG] -----> case 0b000110dw: d!=1 ou d!=0\n");
                return 0;
            }
            break;
        }
        /*case 100000sw mod 011 géré ==> p2*/
        case 0b00111000:
        {
            ModRM m= decode_modrm(file,w);
            if (d==0)
            {
                printf("CMP %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                printf("CMP %s, %s\n", m.reg_name, m.rm_name);
            }
            else 
            {
                printf("[DECODE6BITS DEBUG] -----> case 0b001110dw: d!=1 ou d!=0\n");
                return 0;
            }
            break;
        }
        /*case 100000sw mod 111 géré ==> p2*/
        

    // =================================================== page 3 ===================================================       
        case 0b11010000:
        {
            ModRM m= decode_modrm(file,w);
            int v=d;
            char* count;
            if (v==0)
                count="1";
            else if (v==1)
                count="CL";
            else
            {
                printf("[DECODE6BITS DEBUG] -----> case 0b110100vw: v!=1 ou v!=0\n");
                return 0;
            }

            switch (m.reg)
            {
                case 0b100: 
                {
                    printf("SHL %s, %s\n", m.rm_name, count);
                    break;
                }
                case 0b101:
                {
                    printf("SHR %s, %s\n", m.rm_name, count);
                    break;
                }
                case 0b111:
                {
                    printf("SAR %s, %s\n", m.rm_name, count);
                    break;
                }
                case 0b000:
                {
                    printf("ROL %s, %s\n", m.rm_name, count);
                    break;
                }
                case 0b001:
                {
                    printf("ROR %s, %s\n", m.rm_name, count);
                    break;
                }
                case 0b010:
                {
                    printf("RCL %s, %s\n", m.rm_name, count);
                    break;
                }
                case 0b011:
                {
                    printf("RCR %s, %s\n", m.rm_name, count);
                    break;
                }
                default:
                {
                    printf("[DECODE6BITS DEBUG] -----> case 0b110100vw: m.reg inconnu\n");
                    return 0;
                }
            }
            break;
        }
        case 0b00100000:
        {
            ModRM m= decode_modrm(file,w);
            if (d==0)
            {
                printf("AND %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                printf("AND %s, %s\n", m.reg_name, m.rm_name);
            }
            else 
            {
                printf("[DECODE6BITS DEBUG] -----> case 0b001000dw: d!=1 ou d!=0\n");
                return 0;
            }
            break;
        }
        case 0b00001000:
        {
            ModRM m= decode_modrm(file,w);
            if (d==0)
            {
                printf("OR %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                printf("OR %s, %s\n", m.reg_name, m.rm_name);
            }
            else 
            {
                printf("[DECODE6BITS DEBUG] -----> case 0b000010dw: d!=1 ou d!=0\n");
                return 0;
            }
            break;
        }
        case 0b00110000:
        {
            ModRM m= decode_modrm(file,w);
            if (d==0)
            {
                printf("XOR %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                printf("XOR %s, %s\n", m.reg_name, m.rm_name);
            }
            else 
            {
                printf("[DECODE6BITS DEBUG] -----> case 0b001100dw: d!=1 ou d!=0\n");
                return 0;
            }
            break;
        }
        default:
        {
            return 0;
        }
    }
    return 1;
}

int decode5bits(FILE* file, unsigned char curr, int w, int d)
{
    switch(curr & 0b11111000)
    {
        case 0b01010000:
        {
            int reg= curr & 0b00000111;
            char* reg_name= get_reg(reg, 1);
            if (reg_name!=NULL)
            {
                printf("PUSH %s\n",reg_name);
            }
            else
            {
                printf("[DECODE5BITS DEBUG] -----> case 0b01010reg: reg_name NULL\n");
                return 0;
            }
            break;
        }
        case 0b01011000:
        {
            int reg= curr & 0b00000111;
            char* reg_name= get_reg(reg, 1);
            if (reg_name!=NULL)
            {
                printf("POP %s\n",reg_name);
            }
            else
            {
                printf("[DECODE5BITS DEBUG] -----> case 0b01011reg: reg_name NULL\n");
                return 0;
            }
            break;
        }
        case 0b10010000:
        {
            int reg= curr & 0b00000111;
            char* reg_name= get_reg(reg, 1);
            if (reg_name!=NULL)
            {
                printf("XCHG AX, %s\n",reg_name);
            }
            else
            {
                printf("[DECODE5BITS DEBUG] -----> case 0b10010reg: reg_name NULL\n");
                return 0;
            }
            break;
        }
        case 0b01000000:
        {
            int reg= curr & 0b00000111;
            char* reg_name= get_reg(reg, 1);
            if (reg_name!=NULL)
            {
                printf("INC %s\n",reg_name);
            }
            else
            {
                printf("[DECODE5BITS DEBUG] -----> case 0b01000reg: reg_name NULL\n");
                return 0;
            }
            break;
        }
        case 0b01001000:
        {
            int reg= curr & 0b00000111;
            char* reg_name= get_reg(reg, 1);
            if (reg_name!=NULL)
            {
                printf("DEC %s\n",reg_name);
            }
            else
            {
                printf("[DECODE5BITS DEBUG] -----> case 0b01001reg: reg_name NULL\n");
                return 0;
            }
            break;
        }
        case 0b11011000:
        {
            ModRM m=decode_modrm(file,1);
            int ext= ((curr & 0b00000111)<<3) | m.reg;
            printf("ESC 0x%02x, %s\n", ext, m.rm_name);
            break;
        }
        default:
        {
            return 0;
        }
    }
    return 1;
}

int decode4bits(FILE* file, unsigned char curr, int w, int d)
{
    if ((curr & 0b11110000)==0b10110000)
    {
        int w2=(curr >>3)&1;
        int reg= curr & 0b00000111;
        char* reg_name= get_reg(reg, w2);
        if (reg_name==NULL)
        {
            printf("[DECODE4BITS DEBUG] -----> case 0b1011wreg: reg_name NULL\n");
            return 0;
        }

        if (w2==0)
        {
            printf("MOV %s, 0x%02x\n", reg_name, disp8(file));
        }
        else if (w2==1)
        {
            printf("MOV %s, 0x%04x\n", reg_name, disp16(file));
        }
        else
        {
            printf("[DECODE4BITS DEBUG] -----> case 0b1011wreg: w!=1 ou w!=0\n");
            return 0;
        }
    }
    else
    {
        return 0;
    }
    return 1;
}

int decode3bits(FILE* file, unsigned char curr, int w, int d)
{
    if ((curr & 0b11100111)==0b00000110)
    {
        int segment=(curr>>3)&0b00000011;
        char*seg_name=get_seg(segment);
        if (seg_name!=NULL)
        {
            printf("PUSH %s\n",seg_name);
        }
        else
        {
            printf("[DECODE3BITS DEBUG] -----> case 0b000reg110: seg_name NULL\n");
            return 0;
        }

    }
    else if ((curr & 0b11100111)==0b00000111)
    {
        int segment=(curr>>3)&0b00000011;
        char*seg_name=get_seg(segment);
        if (seg_name!=NULL)
        {
            printf("POP %s\n",seg_name);
        }
        else
        {
            printf("[DECODE3BITS DEBUG] -----> case 0b000reg110: seg_name NULL\n");
            return 0;
        }
    }
    else
    {
        return 0;
    }
    return 1;
}


void instructionDecoding(FILE * file)
{
    unsigned char curr;
    if (fread(&curr,1,1,file)!=1)
        return;

    int w=curr & 0b00000001;
    int d=(curr>>1) & 0b0000001;

    if (decode8bits(file,curr,w,d)==1)
        return;
    else if (decode7bits(file,curr,w,d)==1)
        return;
    else if (decode6bits(file,curr,w,d)==1)
        return;
    else if (decode5bits(file,curr,w,d)==1)
        return;
    else if (decode4bits(file,curr,w,d)==1)
        return;
    else if (decode3bits(file,curr,w,d)==1)
        return;
    else
        printf("[INSTRUCTIONDECODING DEBUG] -----> unknown opcode (0x%02x)\n",curr); 

}