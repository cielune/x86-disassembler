#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


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
void instructionDecoding(FILE * file, uint16_t pc,char*out);
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
            case 0b000: return "ax";
            case 0b001: return "cx";
            case 0b010: return "dx";
            case 0b011: return "bx";
            case 0b100: return "sp";
            case 0b101: return "bp";
            case 0b110: return "si";
            case 0b111: return "di";
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
            case 0b000: return "al";
            case 0b001: return "cl";
            case 0b010: return "dl";
            case 0b011: return "bl";
            case 0b100: return "ah";
            case 0b101: return "ch";
            case 0b110: return "dh";
            case 0b111: return "bh";
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
        case 0b00: return "es";
        case 0b01: return "cs";
        case 0b10: return "ss";
        case 0b11: return "ds";
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
    int16_t d16;
    int8_t d8;

    if (mod==0b00)
    {
        if (rm==0b110)
        {
            sprintf(buffer,"[%04x]",disp16(file));
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
        d8=(int8_t)disp8(file);
        int abs;
        char signe;
        if (d8<0)
        {
            signe='-';
            abs=-d8;
        }
        else 
        {
            signe='+';
            abs=d8;
        }
        switch (rm)
        {
            case 0b000: sprintf(buffer,"[bx+si%c%x]",signe,abs);break;
            case 0b001: sprintf(buffer,"[bx+di%c%x]",signe,abs);break;
            case 0b010: sprintf(buffer,"[bp+si%c%x]",signe,abs);break;
            case 0b011: sprintf(buffer,"[bp+di%c%x]",signe,abs);break;
            case 0b100: sprintf(buffer,"[si%c%x]",signe,abs);break;
            case 0b101: sprintf(buffer,"[di%c%x]",signe,abs);break;
            case 0b110: sprintf(buffer,"[bp%c%x]",signe,abs);break;
            case 0b111: sprintf(buffer,"[bx%c%x]",signe,abs);break;
            default:
                printf("[GET_EA DEBUG] -----> r/m (mod=01) inconnu | \t r/m = %i\n",rm);
                break;
        }
    }
    else if (mod==0b10)
    {
        // ========== 16-bit DISP ==========
        d16=(int16_t)disp16(file);
        int abs;
        char signe;
        if (d16<0)
        {
            signe='-';
            abs=-d16;
        }
        else 
        {
            signe='+';
            abs=d16;
        }
        switch (rm)
        {
            case 0b000: sprintf(buffer,"[bx+si%c%x]",signe,abs);break;
            case 0b001: sprintf(buffer,"[bx+di%c%x]",signe,abs);break;
            case 0b010: sprintf(buffer,"[bp+si%c%x]",signe,abs);break;
            case 0b011: sprintf(buffer,"[bp+di%c%x]",signe,abs);break;
            case 0b100: sprintf(buffer,"[si%c%x]",signe,abs);break;
            case 0b101: sprintf(buffer,"[di%c%x]",signe,abs);break;
            case 0b110: sprintf(buffer,"[bp%c%x]",signe,abs);break;
            case 0b111: sprintf(buffer,"[bx%c%x]",signe,abs);break;
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
    uint16_t pc=0x0000;

    while (curr_pos<text_size)
    {
        long avant=ftell(file);

        char instr[128]={0};
        instructionDecoding(file,pc,instr);
        
        long apres=ftell(file);
        long diff=apres-avant;

        if (diff==0)
        {
            unsigned char idk;
            fseek(file,1,SEEK_CUR);
            printf("[DISASMBBASEDECODING] opcode inconnu (0x%02x), skipped.\n",idk);
            diff=1;
            
        }
        fseek(file,avant,SEEK_SET);
        char hexbuf[32]={0};
        int pos=0;
        
        for (long i=0; i<diff && i<10;i++)
        {
            uint8_t bytesinstru;
            fread(&bytesinstru,1,1,file);
            pos+=sprintf(hexbuf+pos, "%02x",bytesinstru);
        }
        printf("%04x: %-14s%s",pc,hexbuf,instr);
        pc+=(uint16_t)diff;
        curr_pos+=diff;
    }      
}


int decode8bits(FILE *file, unsigned char curr,int w, int d,uint16_t pc,char*out)
{
    switch(curr)
    {
    // =================================================== page 1 ===================================================
        case 0b10001110:
        {
            ModRM m=decode_modrm(file,1);
            sprintf(out,"mov %s, %s\n",get_seg(m.reg),m.rm_name);
            break;
        }
        case 0b10001100:
        {
            ModRM m=decode_modrm(file,1);
            sprintf(out,"mov %s, %s\n",m.rm_name,get_seg(m.reg));
            break;
        }
        case 0b11111111:
        {
            ModRM m=decode_modrm(file,1);
            switch (m.reg)
            {
                case 0b110:
                {
                    sprintf(out,"push %s\n",m.rm_name);
                    break;
                }
                case 0b010:
                {
                    sprintf(out,"call %s\n",m.rm_name);
                    break;
                }
                case 0b011:
                {
                    sprintf(out,"call far %s\n",m.rm_name); 
                    break;
                }
                case 0b100:
                {
                    sprintf(out,"jmp %s\n",m.rm_name); 
                    break;
                }
                case 0b101:
                {
                    sprintf(out,"jmp far %s\n",m.rm_name);
                    break;
                }
                case 0b000:
                {
                    sprintf(out,"inc %s\n",m.rm_name);
                    break;
                }
                case 0b001:
                {
                    sprintf(out,"dec %s\n",m.rm_name);
                    break;
                }
                case 0b111:
                {
                    sprintf(out,"push %s\n",m.rm_name);
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
                sprintf(out,"pop %s\n",m.rm_name);
            }
            else
            {
                printf("[DECODE8BITS DEBUG] -----> case 0b10001111: unknown m.reg (%i)",m.reg);
            }
            break;
        }
        case 0b11010111:
        {
            sprintf(out,"xlat\n");
            break;
        }
        case 0b10001101:
        {
            ModRM m=decode_modrm(file,1);
            sprintf(out,"lea %s, %s\n",m.reg_name,m.rm_name);
            break;
        }
        case 0b11000101:
        {
            ModRM m=decode_modrm(file,1);
            sprintf(out,"lds %s, %s\n",m.reg_name,m.rm_name);
            break;
        }
        case 0b11000100:
        {
            ModRM m=decode_modrm(file,1);
            sprintf(out,"les %s, %s\n",m.reg_name,m.rm_name);
            break;
        }
        case 0b10011111:
        {
            sprintf(out,"lahf\n");
            break;
        }
        case 0b10011110:
        {
            sprintf(out,"sahf\n");
            break;
        }
        case 0b10011100:
        {
            sprintf(out,"pushf\n");
            break;
        }
        case 0b10011101:
        {
            sprintf(out,"popf\n");
            break;
        }


    // =================================================== page 2 ===================================================
        case 0b00110111:
        {
            sprintf(out,"aaa\n");
            break;
        }
        case 0b00100111:
        {
            sprintf(out,"baa\n");
            break;
        }
        case 0b00111111:
        {
            sprintf(out,"aas\n");
            break;
        }
        case 0b00101111:
        {
            sprintf(out,"das\n");
            break;
        }
        case 0b11010100:
        {
            unsigned char tmp;
            fread(&tmp,1,1,file);
            sprintf(out,"aam\n"); 
            break;
        }
        case 0b11010101:
        {
            unsigned char tmp;
            fread(&tmp,1,1,file);
            sprintf(out,"aad\n"); 
            break;
        }
        case 0b10011000:
        {
            sprintf(out,"cbw\n");
            break;
        }
        case 0b10011001:
        {
            sprintf(out,"cwd\n");
            break;
        }


    // =================================================== page 3 ===================================================
        case 0b11101000:
        {
            int16_t offset=(int16_t)disp16(file);
            sprintf(out,"call %04x\n", (uint16_t)(pc+3+offset));
            break;
        }
        /*case 11111111 mod 010 géré ==> p1*/
        case 0b10011010:
        {
            uint16_t offset= disp16(file);
            uint16_t segment= disp16(file);
            sprintf(out,"call %04x:%04x\n",segment,offset);
            break;
        }
        /*case 11111111 mod 011 géré ==> p1*/
    

    // =================================================== page 4 ===================================================
        case 0b11101001:
        {
            int16_t offset=(int16_t)disp16(file);
            sprintf(out,"jmp %04x\n", (uint16_t)(pc+3+(offset)));
            break;
        }
        case 0b11101011:
        {
            int8_t offset=(int8_t)disp8(file);
            sprintf(out,"jmp short %04x\n", (uint16_t)(pc+2+(offset)));
            break;
        }
        /*case 11111111 mod 100 géré ==> p1*/
        case 0b11101010:
        {
            uint16_t offset= disp16(file);
            uint16_t segment= disp16(file);
            sprintf(out,"jmp %04x:%04x\n",segment,offset);
            break;
        }
        /*case 11111111 mod 101 géré ==> p1*/
        
        case 0b11000011:
        {
            sprintf(out,"ret\n");
            break;
        }
        case 0b11000010:
        {
            sprintf(out,"ret %04x\n", disp16(file));
            break;
        }
        case 0b11001011:
        {
            sprintf(out,"retf\n");
            break;
        }
        case 0b11001010:
        {
            sprintf(out,"retf %04x\n", disp16(file));
            break;
        }
        case 0b01110100:
        {
            int8_t offset= (int8_t)disp8(file);
            sprintf(out,"je %04x\n", pc+2+offset);
            break;
        }
        
        case 0b01111100:
        {
            sprintf(out,"jl %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }
        case 0b01111110:
        {
            sprintf(out,"jle %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }
        case 0b01110010:
        {
            sprintf(out,"jb %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }
        case 0b01110110:
        {
            sprintf(out,"jbe %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }
        case 0b01111010:
        {
            sprintf(out,"jp %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }

        case 0b01110000:
        {
            sprintf(out,"jo %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }
        case 0b01111000:
        {
            sprintf(out,"js %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }
        case 0b01110101:
        {
            int8_t offset=(int8_t)disp8(file);
            sprintf(out,"jne %04x\n", (uint16_t)(pc+2+offset));
            break;
        }
        case 0b01111101:
        {
            sprintf(out,"jnl %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }
        case 0b01111111:
        {
            sprintf(out,"jnle %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }

        case 0b01110011:
        {
            sprintf(out,"jnb %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }
        case 0b01110111:
        {
            sprintf(out,"jnbe %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }
        case 0b01111011:
        {
            sprintf(out,"jnp %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }
        case 0b01110001:
        {
            sprintf(out,"jno %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }
        case 0b01111001:
        {
            sprintf(out,"jns %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }
    
        case 0b11100010:
        {
            int8_t offset= (uint16_t)disp8(file);
            sprintf(out,"loop %04x\n", (uint16_t)(pc+2+offset));
            break;
        }
        case 0b11100001:
        {
            sprintf(out,"loopz %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }
        case 0b11100000:
        {
            sprintf(out,"loopnz %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }
        case 0b11100011:
        {
            sprintf(out,"jcxz %04x\n", (uint16_t)(pc+2+((uint16_t)disp8(file))));
            break;
        }

        case 0b11001101:
        {
            sprintf(out,"int %02x\n", disp8(file));
            break;
        }
        case 0b11001100:
        {
            sprintf(out,"int 3\n");
            break;
        }
        case 0b11001110:
        {
            sprintf(out,"into\n");
            break;
        }
        case 0b11001111:
        {
            sprintf(out,"iret\n");
            break;
        }
        

        // =================================================== page 5 ===================================================
        case 0b11111000:
        {
            sprintf(out,"clc\n");
            break;
        }
        case 0b11110101:
        {
            sprintf(out,"cmc\n");
            break;
        }
        case 0b11111001:
        {
            sprintf(out,"stc\n");
            break;
        }
        case 0b11111100:
        {
            sprintf(out,"cld\n");
            break;
        }
        case 0b11111101:
        {
            sprintf(out,"std\n");
            break;
        }
        case 0b11111010:
        {
            sprintf(out,"cli\n");
            break;
        }
        case 0b11111011:
        {
            sprintf(out,"sti\n");
            break;
        }
        case 0b11110100:
        {
            sprintf(out,"hlt\n");
            break;
        }
        case 0b10011011:
        {
            sprintf(out,"wait\n");
            break;
        }
        case 0b11110000:
        {
            sprintf(out,"lock\n");
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

int decode7bits(FILE *file, unsigned char curr, int w, int d,uint16_t pc,char*out)
{
    switch(curr & 0b11111110)
    {
    // =================================================== page 1 ===================================================
        case 0b11000110:
        {
            ModRM m= decode_modrm(file,w);
            if (w==0)
            {
                sprintf(out,"mov byte %s, %02x\n",m.rm_name,disp8(file));
            }
            else if (w==1)
            {
                sprintf(out,"mov %s, %04x\n",m.rm_name,disp16(file));
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
                sprintf(out,"mov al, [%04x]\n", disp16(file));
            }
            else if (w==1)
            {
                sprintf(out,"mov ax, [%04x]\n", disp16(file));
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
                sprintf(out,"mov [%04x], al\n", disp16(file));
            }
            else if (w==1)
            {
                sprintf(out,"mov [%04x], ax\n", disp16(file));
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
            sprintf(out,"xchg %s, %s\n", m.rm_name, m.reg_name);
            break;
        }
        case 0b11100100:
        {
            if (w==0)
            {
                sprintf(out,"in al, %02x\n", disp8(file));
            }
            else if (w==1)
            {
                sprintf(out,"in ax, %02x\n", disp8(file));
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
                sprintf(out,"in al, dx\n");
            }
            else if (w==1)
            {
                sprintf(out,"in ax, dx\n");
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
                sprintf(out,"out %02x, al\n", disp8(file));
            }
            else if (w==1)
            {
                sprintf(out,"out %02x, ax\n", disp8(file));
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
                sprintf(out,"out al, dx\n");
            }
            else if (w==1)
            {
                sprintf(out,"out ax, dx\n");
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
                sprintf(out,"add al, %02x\n",disp8(file));
            }
            else if (w==1)
            {
                sprintf(out,"add ax, %04x\n",disp16(file));
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
                sprintf(out,"adc al, %02x\n",disp8(file));
            }
            else if (w==1)
            {
                sprintf(out,"adc ax, %04x\n",disp16(file));
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
                    sprintf(out,"inc %s\n",m.rm_name);
                    break;
                }
                case 0b001:
                {
                    sprintf(out,"dec %s\n",m.rm_name);
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
                sprintf(out,"sub al, %02x\n",disp8(file));
            }
            else if (w==1)
            {
                sprintf(out,"sub ax, %04x\n",disp16(file));
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
                sprintf(out,"sbb al, %02x\n",disp8(file));
            }
            else if (w==1)
            {
                sprintf(out,"sbb ax, %04x\n",disp16(file));
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
                    sprintf(out,"neg %s\n",m.rm_name);
                    break;
                }
                case 0b100:
                {
                    sprintf(out,"mul %s\n",m.rm_name);
                    break;
                }
                case 0b101:
                {
                    sprintf(out,"imul %s\n",m.rm_name);
                    break;
                }
                case 0b110:
                {
                    sprintf(out,"div %s\n",m.rm_name);
                    break;
                }
                case 0b111:
                {
                    sprintf(out,"idiv %s\n",m.rm_name);
                    break;
                }
                case 0b010:
                {
                    sprintf(out,"not %s\n",m.rm_name);
                    break;
                }
                case 0b000:
                {
                    if (w==0)
                    {
                        uint8_t val8= disp8(file);
                        if (m.mod!=0b11)
                            sprintf(out,"test byte %s, %x\n",m.rm_name,val8);
                        else
                            sprintf(out,"test %s, %x\n",m.rm_name,val8);
                    }
                    else if (w==1)
                    {
                        uint16_t val16= disp16(file);
                        /*if (m.mod!=0b11)
                            sprintf(out,"test %s, %x\n",m.rm_name,val16);
                        else*/
                            sprintf(out,"test %s, %04x\n",m.rm_name,val16);
                    }
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
                sprintf(out,"cmp al, %02x\n",disp8(file));
            }
            else if (w==1)
            {
                sprintf(out,"cmp ax, %04x\n",disp16(file));
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
                        sprintf(out,"and %s, %02x\n",m.rm_name,disp8(file));
                    else if (w==1)
                        sprintf(out,"and %s, %04x\n",m.rm_name,disp16(file));
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
                        sprintf(out,"or %s, %02x\n",m.rm_name,disp8(file));
                    else if (w==1)
                        sprintf(out,"or %s, %04x\n",m.rm_name,disp16(file));
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
                        sprintf(out,"xor %s, %02x\n",m.rm_name,disp8(file));
                    else if (w==1)
                        sprintf(out,"xor %s, %04x\n",m.rm_name,disp16(file));
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
                sprintf(out,"and al, %02x\n",disp8(file));
            }
            else if (w==1)
            {
                sprintf(out,"and ax, %04x\n",disp16(file));
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
            sprintf(out,"test %s, %s\n", m.reg_name, m.rm_name);
            break;
        }
        /*case 1111011w mod 000 géré ==> p2*/
        case 0b10101000:
        {
            if (w==0)
            {
                sprintf(out,"test al, %x\n",disp8(file));
            }
            else if (w==1)
            {
                uint16_t disp16val=disp16(file);
                sprintf(out,"test ax, %04x\n",disp16val);
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
                sprintf(out,"or al, %02x\n",disp8(file));
            }
            else if (w==1)
            {
                sprintf(out,"or ax, %04x\n",disp16(file));
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
                sprintf(out,"xor al, %02x\n",disp8(file));
            }
            else if (w==1)
            {
                sprintf(out,"xor ax, %04x\n",disp16(file));
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
            sprintf(out,"rep ");
            instructionDecoding(file, (uint16_t)(pc+1),out+strlen(out));
            break;
        }
        case 0b10100100:
        {
            if (w==0)
            {
                sprintf(out,"movsb\n");
            }
            else if (w==1)
            {
                sprintf(out,"movsw\n");
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
                sprintf(out,"cmpsb\n");
            }
            else if (w==1)
            {
                sprintf(out,"cmosw\n");
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
                sprintf(out,"scasb\n");
            }
            else if (w==1)
            {
                sprintf(out,"scasw\n");
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
                sprintf(out,"lodsb\n");
            }
            else if (w==1)
            {
                sprintf(out,"lodsw\n");
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
                sprintf(out,"stosb\n");
            }
            else if (w==1)
            {
                sprintf(out,"stosw\n");
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
            //printf("[DECODE7BITS DEBUG] -----> unknown opcode (%02x)\n",curr); 
            return 0;
        }
    }
    return 1; 
}

int decode6bits(FILE *file, unsigned char curr, int w, int d,char*out)
{
    switch(curr & 0b11111100)
    {
    // =================================================== page 1 =================================================== 
        case 0b10001000: 
        {
            ModRM m= decode_modrm(file,w);
            if (d==0)
            {
                sprintf(out,"mov %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                sprintf(out,"mov %s, %s\n", m.reg_name, m.rm_name);
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
                sprintf(out,"add %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                sprintf(out,"add %s, %s\n", m.reg_name, m.rm_name);
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
                        sprintf(out,"and %s, %02x\n",m.rm_name,disp8(file));
                    else if (w==1)
                    {
                        if (s==1)
                        {
                            int8_t idk= (int8_t)disp8(file);
                            if (idk<0)
                                sprintf(out,"and %s, %x\n",m.rm_name,idk);
                            else
                                sprintf(out,"and %s, %x\n",m.rm_name,(uint8_t)idk);
                        }
                        else if (s==0)
                        {
                            sprintf(out,"and %s, %04x\n",m.rm_name,disp16(file));
                        }
                        else
                        {
                            printf("[DECODE6BITS DEBUG]. -----> case 0b1000000w, reg=0b100: s!=1 ou s!=0\n");
                            return 0;
                        }
                    }
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
                        sprintf(out,"or %s, %02x\n",m.rm_name,disp8(file));
                    else if (w==1)
                    {
                        if (s==1)
                        {
                            sprintf(out,"or %s, %04x\n",m.rm_name,(int16_t)(int8_t)disp8(file));
                        }
                        else if (s==0)
                        {
                            sprintf(out,"or %s, %04x\n",m.rm_name,disp16(file));
                        }
                        else
                        {
                            printf("[DECODE6BITS DEBUG]. -----> case 0b1000000w, reg=0b001: s!=1 ou s!=0\n");
                            return 0;
                        }
                    }
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
                        sprintf(out,"xor %s, %02x\n",m.rm_name,disp8(file));
                    else if (w==1)
                    {
                        if (s==1)
                        {
                            sprintf(out,"xor %s, %x\n",m.rm_name,(int16_t)(int8_t)disp8(file));
                        }
                        else if (s==0)
                        {
                            sprintf(out,"xor %s, %x\n",m.rm_name,disp16(file));
                        }
                        else
                        {
                            printf("[DECODE6BITS DEBUG]. -----> case 0b1000000w, reg=0b110: s!=1 ou s!=0\n");
                            return 0;
                        }
                    }
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
                            sprintf(out,"add %s, %02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            sprintf(out,"add %s, %04x\n",m.rm_name,disp16(file));
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
                            sprintf(out,"add %s, %02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            sprintf(out,"add %s, %x\n", m.rm_name, (int16_t)(int8_t)disp8(file));
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
                            sprintf(out,"adc %s, %02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            sprintf(out,"adc %s, %04x\n", m.rm_name, disp16(file));
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
                            sprintf(out,"adc %s, %02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            sprintf(out,"adc %s, %x\n", m.rm_name, (int16_t)(int8_t)disp8(file));
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
                            sprintf(out,"sub %s, %02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            sprintf(out,"sub %s, %04x\n", m.rm_name, disp16(file));
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
                            sprintf(out,"sub %s, %02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            sprintf(out,"sub %s, %x\n", m.rm_name, (int16_t)(int8_t)disp8(file));
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
                            sprintf(out,"sbb %s, %02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            sprintf(out,"sbb %s, %04x\n", m.rm_name, disp16(file));
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
                            sprintf(out,"sbb %s, %02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            sprintf(out,"sbb %s, %x\n", m.rm_name, (int16_t)(int8_t)disp8(file));
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
                            sprintf(out,"cmp byte %s, %x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            sprintf(out,"cmp %s, %04x\n", m.rm_name, disp16(file));
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
                            sprintf(out,"cmp byte %s, %02x\n", m.rm_name, disp8(file));
                        }
                        else if (w==1)
                        {
                            int8_t idk=(int8_t)disp8(file);
                            if (idk<0)
                            {
                                sprintf(out,"cmp %s, %d\n", m.rm_name, idk);
                            }
                            else
                                sprintf(out,"cmp %s, %x\n", m.rm_name, (uint8_t)idk);
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
                sprintf(out,"adc %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                sprintf(out,"adc %s, %s\n", m.reg_name, m.rm_name);
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
                sprintf(out,"sub %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                sprintf(out,"sub %s, %s\n", m.reg_name, m.rm_name);
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
                sprintf(out,"sbb %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                sprintf(out,"sbb %s, %s\n", m.reg_name, m.rm_name);
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
                sprintf(out,"cmp %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                sprintf(out,"cmp %s, %s\n", m.reg_name, m.rm_name);
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
                count="cl";
            else
            {
                printf("[DECODE6BITS DEBUG] -----> case 0b110100vw: v!=1 ou v!=0\n");
                return 0;
            }

            switch (m.reg)
            {
                case 0b100: 
                {
                    sprintf(out,"shl %s, %s\n", m.rm_name, count);
                    break;
                }
                case 0b101:
                {
                    sprintf(out,"shr %s, %s\n", m.rm_name, count);
                    break;
                }
                case 0b111:
                {
                    sprintf(out,"sar %s, %s\n", m.rm_name, count);
                    break;
                }
                case 0b000:
                {
                    sprintf(out,"rol %s, %s\n", m.rm_name, count);
                    break;
                }
                case 0b001:
                {
                    sprintf(out,"ror %s, %s\n", m.rm_name, count);
                    break;
                }
                case 0b010:
                {
                    sprintf(out,"rcl %s, %s\n", m.rm_name, count);
                    break;
                }
                case 0b011:
                {
                    sprintf(out,"rcr %s, %s\n", m.rm_name, count);
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
                sprintf(out,"and %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                sprintf(out,"and %s, %s\n", m.reg_name, m.rm_name);
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
                sprintf(out,"or %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                sprintf(out,"or %s, %s\n", m.reg_name, m.rm_name);
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
                sprintf(out,"xor %s, %s\n", m.rm_name, m.reg_name);
            }
            else if (d==1)
            {
                sprintf(out,"xor %s, %s\n", m.reg_name, m.rm_name);
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

int decode5bits(FILE* file, unsigned char curr, int w, int d,char*out)
{
    switch(curr & 0b11111000)
    {
        case 0b01010000:
        {
            int reg= curr & 0b00000111;
            char* reg_name= get_reg(reg, 1);
            if (reg_name!=NULL)
            {
                sprintf(out,"push %s\n",reg_name);
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
                sprintf(out,"pop %s\n",reg_name);
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
                sprintf(out,"xchg %s, ax\n",reg_name);
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
                sprintf(out,"inc %s\n",reg_name);
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
                sprintf(out,"dec %s\n",reg_name);
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
            sprintf(out,"ESC %02x, %s\n", ext, m.rm_name);
            break;
        }
        default:
        {
            return 0;
        }
    }
    return 1;
}

int decode4bits(FILE* file, unsigned char curr, int w, int d,char*out)
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
            sprintf(out,"mov %s, %02x\n", reg_name, disp8(file));
        }
        else if (w2==1)
        {
            sprintf(out,"mov %s, %04x\n", reg_name, disp16(file));
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

int decode3bits(FILE* file, unsigned char curr, int w, int d,char*out)
{
    if ((curr & 0b11100111)==0b00000110)
    {
        int segment=(curr>>3)&0b00000011;
        char*seg_name=get_seg(segment);
        if (seg_name!=NULL)
        {
            sprintf(out,"push %s\n",seg_name);
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
            sprintf(out,"pop %s\n",seg_name);
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


void instructionDecoding(FILE * file, uint16_t pc, char *out)
{
    uint8_t curr;
    if (fread(&curr,1,1,file)!=1)
        return;

    int w=curr & 0b00000001;
    int d=(curr>>1) & 0b0000001;

    if (decode8bits(file,curr,w,d, pc,out)==1)
        return;
    else if (decode7bits(file,curr,w,d, pc,out)==1)
        return;
    else if (decode6bits(file,curr,w,d,out)==1)
        return;
    else if (decode5bits(file,curr,w,d,out)==1)
        return;
    else if (decode4bits(file,curr,w,d,out)==1)
        return;
    else if (decode3bits(file,curr,w,d,out)==1)
        return;
    else
        printf("[INSTRUCTIONDECODING DEBUG] -----> unknown opcode (%02x)\n",curr); 

}