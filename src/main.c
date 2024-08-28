/*
Tito Guidotti - R.A. 10388629

Agosto de 2024

https://giflib.sourceforge.net/whatsinagif/bits_and_bytes.html
https://docs.fileformat.com/image/gif/

*/
#include <stdio.h>
#include <stdlib.h>
#include "lzw.c"

#define byte unsigned char

// https://stackoverflow.com/a/38417562/10480432
#define bytes_to_u16(MSB, LSB) (((unsigned int)((byte)MSB)) & 255) << 8 | (((byte)LSB) & 255)

#define bool2str(bool) bool ? "v" : "f"

void printBits(size_t const size, void const *const ptr)
{
    byte *b = (byte *)ptr;
    byte a;
    int i, j;

    for (i = size - 1; i >= 0; i--)
    {
        for (j = 7; j >= 0; j--)
        {
            a = (b[i] >> j) & 1;
            printf("%u", a);
        }
    }
    puts("");
}

typedef struct Color
{
    byte r;
    byte g;
    byte b;
} Color;

typedef struct GraphicsControlExtension
{
    byte block_size;
    byte disposal_method;
    byte user_input_flag;
    byte transparent_color_flag;
    unsigned int delay_time;
    byte transparent_color_index;
} GraphicsControlExtension;

void getGraphicsControlExtension(FILE *arq, GraphicsControlExtension *g)
{
    g->block_size = getc(arq);
    byte packed = getc(arq);
    g->disposal_method = (packed >> 2) & 7;
    g->user_input_flag = (packed >> 1) & 1;
    g->transparent_color_flag = packed & 1;
    g->transparent_color_index = getc(arq);

    char delay_bytes[2];
    delay_bytes[0] = getc(arq);
    delay_bytes[0] = getc(arq);
    g->delay_time = bytes_to_u16(delay_bytes[1], delay_bytes[0]);

    // le ultimo byte vazio
    packed = getc(arq);
    if (packed)
    {
        puts("Erro: Byte deveria ser vazio");
    }
}

void printGraphicsControlExtension(GraphicsControlExtension *g)
{
    printf("block_size: %u, disposal_method: %u, user_input_flag: %u, transparent_color_flag: %u, delay: %u, transparent block index: %u\n", g->block_size, g->disposal_method, g->user_input_flag, g->transparent_color_flag, g->delay_time, g->transparent_color_index);
}

int getSubByteValue(int src, int bits, int offset){
    return src >> offset & (~0 >> sizeof(int)-bits);
}

int main(int argc, byte *argv[])
{   
    FILE * arq;
    if (argc == 1)
    {
        // printf("Numero inválido de argumentos.\nUso: gig.exe <imagem.gif>\n");
        // return 1;
        arq = fopen("../sample.gif", "rb");
    }
    else{
        arq = fopen(argv[1], "rb");
    }
    if (!arq)
    {
        fprintf(stderr, "error: file open failed '%s'.\n", argv[1]);
        return 1;
    }
    byte buffer[3];
    for (int i = 0; i < 3; i++)
        buffer[i] = getc(arq);

    if (buffer[0] != 'G' || buffer[1] != 'I' || buffer[2] != 'F')
    {
        fprintf(stderr, "erro: arq não é um GIF\n");
        return 1;
    }

    for (int i = 0; i < 3; i++)
        buffer[i] = getc(arq);

    byte flag89a = 0;

    if (buffer[0] == '8' && buffer[1] == '9' && buffer[2] == 'a')
    {
        printf("É um GIF de versão 89a\n");
        flag89a = 1;
    }
    else if (buffer[0] == '8' && buffer[1] == '7' && buffer[2] == 'a')
    {
        printf("É um GIF de versão 87a\n");
    }
    else
    {
        fprintf(stderr, "erro: versão inválida (%c%c%c)\n", buffer[0], buffer[1], buffer[2]);
        return 1;
    }

    buffer[0] = getc(arq);
    buffer[1] = getc(arq);

    unsigned int canvas_width = bytes_to_u16(buffer[1], buffer[0]);

    buffer[0] = getc(arq);
    buffer[1] = getc(arq);

    unsigned int canvas_height = bytes_to_u16(buffer[1], buffer[0]);

    printf("Dimensões: %u x %u\n", canvas_width, canvas_height);

    byte packed_field = getc(arq);

    byte global_color_table_flag = (packed_field >> 7) & 1;
    byte bits_per_pixel = ((packed_field >> 4) & 7) + 1;
    byte sort_flag = (packed_field >> 3) & 1;
    int global_color_table_size = 1 << ((packed_field & 7) + 1); // 2^(N+1)

    printf("Global color table flag: %s\n", bool2str(global_color_table_flag));
    printf("Bits per pixel: %u\n", bits_per_pixel);
    printf("As cores%s estão ordenadas por frequencia\n", sort_flag ? "" : " não");
    printf("Tamanho tabela de cores: %u\n", global_color_table_size);

    // dois valores não usados hoje em dia
    buffer[0] = getc(arq); // background_color_index - para cores de fundo não especificadas
    buffer[1] = getc(arq); // pixel aspect ratio

    Color *global_color_table = 0;

    if (global_color_table_flag)
    {
        global_color_table = malloc(sizeof(Color) * global_color_table_size);
        printf("\nTabela global de cores\n");
        for (unsigned int i = 0; i < global_color_table_size; i++)
        {
            global_color_table[i].r = getc(arq);
            global_color_table[i].g = getc(arq);
            global_color_table[i].b = getc(arq);
            printf("Cor %u: (%u, %u, %u)\n", i + 1, global_color_table[i].r, global_color_table[i].g, global_color_table[i].b);
        }
    }

    byte possuiGraphicsControlExtension = 0;
    GraphicsControlExtension graphicsControlExtension;

    while (1)
    {
        buffer[0] = getc(arq);
        // codigo de Extension
        if (buffer[0] == 33)
        {
            puts("Extensão");
            buffer[0] = getc(arq);
            // codigo de GraphicsControlExtension
            if (buffer[0] == 249)
            {
                getGraphicsControlExtension(arq, &graphicsControlExtension);
                possuiGraphicsControlExtension = 1;
                printGraphicsControlExtension(&graphicsControlExtension);
            }
            // text extension
            else if(buffer[0] == 1){
                printf("Extensão de texto\nO texto é:\"");
                byte block_size = getc(arq);
                while (block_size)
                {
                    for (size_t i = 0; i < block_size; i++)
                        putchar(getc(arq));
                    block_size = getc(arq);
                }
                
            }
            //application extension
            else if(buffer[0]==255){
                printf("Extesão de aplicativo\nConteudo é:\"");
                byte block_size = getc(arq);
                while (block_size)
                {
                    for (size_t i = 0; i < block_size; i++)
                        putchar(getc(arq));
                    block_size = getc(arq);
                }
                puts("\"");
            }
            // Ignora demais codigos
            else
            {
                printf("Ignorando extension %u\n", buffer[0]);
                byte tamanho = getc(arq);
                for (byte i = 0; i < tamanho; i++)
                    getc(arq);
            }
        }
        // Image descriptor
        else if (buffer[0] == 44) //x2C
        {
            puts("Aqui começa uma das imagens");
            buffer[0] = getc(arq);
            buffer[1] = getc(arq);
            unsigned int left = bytes_to_u16(buffer[1], buffer[0]);
            buffer[0] = getc(arq);
            buffer[1] = getc(arq);
            unsigned int top = bytes_to_u16(buffer[1], buffer[0]);
            buffer[0] = getc(arq);
            buffer[1] = getc(arq);
            unsigned int width = bytes_to_u16(buffer[1], buffer[0]);
            buffer[0] = getc(arq);
            buffer[1] = getc(arq);
            unsigned int height = bytes_to_u16(buffer[1], buffer[0]);

            printf("Imagem tem o rect (%u, %u, %u, %u)\n", left, top, width, height);

            byte packed = getc(arq);
            byte local_color_table_flag = (packed >> 7) & 1;
            byte interlace_flag = (packed >> 6) & 1;
            byte sort_flag = (packed >> 5) & 1;
            byte local_table_size = 1 << ((packed & 7) + 1); // 2^(N+1);

            Color *local_color_table = 0;

            if (local_color_table_flag)
            {
                local_color_table = malloc(sizeof(Color) * local_table_size);
                printf("\nTabela local de cores\n");
                for (unsigned int i = 0; i < local_table_size; i++)
                {
                    local_color_table[i].r = getc(arq);
                    local_color_table[i].g = getc(arq);
                    local_color_table[i].b = getc(arq);
                    printf("Cor %u: (%u, %u, %u)\n", i + 1, local_color_table[i].r, local_color_table[i].g, local_color_table[i].b);
                }
            }

            //aqui começa os dados da imagem em si

            byte LZW_minimum_code_size = getc(arq);
            printf("LZW minimum code size: %u\n",LZW_minimum_code_size);
            byte first_code_size = LZW_minimum_code_size+1;

            byte block_size = getc(arq);
            
            puts("Iniciando decompressão");

            int bundledCodesSize = block_size;
            int bundledCodesOcup = 0;
            byte* bundledCodes = malloc(bundledCodesSize);

            while (block_size)
            {
                // byte lastread = getc(arq);

                for (size_t i = 0; i < block_size; i++){
                    byte read = getc(arq);
                    // printf("%u - ",read);
                    bundledCodes[bundledCodesOcup++] = read;
                }
                block_size = getc(arq);
                if(block_size){
                    bundledCodesSize+=block_size;
                    printf("realloc %u\n",bundledCodesSize);
                    bundledCodes = realloc(bundledCodes, bundledCodesSize);
                }
            }          

            byte* codes = malloc(bundledCodesOcup);
            int unbundled_len = LZW_decompress_from_bitstream(bundledCodes, bundledCodesOcup,codes,LZW_minimum_code_size);

            for (int i = 0; i < unbundled_len; i++)
                printf("%u ",codes[i]);
                
            puts("\n");

            free(bundledCodes);
            free(codes);
            

        }
        else if (buffer[0] == 59)
        {
            printf("Fim de arq\n");
            return 0;
        }
        else
        {
            printf("Cheguei em codigo desconhecido: %u\n", buffer[0]);
            return 1;
        }
    }
}